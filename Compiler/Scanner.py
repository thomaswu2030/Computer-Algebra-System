# This is a scanner that scans the input as tokens. Tokentypes are in the Structures folder
# The structure is learned and adapted from "Crafting Interpreters" by Robert Nystrom

from  CAS.Structures.token_types import TokenType, Token

class Scanner:
    def __init__(self, source):
        self.source = source
        self.tokens = []
        self.start = 0
        self.current = 0
        self.line = 1

        # Keywords
        self.keywords = {"sin":TokenType.SIN,
                "cos":TokenType.COS,
                "tan":TokenType.TAN,
                "sqrt":TokenType.SQRT,
                "differentiate":TokenType.DIFFERENTIATE,
                "lim":TokenType.LIMIT,
                "pi":TokenType.PI,
                "integrate":TokenType.INTEGRATE,
                "e":TokenType.E,
                "log":TokenType.LOG,
                "mod":TokenType.MODULE
                }

    # Helper functions
    def is_at_end(self):
        return self.current >= len(self.source)

    def peek(self):
        if (self.is_at_end()):
            return '\0'
        return self.source[self.current]

    def advance(self):
        if self.is_at_end():
            return "\0"

        char = self.source[self.current]
        self.current += 1
        return char

    def match(self, expected):
        if self.is_at_end() or self.peek() != expected:
            return False

        self.advance()
        return True

    def add_token(self, token_type, literal=None):
        lexeme = self.source[self.start:self.current]
        token = Token(token_type, lexeme, literal, self.line)
        self.tokens.append(token)

    # scan tokens by matching characters
    def scan_token(self):
        c = self.advance()

        match c:
            case "(":
                self.add_token(TokenType.LEFT_PAREN)
            case ")":
                self.add_token(TokenType.RIGHT_PAREN)
            case "=":
                self.add_token(TokenType.EQUAL)
            case "+":
                self.add_token(TokenType.PLUS)
            case "-":
                self.add_token(TokenType.MINUS)
            case "*":
                self.add_token(TokenType.MULTIPLY)
            case "/":
                self.add_token(TokenType.DIVIDE)
            case "^":
                self.add_token(TokenType.POWER)
            case ",":
                self.add_token(TokenType.COMMA)
            case "\n":
                self.add_token(TokenType.NEWLINE)
                self.line += 1
                     
            case ">":
                if self.match("="):
                    self.add_token(TokenType.GREATER_EQUAL)
                else:
                    self.add_token(TokenType.GREATER)
            case "<":
                if self.match("="):
                    self.add_token(TokenType.SMALLER_EQUAL)
                else:
                    self.add_token(TokenType.SMALLER)

            case "!":
                if self.match("="):
                    self.add_token(TokenType.NOT_EQUAL)
                else:
                    raise ValueError("Syntax error at line " + str(self.line))

            # possible keywords
            case _ if c.isalpha():
                while self.peek().isalnum() or self.peek() == "_":
                    self.advance()

                text = self.source[self.start:self.current]
                token_type = self.keywords.get(text, TokenType.IDENTIFIER)
                self.add_token(token_type)

            # numbers
            case _ if c.isnumeric():
                while self.peek().isnumeric():
                    self.advance()

                if self.peek() == ".":
                    self.advance()

                    while self.peek().isnumeric():
                        self.advance()

                text = self.source[self.start:self.current]
                value = float(text)
                self.add_token(TokenType.NUMBER, value)
                
            case " " | "\r" | "\t":
                pass
            case _:
                raise ValueError("Syntax error at line " + str(self.line))


    def scan_tokens(self):
        while (not self.is_at_end()):
            self.start = self.current
            self.scan_token()

        return self.tokens