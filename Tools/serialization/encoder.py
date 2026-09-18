# encode python structure to JSON text file.
import json
from Structures import asts

def to_data(node):
    if isinstance(node, asts.Literal):
        return {"kind": "number", 
                "value": node.value}

    if isinstance(node, asts.Binary):
        operator = node.operator.lexeme
        left_data = to_data(node.left)
        right_data = to_data(node.right)
        return {
            "kind": "binary",
            "operator": operator,
            "left": left_data,
            "right": right_data,
        }

    if isinstance(node, asts.Unary):
        operator = node.operator.lexeme
        right_data = to_data(node.right)
        return {
            "kind": "unary",
            "operator": operator,
            "right": right_data
        }

    if isinstance(node, asts.Call):
        operator = node.operator.lexeme
        arguments = [to_data(arg) for arg in node.arguments]
        return {
            "kind": "call",
            "operator": operator,
            "arguments": arguments
        }

    if isinstance(node, asts.Grouping):
        expression = to_data(node.expression)
        return {
            "kind": "grouping",
            "expression": expression
        }

    if isinstance(node, asts.Symbol):
        return {
            "kind": "symbol", 
            "name": node.name
        }

    raise TypeError(f"Unsupported AST node: {type(node).__name__}")

def to_JSON(tree, output_path):
    data = to_data(tree)
    with open(output_path,"w", encoding="utf-8") as file:
        json.dump(data, file, indent=2, allow_nan=False)
