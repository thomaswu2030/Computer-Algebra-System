#Source -> Python AST -> native numerical/symbolic engine.
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parent
if __package__ in (None,''):
    sys.path.insert(0,str(ROOT))
from Compiler.Scanner import Scanner
from Compiler.Parser import Parser
from Tools.serialization.encoder import to_data

def parse_expression(source):
    # scan text into tokens, parse their precedence, then serialize the tree.
    parser=Parser(Scanner(source).scan_tokens())
    tree=parser.parse()
    if tree is None: raise ValueError(parser.error or 'Expected expression')
    return to_data(tree)

def run_engine(ast,mode='simplify',variable='x',values=None):
    executable=ROOT/'build/cas_engine.exe'
    if not executable.exists(): raise RuntimeError('Build first: .venv\\Scripts\\python.exe build.py')
    request={'ast':ast,'mode':mode,'variable':variable}
    # Omit values when no assignments were requested. In differentiation mode,
    # the presence of this field also asks the engine to evaluate the derivative.
    if values is not None: request['values']=values
    environment=os.environ.copy()
    compiler=Path('C:/msys64/ucrt64/bin')
    if compiler.exists():
        environment['PATH']=str(compiler)+os.pathsep+environment.get('PATH','')
    # send one JSON request through stdin and capture the JSON response from stdout.
    # reject NaN/infinity during encoding and limit native execution to 30 seconds.
    result=subprocess.run([str(executable)],input=json.dumps(request,allow_nan=False),
                          capture_output=True,text=True,encoding='utf-8',env=environment,timeout=30)

    if not result.stdout: raise RuntimeError(result.stderr or f'Engine exited with {result.returncode}')
    output=json.loads(result.stdout)
    if 'error' in output: raise ValueError(output['error'])
    if result.returncode: raise RuntimeError(result.stderr)
    return output

def main():
    parser=argparse.ArgumentParser(description='Numeric evaluation, symbolic simplification, and differentiation')
    # the expression is optional, prompt below when it was not supplied on the command line.
    parser.add_argument('expression',nargs='?')
    parser.add_argument('--mode',choices=['encode','evaluate','simplify','differentiate','autodiff'],default='simplify')
    parser.add_argument('--variable',default='x')
    # repeat --set for multiple assignments, for example --set x=2 --set y=3.
    parser.add_argument('--set',action='append',default=[],metavar='NAME=VALUE')
    parser.add_argument('--json',action='store_true',help='Show result AST and unsimplified derivative')
    args=parser.parse_args()
    try:
        if args.expression is not None:
            source = args.expression
        else:
            source = input('Expression: ')
        ast=parse_expression(source)
        # save the encoded tree for inspection; native execution uses the in-memory AST.
        path=ROOT/'Tools/serialization/expression.json'
        path.write_text(json.dumps(ast,indent=2,allow_nan=False),encoding='utf-8')
        if args.mode=='encode':
            print(path)
            return 0
        # convert NAME=VALUE arguments into the numeric environment expected by C++.
        values={}
        for entry in args.set:
            name,value=entry.split('=',1)
            values[name]=float(value)
        # none means no assignments were requested, preserve symbolic variables.
        assignments = None
        if args.set:
            assignments = values
        result = run_engine(ast, args.mode, args.variable, assignments)
        # full JSON includes result metadata; normal output shows the available text/numbers.
        if args.json: print(json.dumps(result,indent=2,allow_nan=False))
        else:
            if 'text' in result: print(result['text'])
            if 'value' in result: print('Value:',result['value'])
            if 'derivative' in result: print('Derivative:',result['derivative'])
        return 0
    # report expected input/process failures without a traceback, return a failing exit status.
    except (ValueError,RuntimeError,OSError,EOFError,RecursionError,subprocess.TimeoutExpired) as error:
        print('Error:',error,file=sys.stderr)
        return 1

if __name__=='__main__': raise SystemExit(main())
