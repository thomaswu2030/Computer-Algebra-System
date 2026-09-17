# Abstract syntax trees (asts). 

from abc import ABC, abstractmethod


# The abstract class is imported for future updates. Now it is unused
class Expr(ABC): 
    pass

class Binary (Expr):
    def __init__(self, left, operator, right):
        self.left = left
        self.operator = operator
        self.right = right

class Grouping (Expr):
    def __init__(self, expression):
        self.expression = expression

class Literal (Expr):
    def __init__(self, value):
        self.value = value

class Unary (Expr):
    def __init__(self, operator, right):
        self.operator = operator
        self.right = right

class Call (Expr):
    def __init__(self, operator, arguments):
        self.operator = operator
        self.arguments = arguments

class Symbol (Expr):
    def __init__(self, name):
        self.name = name
