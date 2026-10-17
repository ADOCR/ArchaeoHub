"""Contratos de evidencia, cobertura, recuperación negativa y publicación sin sobrescrituras."""
from pathlib import Path
import json
import re
import sys
import tempfile
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"modules/documents"))
from document_library import analyze, export, digest
from document_analysis import (enrich, passages, verified_evidence, validate_relation, constrained, conforms, synthesize,
                               LocalModel, READ_SCHEMA, SUMMARY_SCHEMA, PAIR_SCHEMA)


class FixtureModel:
    def __init__(self, request): self.calls=0
    def verify(self): return {"name":"fixture:local","digest":"synthetic-only","localOnly":True}
    def generate(self,prompt,schema,budget=0):
        self.calls+=1
        data=json.loads(prompt.rsplit("\n",1)[-1])
        if set(schema["properties"])==set(READ_SCHEMA["properties"]):
            passage=data[0];text=passage["text"]
            evidence=[passage["id"]]
            names=("Depósito bancario","Interés") if "bancario" in text else ("Muestreo","Sesgo")
            return {"claims":[{"kind":"method","text":"El documento describe "+text[:120],"evidence":evidence}],
                    "concepts":[{"name":name,"definition":text[:160],"evidence":evidence} for name in names],
                    "relations":[] if "bancario" in text else [{"from":names[0],"to":names[1],"type":"limits","explanation":"El método reduce el sesgo de selección.","evidence":evidence}]}
        if set(schema["properties"])==set(SUMMARY_SCHEMA["properties"]):
            return {"claims":[{"kind":c["kind"],"text":c["text"],"supports":[c["id"]]} for c in data]}
        if set(schema["properties"])==set(PAIR_SCHEMA["properties"]):
            concepts=data["concepts"]
            if any("bancario" in c["definition"] for c in concepts): return {"decision":"unrelated","reason":"Contextos diferentes.","relations":[]}
            left=next(c for c in concepts if c["source"]==data["left"])
            right=next(c for c in concepts if c["source"]==data["right"])
            return {"decision":"related","reason":"Una fuente aplica el método de la otra.","relations":[{"from":left["id"],"to":right["id"],"type":"complements",
                    "explanation":"Una fuente define el muestreo y la otra explica su aplicación para reducir el sesgo.",
                    "evidence":[c["evidence"][0] for c in (left,right)]}]}
        raise AssertionError("Unexpected model request")


class GroundedNotes(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.root=Path(self.temp.name)
        self.source=self.root/"Fuentes";self.source.mkdir();self.output=self.root/"Bóveda";self.output.mkdir()
        (self.output/".obsidian").mkdir();(self.output/".obsidian/app.json").write_text('{"keep":true}')
        self.keep=self.output/"Mi nota.md";self.keep.write_text("USER NOTE")
    def tearDown(self): self.temp.cleanup()
    def note(self,name,text):
        file=self.source/name;file.write_text(text,encoding="utf-8");return file
    def prepare(self):
        self.base=self.root/"plan.json";self.target=self.root/"research.json"
        self.base_result=analyze({"paths":[str(self.source)],"planFile":str(self.base)})
        return {"planFile":str(self.base),"planDigest":self.base_result["planDigest"],"analysisFile":str(self.target),"model":"fixture:local"}
    def test_coverage_locators_and_last_character_are_preserved(self):
        text=("Texto de método y contexto.\n"*1000)+"ÚLTIMO CARÁCTER"
        item={"id":"doc","format":".md","blocks":[{"locator":"texto","text":text}]}
        values=passages(item)
        self.assertEqual("".join(v["text"] for v in values),text)
        self.assertTrue(values[-1]["text"].endswith("ÚLTIMO CARÁCTER"))
        self.assertTrue(all(v["locator"].startswith("líneas") for v in values))
        self.assertEqual(len({v["id"] for v in values}),len(values))
    def test_citation_schema_does_not_restrict_the_human_summary(self):
        schema=constrained(READ_SCHEMA,evidence=["e1","e2"])
        valid={"claims":[{"kind":"method","text":"El muestreo reduce el sesgo de selección.","evidence":["e1"]}],"concepts":[],"relations":[]}
        self.assertTrue(conforms(valid,schema))
        valid["claims"][0]["evidence"]=["invented"]
        self.assertFalse(conforms(valid,schema))
        self.assertNotIn("enum",READ_SCHEMA["properties"]["claims"]["items"]["properties"]["text"])
    def test_quotes_must_exist_in_the_named_passage(self):
        lookup={"p1":{"id":"p1","document":"doc","locator":"p. 7","text":"Se registraron 14 fragmentos.\nNo hubo datación."}}
        self.assertTrue(verified_evidence([{"passage":"p1","quote":"No hubo datación."}],lookup))
        for quote,p in (("Se registraron 140 fragmentos.","p1"),("No hubo datación.","p2"),("no hubo datación.","p1")):
            self.assertEqual(verified_evidence([{"passage":p,"quote":quote}],lookup),[])
    def test_every_section_reaches_the_model_including_the_document_end(self):
        text=("Muestreo contextual limita el sesgo de selección.\n"*310)+"MARCADOR FINAL DE LA OBRA"
        self.note("Largo.md",text);request=self.prepare();received=[]
        class Trace(FixtureModel):
            def generate(self,prompt,schema,budget=0):
                if set(schema["properties"])==set(READ_SCHEMA["properties"]):received.extend(p["text"] for p in json.loads(prompt.rsplit("\n",1)[-1]))
                return super().generate(prompt,schema,budget)
        result=enrich(request,client_factory=Trace)
        self.assertEqual("".join(received),text)
        self.assertEqual(result["analysis"]["statistics"]["sourceCharactersRead"],len(text))
    def test_long_synthesis_keeps_progress_alive_between_model_calls(self):
        class Reduce:
            def generate(self,prompt,schema,budget=0):
                data=json.loads(prompt.rsplit("\n",1)[-1])
                return {"claims":[{"kind":"method","text":"Síntesis de las secciones recibidas.","supports":[c["id"] for c in data]}]}
        claims=[{"id":f"a{i}","kind":"method","text":"Texto extenso de metodología. "*180,"evidence":[{"id":f"e{i}"}]} for i in range(4)]
        pulses=[]
        result=synthesize(Reduce(),claims,{"rejectedProposals":0},lambda *values:pulses.append(values))
        self.assertGreater(len(pulses),1)
        self.assertEqual({e["id"] for c in result for e in c["evidence"]},{f"e{i}" for i in range(4)})
    def test_cross_document_relation_requires_both_sources(self):
        source={"id":"a"};target={"id":"b"}
        lookup={"p":{"id":"p","document":"doc","locator":"texto","text":"Muestreo contextual verificable."}}
        relation={"type":"supports","explanation":"Explica la selección.","evidence":[{"passage":"p","quote":"Muestreo contextual verificable."}]}
        self.assertIsNone(validate_relation(relation,lookup,source,target,cross=True))
    def test_fiches_concepts_connections_and_every_link_resolves(self):
        self.note("Método.md","El muestreo aleatorio reduce el sesgo y permite evaluar un depósito arqueológico.")
        self.note("Aplicación.md","El muestreo estratificado se aplica al depósito arqueológico para reducir el sesgo.")
        self.note("Banco.md","Un depósito bancario produce interés financiero y requiere garantías de liquidez.")
        before={p:digest(p) for p in self.source.iterdir()};request=self.prepare();base_bytes=self.base.read_bytes()
        result=enrich(request,client_factory=FixtureModel)
        self.assertEqual(self.base.read_bytes(),base_bytes)
        analysis=result["analysis"];self.assertEqual(len(analysis["documents"]),3)
        self.assertEqual(analysis["statistics"]["sourceCharactersRead"],sum(len(p.read_text(encoding="utf-8")) for p in self.source.iterdir()))
        cross=[r for r in analysis["relations"] if r["scope"]=="cross-document"]
        self.assertEqual(len(cross),1)
        self.assertEqual(len({e["document"] for e in cross[0]["evidence"]}),2)
        self.assertTrue(all(r["reviewStatus"]=="proposed" for r in analysis["relations"]))
        all_concepts=[c for d in analysis["documents"] for c in d["concepts"]]
        muestreo=[c for c in all_concepts if c["name"]=="Muestreo"]
        self.assertEqual(len(muestreo),2);self.assertNotEqual(muestreo[0]["id"],muestreo[1]["id"])
        value=export({"planFile":str(self.target),"planDigest":result["planDigest"],"destination":str(self.output)})
        folder=Path(value["folder"]);manifest=json.loads((folder/"archaeohub-library.json").read_text(encoding="utf-8"))
        self.assertEqual(len(list((folder/"Fichas").glob("*.md"))),3)
        self.assertEqual(len(list((folder/"Conexiones").glob("*.md"))),len(analysis["relations"]))
        self.assertEqual(list((folder/"Temas").iterdir()),[])
        for path in folder.rglob("*.md"):
            text=path.read_text(encoding="utf-8")
            for target,anchor in re.findall(r"\[\[([^\]|#]+)(?:#\^([^\]|]+))?(?:\|[^\]]*)?\]\]",text):
                target_path=self.output/(target+".md");self.assertTrue(target_path.is_file(),target)
                if anchor: self.assertIn("^"+anchor,target_path.read_text(encoding="utf-8"))
        self.assertEqual(before,{p:digest(p) for p in self.source.iterdir()})
        self.assertEqual(self.keep.read_text(),"USER NOTE")
        self.assertEqual((self.output/".obsidian/app.json").read_text(),'{"keep":true}')
        self.assertTrue(all(digest(folder/path)==value for path,value in manifest["files"].items()))
    def test_failure_and_cancel_do_not_replace_the_base_or_publish(self):
        self.note("Uno.md","Muestreo arqueológico y documentación de sesgos.");request=self.prepare();before=self.base.read_bytes()
        class Failed(FixtureModel):
            def generate(self,*args): raise ValueError("Fixture failure")
        with self.assertRaisesRegex(ValueError,"Fixture failure"):enrich(request,client_factory=Failed)
        self.assertEqual(self.base.read_bytes(),before);self.assertFalse(self.target.exists())
        flag=self.root/"cancel";request["cancelFile"]=str(flag)
        def cancel(**values): flag.write_text("cancel")
        with self.assertRaises(InterruptedError):enrich(request,cancel,FixtureModel)
        self.assertFalse(self.target.exists());self.assertEqual(self.base.read_bytes(),before)
    def test_changed_original_refuses_research(self):
        source=self.note("Uno.md","Muestreo y contexto arqueológico.");request=self.prepare();source.write_text("CHANGED")
        with self.assertRaisesRegex(ValueError,"Cambió"):enrich(request,client_factory=FixtureModel)
        self.assertFalse(self.target.exists())
    def test_bad_summary_citations_do_not_publish(self):
        self.note("Uno.md","Muestreo y contexto arqueológico.");request=self.prepare()
        class Bad(FixtureModel):
            def generate(self,prompt,schema,budget=0):
                if set(schema["properties"])==set(SUMMARY_SCHEMA["properties"]):return {"claims":[{"kind":"finding","text":"Inventado","supports":["nonexistent"]}]}
                return super().generate(prompt,schema,budget)
        with self.assertRaisesRegex(ValueError,"referencias válidas"):enrich(request,client_factory=Bad)
        self.assertFalse(self.target.exists())
    def test_model_text_does_not_create_arbitrary_wikilinks(self):
        from document_notes import plain
        self.assertNotIn("[[",plain("Intento [[Nota inventada]] y <script>"))
    def test_truncated_model_output_retries_then_fails(self):
        client=LocalModel({"model":"fixture:local"})
        valid={"decision":"unrelated","reason":"No hay relación.","relations":[]}
        with patch.object(client,"http",side_effect=[{"done_reason":"length","response":"{}"},{"done_reason":"stop","response":json.dumps(valid)}]) as call:
            self.assertEqual(client.generate("fixture",PAIR_SCHEMA),valid)
            self.assertEqual(call.call_args_list[1].args[2]["options"]["num_predict"],7200)
        with patch.object(client,"http",return_value={"done_reason":"length"}):
            with self.assertRaisesRegex(ValueError,"reintentar"):client.generate("fixture",PAIR_SCHEMA)
    def test_remote_model_is_rejected_without_generation(self):
        client=LocalModel({"model":"fixture:local"})
        with patch.object(client,"http",side_effect=[{"models":[{"name":"fixture:local"}]},{"remote_model":"other","remote_host":"example.invalid"}]):
            with self.assertRaisesRegex(ValueError,"pesos locales"):client.verify()
    def test_missing_model_not_downloaded(self):
        client=LocalModel({"model":"fixture:local"})
        with patch.object(client,"http",return_value={"models":[]}) as call:
            with self.assertRaisesRegex(ValueError,"No está instalado"):client.verify()
            self.assertEqual(call.call_count,1)
    def test_no_relationship_is_not_exported_as_contrast(self):
        lookup={"p":{"id":"p","document":"d","locator":"texto","text":"Un depósito bancario guarda dinero en una cuenta."}}
        relation={"type":"contrasts","explanation":"No hay conexión sustantiva entre ambos conceptos.","evidence":["p"]}
        self.assertIsNone(validate_relation(relation,lookup,{"id":"a"},{"id":"b"}))


if __name__=="__main__":unittest.main()
