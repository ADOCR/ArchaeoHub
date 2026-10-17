"""Pruebas de regresión del límite de publicación; todas las entradas son sintéticas."""
from pathlib import Path
import sys,tempfile,unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from check_publication import findings

class PublicationAuditTests(unittest.TestCase):
    def examine(self,relative,content):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);path=root/relative;path.parent.mkdir(parents=True,exist_ok=True);path.write_text(content,encoding='utf-8')
            return findings(root,relative)
    def test_source_and_synthetic_system_paths(self):
        self.assertEqual(self.examine('core/example.cpp','// System fallback: C:/Windows/System32\n'),[])
        self.assertEqual(self.examine('tools/build_example.py','# build helper\n'),[])
    def test_personal_paths_are_rejected_without_echoing_values(self):
        value='Z:'+ '/Users/'+ 'synthetic-person/Documents/file'
        self.assertIn('private-user-path',self.examine('core/example.cpp',value))
    def test_data_and_unapproved_models_are_rejected(self):
        self.assertIn('data-binary-or-credential-file',self.examine('resources/field.csv','synthetic'))
        self.assertIn('unapproved-model',self.examine('modules/transcription/new.onnx','synthetic'))
    def test_credential_patterns(self):
        value='ghp_'+('A'*36)
        result=self.examine('config/example.json',value)
        self.assertIn('github-token',result);self.assertNotIn(value,str(result))
        self.assertIn('private-key',self.examine('config/example.json','-----BEGIN '+'PRIVATE KEY-----'))
    def test_private_and_unexpected_paths(self):
        self.assertIn('private-or-generated-path',self.examine('docs/audit/report.md','synthetic'))
        self.assertIn('unexpected-path',self.examine('unreviewed/report.md','synthetic'))
        self.assertIn('unreviewed-image',self.examine('docs/private-photo.png','synthetic'))
if __name__=='__main__':unittest.main()
