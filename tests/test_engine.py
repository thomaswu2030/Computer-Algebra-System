# Integration test. The main credit for this file goes to codex
from pathlib import Path
import random
import sys
import subprocess
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[2]))
from CAS.main import parse_expression, run_engine

class EngineTests(unittest.TestCase):
    def test_command_line_preserves_power(self):
        root = Path(__file__).resolve().parents[1]
        result = subprocess.run([sys.executable, str(root/'main.py'), 'x^3+sin(x)',
                                 '--mode', 'autodiff', '--set', 'x=2'],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('Derivative: 11.583853', result.stdout)

    def run_expr(self,source,mode='evaluate',values=None,variable='x'):
        return run_engine(parse_expression(source),mode,variable,values)

    def test_numeric_and_variables(self):
        cases={'2+3*4':14,'(2+3)*4':20,'-2^2':-4,'2^3^2':512,
               '2^-3':0.125,'sin(pi/2)':1,'cos(0)':1,'tan(0)':0,
               'sqrt(9)':3,'log(8,2)':3,'mod(-8,3)':-2,'2>=2':1,'2!=2':0}
        for source,expected in cases.items():
            with self.subTest(source=source): self.assertAlmostEqual(self.run_expr(source)['value'],expected)
        self.assertEqual(self.run_expr('x*y+2',values={'x':3,'y':4})['value'],14)
        with self.assertRaisesRegex(ValueError,'symbol'): self.run_expr('x+1')

    def test_simplification_and_partial_substitution(self):
        cases={'x+2*3':'(x + 6)','x+x':'(2 * x)','(x+0)*1':'x',
               'sin(0)+x':'x','x-x':'0','-(-x)':'x','x*0':'0'}
        for source,expected in cases.items():
            with self.subTest(source=source): self.assertEqual(self.run_expr(source,'simplify')['text'],expected)
        result=self.run_expr('x+y*2','simplify',{'y':3})
        self.assertEqual(result['text'],'(x + 6)')

    def test_simplification_equivalence(self):
        rng=random.Random(17)
        for source in ['x+x','sin(x)+2*3','-(-x)','(x+0)*1','0*(1/x)','x/x','sqrt(x)-sqrt(x)']:
            ast=self.run_expr(source,'simplify')['ast']
            for _ in range(3):
                values={'x':rng.uniform(0.2,3)}
                self.assertAlmostEqual(run_engine(ast,'evaluate',values=values)['value'],self.run_expr(source,values=values)['value'])

    def test_original_domains_are_not_erased(self):
        for source,x in [('x/x',0),('0*(1/x)',0),('sqrt(x)-sqrt(x)',-1),('x^0',0)]:
            ast=self.run_expr(source,'simplify')['ast']
            with self.assertRaises(ValueError): run_engine(ast,'evaluate',values={'x':x})
        with self.assertRaises(ValueError): self.run_expr('0*(1/0)','simplify')

    def test_derivatives(self):
        # Compare symbolic differentiation, forward-mode AD, and finite differences.
        for source in ['x*x','x^3+sin(x)','1/x','cos(x)','tan(x)','sqrt(x)',
                       'log(x,2)','log(x,x+2)','x^x','2^x','sin(x*x)','(x+1)*(x-1)']:
            derivative=self.run_expr(source,'differentiate')['ast']
            for x in [0.7,1.3,2.1]:
                h=0.00001
                finite=(self.run_expr(source,values={'x':x+h})['value']-
                        self.run_expr(source,values={'x':x-h})['value'])/(2*h)
                symbolic=run_engine(derivative,'evaluate',values={'x':x})['value']
                ad=self.run_expr(source,'autodiff',{'x':x})['derivative']
                with self.subTest(source=source,x=x):
                    self.assertAlmostEqual(symbolic,finite,delta=0.00001*max(1,abs(finite)))
                    self.assertAlmostEqual(ad,symbolic,delta=0.0000001*max(1,abs(symbolic)))
        for source,expected in [('x^2',0),('x^1',1),('x*x',0)]:
            self.assertEqual(self.run_expr(source,'autodiff',{'x':0})['derivative'],expected)
            self.assertEqual(self.run_expr(source,'differentiate',{'x':0})['value'],expected)
        self.assertEqual(self.run_expr('x*y','autodiff',{'x':2,'y':3},'y')['derivative'],2)
        self.assertEqual(self.run_expr('y','differentiate')['text'],'0')

    def test_errors(self):
        for source in ['1@2','1 2','(1+2','sin()','log(1)','']:
            with self.subTest(source=source),self.assertRaises(ValueError): parse_expression(source)
        for source in ['1/0','sqrt(-1)','log(1,1)','mod(2,0)','(-2)^0.5','0^0','10^1000']:
            with self.subTest(source=source),self.assertRaises(ValueError): self.run_expr(source)
        for mode in ['differentiate','autodiff']:
            for source in ['x>2','mod(x,2)']:
                with self.subTest(source=source,mode=mode),self.assertRaises(ValueError): self.run_expr(source,mode,{'x':3})
        with self.assertRaises(ValueError): self.run_expr('sqrt(x)','autodiff',{'x':0})
        # A stationary exponent is not a constant exponent.
        with self.assertRaises(ValueError): self.run_expr('(-2)^(x*x)','autodiff',{'x':0})

if __name__=='__main__': unittest.main(verbosity=2)
