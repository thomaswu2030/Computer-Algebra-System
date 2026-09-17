# Token and token types are here
from enum import Enum, auto

class TokenType(Enum):
    # operators
    COMMA = auto()
    LEFT_PAREN = auto()
    RIGHT_PAREN = auto()
    PLUS = auto()
    MINUS = auto()
    MULTIPLY = auto()
    DIVIDE = auto()
    GREATER = auto()
    SMALLER = auto()
    GREATER_EQUAL = auto()
    SMALLER_EQUAL = auto()
    EQUAL = auto()
    NOT_EQUAL = auto()
    POWER = auto()

    # identifier
    IDENTIFIER = auto()
    E = auto()
    PI = auto()
    POSITIVE_INFINITY = auto()
    NEGATIVE_INFINITY = auto()
    LIMIT = auto()
    SQRT = auto()
    DIFFERENTIATE = auto()
    INTEGRATE = auto()
    SIN = auto()
    COS = auto()
    TAN = auto()
    LOG = auto()
    MODULE = auto()

    NUMBER = auto()

    NEWLINE = auto()

class Token:
    def __init__(self, type, lexeme, literal, line):
        self.type = type
        self.lexeme = lexeme
        self.literal = literal
        self.line = line