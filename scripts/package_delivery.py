"""Gera arquivo de entrega com fontes, sem ambientes, binarios, caches ou Git.
Nao faz commit, push, PR ou publicacao no GitHub.
"""
from pathlib import Path
import zipfile
ROOT=Path(__file__).resolve().parents[1]

def main():
    output=ROOT/'output/CP1_CP2_Entrega.zip';output.parent.mkdir(exist_ok=True)
    roots=['Arduino','ESP32','Dashboard','api','scripts','tests','docs','infra','local','postman','.vscode']
    blocked={'.deps','build','__pycache__','.venv','.local'}
    with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED) as archive:
        for name in ['README.md','.gitignore']:
            archive.write(ROOT/name,name)
        for name in roots:
            for path in sorted((ROOT/name).rglob('*')):
                relative=path.relative_to(ROOT)
                if path.is_file() and path.name != '.DS_Store' and not blocked.intersection(relative.parts) and path.suffix not in ('.pyc','.o','.a','.dylib'):
                    archive.write(path,relative.as_posix())
    with zipfile.ZipFile(output) as archive:
        assert archive.testzip() is None
        assert 'Arduino/Arduino.ino' in archive.namelist()
        assert not any('/build/' in name or '/.deps/' in name for name in archive.namelist())
        print(f'Entrega preparada: {output} ({len(archive.namelist())} arquivos)')

if __name__=='__main__': main()
