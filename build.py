from pathlib import Path
import shutil
import subprocess

ROOT=Path(__file__).resolve().parent

def build():
    compiler=shutil.which('g++')
    if not compiler:
        raise RuntimeError('g++ is not on PATH. Use the MSYS2 UCRT64 compiler.')
    (ROOT/'build').mkdir(exist_ok=True)
    sources=[ROOT/'Symbolic Engine'/name for name in
             ['evaluator.cpp','ast_utils.cpp','simplifier.cpp','differentiation.cpp','autodiff.cpp','runner.cpp']]
    sources.append(ROOT/'Tools/serialization/decoder.cpp')
    subprocess.run([compiler,'-std=c++20','-O2','-Wall','-Wextra','-Werror',
                    *map(str,sources),'-o',str(ROOT/'build/cas_engine.exe')],check=True)
    print('Built build/cas_engine.exe')

if __name__=='__main__': build()
