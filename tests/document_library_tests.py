from pathlib import Path
import hashlib
import json
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"modules/documents"))
from document_library import analyze, export, shared_topics, safe_name, digest
from document_worker_tests import create_pdf,create_docx


class LibraryContracts(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.root=Path(self.temp.name)
        self.source=self.root/"Origen";self.source.mkdir();self.destination=self.root/"Destino";self.destination.mkdir()
        self.plan=self.root/"plan.json"
    def tearDown(self):self.temp.cleanup()
    def note(self,name,text):
        path=self.source/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_text(text,encoding="utf-8");return path
    def analyze(self):return analyze({"paths":[str(self.source)],"planFile":str(self.plan)})
    def export(self,result,**extra):return export({"planFile":str(self.plan),"planDigest":result["planDigest"],"destination":str(self.destination),**extra})
    def test_pdf_markdown_graph_and_preservation(self):
        a=self.note("Cerámica.md","---\ntitle: Registro\n---\n# Cerámica\nCerámica estratigrafía contextual. [[Otro]]")
        b=self.note("Otro.md","# Estudio\nCerámica estratigrafía comparada.")
        pdf=self.source/"Reporte.pdf";create_pdf(pdf,"Ceramica estratigrafia, operation 2, pit 7.")
        before={p:digest(p) for p in self.source.iterdir()}
        result=self.analyze();self.assertEqual(result["ready"],3);self.assertGreater(len(result["topics"]),0)
        value=self.export(result);folder=Path(value["folder"]);manifest=json.loads((folder/"archaeohub-library.json").read_text(encoding="utf-8"))
        self.assertEqual(value["documents"],3);self.assertTrue((folder/"Índice.md").is_file())
        notes=list((folder/"Documentos").glob("*.md"));self.assertEqual(len(notes),3)
        original=next(p for p in notes if p.name.startswith("Cerámica--")).read_text(encoding="utf-8")
        self.assertTrue(original.startswith("---\ntitle: Registro"));self.assertIn("[[Documentos/Otro--",original);self.assertIn("[[Temas/ceramica--",original)
        self.assertTrue(all(digest(folder/path)==sha for path,sha in manifest["files"].items()))
        self.assertEqual(before,{p:digest(p) for p in self.source.iterdir()});self.assertFalse((folder/".obsidian").exists())
    def test_scan_failure_does_not_stop_valid_files(self):
        self.note("Bueno.md","Cerámica registrada.");pdf=self.source/"Escaneado.pdf";create_pdf(pdf,"")
        result=self.analyze();self.assertEqual(result["ready"],1);self.assertEqual(result["failed"],1)
        self.assertEqual(next(r for r in result["rows"] if r["name"]=="Escaneado.pdf")["status"],"needs-ocr")
        output=self.export(result);self.assertEqual(output["pending"],1)
    def test_source_changed_or_plan_tampered_refuses_publication(self):
        path=self.note("Dato.md","Cerámica de contexto.");result=self.analyze();path.write_text("Cambió",encoding="utf-8")
        with self.assertRaisesRegex(ValueError,"Cambió"):self.export(result)
        self.assertEqual(list(self.destination.iterdir()),[])
        self.plan.write_bytes(self.plan.read_bytes()+b" ")
        with self.assertRaisesRegex(ValueError,"plan cambió"):self.export(result)
    def test_export_never_replaces_existing_vault(self):
        self.note("Dato.md","Cerámica de contexto.");result=self.analyze()
        settings=self.destination/".obsidian";settings.mkdir();(settings/"app.json").write_bytes(b'{"user":true}')
        existing=self.destination/"Mi nota.md";existing.write_bytes(b"USER NOTE")
        one=self.export(result);two=self.export(result)
        self.assertNotEqual(one["folder"],two["folder"]);self.assertEqual(existing.read_bytes(),b"USER NOTE");self.assertEqual((settings/"app.json").read_bytes(),b'{"user":true}')
    def test_cancel_during_write_removes_only_private_staging(self):
        self.note("Uno.md","Cerámica de contexto.");self.note("Dos.md","Cerámica de contexto.");result=self.analyze()
        keep=self.destination/"Conservar.txt";keep.write_bytes(b"USER FILE");cancel=self.root/"cancel"
        def progress(**values):cancel.write_text("cancel")
        with self.assertRaises(InterruptedError):
            export({"planFile":str(self.plan),"planDigest":result["planDigest"],"destination":str(self.destination),"cancelFile":str(cancel)},progress)
        self.assertEqual(list(self.destination.iterdir()),[keep]);self.assertEqual(keep.read_bytes(),b"USER FILE")
    def test_output_inside_source_rejected(self):
        self.note("Dato.md","Cerámica de contexto.");result=self.analyze()
        with self.assertRaisesRegex(ValueError,"fuera"):
            export({"planFile":str(self.plan),"planDigest":result["planDigest"],"destination":str(self.source)})
    def test_ambiguous_links_and_code_preserved(self):
        self.note("A/Igual.md","Cerámica de contexto.");self.note("B/Igual.md","Cerámica de otro contexto.")
        self.note("Notas.md","[[Igual]] y `[[Igual]]`\n```md\n[[Igual]]\n```")
        result=self.analyze();folder=Path(self.export(result)["folder"])
        content=next((folder/"Documentos").glob("Notas--*.md")).read_text(encoding="utf-8")
        self.assertIn("`[[Igual]]`",content);self.assertIn("```md\n[[Igual]]\n```",content);self.assertIn("sin resolver",content)
    def test_relative_markdown_resource_rebased_without_copy(self):
        image=self.source/"imagen.txt";image.write_bytes(b"RESOURCE")
        self.note("Uno.md","[Documento](Otro%20documento.md) y [Recurso](imagen.txt)\nCerámica compartida.")
        self.note("Otro documento.md","Cerámica compartida.");result=self.analyze();folder=Path(self.export(result)["folder"])
        content=next((folder/"Documentos").glob("Uno--*.md")).read_text(encoding="utf-8")
        self.assertIn("Otro%20documento--",content);self.assertIn(image.as_uri(),content);self.assertFalse(list(folder.rglob("imagen.txt")))
    def test_names_are_portable_and_collisions_do_not_merge_documents(self):
        self.assertNotIn("/",safe_name("A/B: C|D"));self.assertNotEqual(safe_name("CON"),"CON")
        self.note("A/Igual.md","Cerámica idéntica.");self.note("B/Igual.md","Cerámica idéntica.")
        result=self.analyze();folder=Path(self.export(result)["folder"]);self.assertEqual(len(list((folder/"Documentos").glob("Igual--*.md"))),2)
    def test_batch_larger_than_summary_limit_and_optional_links(self):
        for i in range(30):self.note(f"Documento{i}.md","Cerámica estratigrafía compartida.")
        result=self.analyze();self.assertEqual(result["ready"],30)
        folder=Path(self.export(result,automaticLinks=False)["folder"]);self.assertEqual(list((folder/"Temas").iterdir()),[])
        self.assertEqual(len(list((folder/"Documentos").iterdir())),30)
    def test_existing_vault_links_include_batch_path(self):
        self.note("Uno.md","Cerámica estratigrafía. [[Dos]]");self.note("Dos.md","Cerámica estratigrafía.")
        (self.destination/".obsidian").mkdir();result=self.analyze();folder=Path(self.export(result)["folder"])
        manifest=json.loads((folder/"archaeohub-library.json").read_text(encoding="utf-8"))
        self.assertEqual(manifest["linkRoot"],str(self.destination));self.assertTrue(manifest["wikiPrefix"].startswith(folder.name+"/"))
        for path in folder.rglob("*.md"):
            import re
            for target in re.findall(r"\[\[([^\]|#]+)(?:[^\]]*)\]\]",path.read_text(encoding="utf-8")):
                self.assertTrue((self.destination/(target+".md")).is_file(),target)
    def test_linked_resources_are_not_probed_and_code_links_untouched(self):
        from unittest.mock import patch
        from document_library import rewrite_links
        items=[{"path":str(self.source/"Uno.md"),"note":"Documentos/Uno","title":"Uno"}]
        text='[Remoto](//example.invalid/share) y [Local](imagen.png) y `[[Uno]]`'
        with patch.object(Path,"is_file",side_effect=AssertionError("Do not read links")),patch.object(Path,"resolve",side_effect=AssertionError("Do not resolve linked files")):
            converted,warnings=rewrite_links(text,items[0],items)
        self.assertIn("//example.invalid/share",converted);self.assertIn("`[[Uno]]`",converted);self.assertIn((self.source/"imagen.png").as_uri(),converted)
    def test_conversion_accepts_long_text_without_expanding_summary_limits(self):
        self.note("Largo.md","Cerámica estratigrafía. "*16000+"MARCADOR FINAL")
        result=self.analyze();self.assertEqual(result["ready"],1)
        folder=Path(self.export(result)["folder"])
        self.assertIn("MARCADOR FINAL",next((folder/"Documentos").iterdir()).read_text(encoding="utf-8"))
    def test_conversion_section_filters_pdf_word_without_ai_or_topics(self):
        self.note("Notas.md","USER NOTE: not part of the PDF/Word conversion")
        create_pdf(self.source/"Campo.pdf","Fourteen fragments were counted.")
        create_docx(self.source/"Informe.docx")
        before={p:digest(p) for p in self.source.iterdir()}
        result=analyze({"paths":[str(self.source)],"planFile":str(self.plan),"formats":[".pdf",".docx"],"topicCandidates":False,"conversionOnly":True})
        self.assertEqual(result["ready"],2);self.assertEqual(result["topics"],[])
        folder=Path(self.export(result,automaticLinks=False)["folder"])
        self.assertEqual(len(list((folder/"Documentos").glob("*.md"))),2)
        self.assertFalse((folder/"Fichas").exists());self.assertEqual(list((folder/"Temas").iterdir()),[])
        self.assertEqual(before,{p:digest(p) for p in self.source.iterdir()})
    def test_invalid_format_filter_is_rejected(self):
        self.note("Dato.md","Contexto arqueológico.")
        with self.assertRaisesRegex(ValueError,"formatos"):
            analyze({"paths":[str(self.source)],"planFile":str(self.plan),"formats":[".exe"]})


if __name__=="__main__":unittest.main()
