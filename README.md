This is a Computer Algebra System that support numeric evaluation, symbolic simplication, and more.

How to run the program: 
In powershell, create a venv
then simply do:
.\.venv\Scripts\python.exe build.py

Some cool examples to try:
.\.venv\Scripts\python.exe main.py 'x+2*3' --mode simplify
.\.venv\Scripts\python.exe main.py 'x*x+sin(x)' --mode differentiate --json
.\.venv\Scripts\python.exe main.py 'x*x+sin(x)' --mode autodiff --set x=2
.\.venv\Scripts\python.exe main.py 'x*y' --mode differentiate --variable y


Modes:

Choose an operation with `--mode`. If omitted, the default is `simplify`.
Run the examples below from the `CAS` directory.

| Mode | What it does |
| --- | --- |
| `simplify` | Reduces constant expressions and applies algebraic rules, keeping unresolved variables symbolic. Optional `--set` assignments substitute known values. |
| `evaluate` | Computes a numerical value. Supply `--set` assignments for every variable in the expression. |
| `differentiate` | Builds and simplifies a symbolic derivative. With assignments, also evaluates the derivative at that point. |
| `autodiff` | Computes the expression's value and a numerical derivative at the supplied point, without building a symbolic derivative expression. |
| `encode` | Saves the parsed expression tree to `Tools/serialization/expression.json` and prints its path. Does not run the C++ engine. |

```powershell
# Simplify: (x + 6)
.\.venv\Scripts\python.exe main.py 'x+2*3' --mode simplify

# Evaluate: Value: 10
.\.venv\Scripts\python.exe main.py 'x^2+1' --mode evaluate --set x=3

# Symbolic derivative: (2 * x)
.\.venv\Scripts\python.exe main.py 'x^2' --mode differentiate

# Automatic differentiation: Value: 9, Derivative: 6
.\.venv\Scripts\python.exe main.py 'x^2' --mode autodiff --set x=3

# Export the parsed tree
.\.venv\Scripts\python.exe main.py 'x+1' --mode encode
```

Both derivative modes use `x` by default; use `--variable y` to differentiate
with respect to `y`. Repeat `--set` for multiple variables, such as
`--set x=2 --set y=3`. Add `--json` to inspect the full engine response,
including the result tree and raw derivative when available.


Expression conventions:

- Logarithms use `log(value, base)`: `log(8,2)` is `3`. For natural logarithms, use `log(x,e)`.
- `sin(x)`, `cos(x)`, and `tan(x)` take angles in radians. `pi` and `e` are built-in constants.
- Write multiplication explicitly: `2*x` or `2*(x+1)`.
- Use `^` for powers. `2^3^2` means `2^(3^2)`, and `-2^2` means `-(2^2)`. Use `(-2)^2` to square a negative base.
- Functions require parentheses; separate arguments with commas: `sqrt(9)`, `mod(8,3)`.
- `mod(a,b)` returns the floating-point remainder with the sign of `a`: `mod(-8,3)` is `-2`.
- Comparisons use `=`, `!=`, `<`, `<=`, `>`, and `>=`, returning `1` for true or `0` for false. `=` compares values; assign variables with `--set x=2` instead.
- Differentiation uses `--mode differentiate` or `--mode autodiff`. The default variable is `x`; choose another with `--variable y`.
