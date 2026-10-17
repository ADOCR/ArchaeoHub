from pathlib import Path
import hashlib
import sys
import tempfile
import unittest
import zipfile
from pypdf import PdfWriter
from pypdf.generic import NameObject, DictionaryObject, DecodedStreamObject
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"modules/documents"))
from document_worker import extract, dispatch

def create_pdf(path,text="Operation 2, pit 7: 14 ceramic fragments."):
    writer=PdfWriter();page=writer.add_blank_page(width=612,height=792)
    font=DictionaryObject({NameObject("/Type"):NameObject("/Font"),NameObject("/Subtype"):NameObject("/Type1"),NameObject("/BaseFont"):NameObject("/Helvetica")})
    page[NameObject("/Resources")]=DictionaryObject({NameObject("/Font"):DictionaryObject({NameObject("/F1"):writer._add_object(font)})})
    stream=DecodedStreamObject();stream.set_data(f"BT /F1 12 Tf 40 700 Td ({text}) Tj ET".encode());page[NameObject("/Contents")]=writer._add_object(stream)
    writer.write(path);writer.close()

def create_docx(path):
    xml='<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:r><w:t>Operación 2: pozo 7.</w:t></w:r></w:p><w:tbl><w:tr><w:tc><w:p><w:r><w:t>14 fragmentos cerámicos.</w:t></w:r></w:p></w:tc></w:tr></w:tbl></w:body></w:document>'
    with zipfile.ZipFile(path,"w") as archive:archive.writestr("word/document.xml",xml)

class DocumentReaderContracts(unittest.TestCase):
    def setUp(self):self.temp=tempfile.TemporaryDirectory();self.root=Path(self.temp.name)
    def tearDown(self):self.temp.cleanup()
    def test_three_formats_and_original_bytes(self):
        paths=[self.root/"Campo.md",self.root/"Informe.docx",self.root/"Reporte.pdf"]
        paths[0].write_text("# Operación 2\nPozo 7: 14 fragmentos. No hay datación absoluta.",encoding="utf-8");create_docx(paths[1]);create_pdf(paths[2])
        before={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
        result=dispatch({"schemaVersion":1,"paths":[str(p) for p in paths]})
        self.assertEqual(len(result["documents"]),3)
        for doc in result["documents"]:self.assertIn("14"," ".join(b["text"] for b in doc["blocks"]))
        self.assertEqual(result["documents"][2]["blocks"][0]["locator"],"p. 1")
        self.assertEqual(before,{p:hashlib.sha256(p.read_bytes()).hexdigest() for p in paths})
    def test_scanned_or_protected_pdf_explains_error(self):
        path=self.root/"Scan.pdf";writer=PdfWriter();writer.add_blank_page(width=100,height=100);writer.write(path)
        with self.assertRaisesRegex(ValueError,"OCR"):extract(path)
        writer.encrypt("password");writer.write(path);writer.close()
        with self.assertRaisesRegex(ValueError,"protegido"):extract(path)
    def test_partial_pdf_lists_unreadable_pages(self):
        path=self.root/"Mixto.pdf";create_pdf(path);from pypdf import PdfReader
        writer=PdfWriter();writer.append(PdfReader(path));writer.add_blank_page(width=100,height=100);writer.write(self.root/"Mixto2.pdf");writer.close()
        result=extract(self.root/"Mixto2.pdf");self.assertIn("Página 2",result["warnings"][0])
    def test_size_legacy_word_and_bad_input(self):
        path=self.root/"Grande.md";path.write_text("x"*2000001)
        with self.assertRaisesRegex(ValueError,"truncado"):extract(path)
        path=self.root/"Antiguo.doc";path.write_bytes(b"DOC")
        with self.assertRaisesRegex(ValueError,"docx"):extract(path)
        with self.assertRaises(ValueError):dispatch({"schemaVersion":1,"paths":[]})

if __name__=="__main__":unittest.main()
