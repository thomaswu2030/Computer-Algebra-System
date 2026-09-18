# This is a parser that parser tokens into asts. The structure is learned and adapted from a book - 
# "Crafting Interpreters" by Robert Nystrom

import math
from Structures.token_types import TokenType
import Structures.asts as asts

class ParserError(Exception):
    pass

class Parser:
    # use recursive descent parsing for precedence matching
    def __init__(self, tokens):
        current = 0  
        self.error = None
        self.tokens = tokens
        self.current = current
        self.length = len(tokens)
        self.functions = {TokenType.SIN: 1,
                TokenType.COS: 1,
                TokenType.TAN: 1,
                TokenType.SQRT: 1,
                TokenType.LIMIT: 1,
                TokenType.LOG: 2,
                TokenType.MODULE: 2,
                TokenType.DIFFERENTIATE: 1
                }

    def parse(self):
        try:
            result = self.expression()
            while self.match(TokenType.NEWLINE):
                continue

            if not self.isAtEnd():
                raise ParserError("Unexpected remaining input")
            
            return result
        except ParserError as error:
            self.error = str(error)
            self.synchronize()
            return None


    def expression(self):
        return self.equality()

    def equality(self):
        expr = self.comparison()

        while self.match(TokenType.EQUAL, TokenType.NOT_EQUAL):
            operator = self.previous()
            right = self.comparison()
            expr = asts.Binary(expr, operator, right)

        return expr

    def comparison(self):
        expr = self.term()

        while self.match(TokenType.GREATER, TokenType.GREATER_EQUAL, TokenType.SMALLER, TokenType.SMALLER_EQUAL):
            operator = self.previous()
            right = self.term()
            expr = asts.Binary(expr, operator, right)

        return expr

    # + and -
    def term(self):
        expr = self.factor()

        while self.match(TokenType.PLUS, TokenType.MINUS):
            operator = self.previous()
            right = self.factor()
            expr = asts.Binary(expr, operator, right)

        return expr

    def factor(self):
        expr = self.unary()

        while self.match(TokenType.MULTIPLY, TokenType.DIVIDE):
            operator = self.previous()
            right = self.unary()
            expr = asts.Binary(expr, operator, right)

        return expr

    def unary(self):
        if self.match(TokenType.MINUS):
            operator = self.previous()
            right = self.unary()
            expr = asts.Unary(operator,right)
            return expr

        return self.power()

    def power(self):
        # The right operand can contain another power. This one is right associate.
        expr = self.call()

        if self.match(TokenType.POWER):
            operator = self.previous()
            right = self.unary()
            expr = asts.Binary(expr, operator, right)

        return expr

    # function calls. Like sin, cos, differentiate
    def call(self):
        args = list()
        if self.match(*self.functions):
            operator = self.previous()
            self.consume(TokenType.LEFT_PAREN, "Expected '(' after a function call")             
            arg = self.expression()
            args.append(arg)
            while self.match(TokenType.COMMA):
                arg = self.expression()
                args.append(arg)
            self.consume(TokenType.RIGHT_PAREN, "Expected ')' after function arguments")

            expected_args_count = self.functions.get(operator.type)
            if not expected_args_count == len(args):
                raise ParserError(
                    f"{operator.lexeme} expects {expected_args_count} arguments, got {len(args)}")
            
            expr = asts.Call(operator,args)
            return expr

        return self.primary()

    # Literals, symbols
    def primary(self):
        if self.match(TokenType.NUMBER):
            number = self.previous()
            expr = asts.Literal(number.literal)
        elif self.match(TokenType.PI):
            expr = asts.Literal(math.pi)
        elif self.match(TokenType.E):
            expr = asts.Literal(math.e)
        elif self.match(TokenType.IDENTIFIER):
            identifier = self.previous()
            expr = asts.Symbol(identifier.lexeme)
        elif self.match(TokenType.LEFT_PAREN):
            expression = self.expression()
            self.consume(TokenType.RIGHT_PAREN, "Expected ')' after grouped expression")
            expr = asts.Grouping(expression)
        else:
            raise ParserError("Expected expression")

        return expr

    # Helper functions
    def check(self, type):
        if self.isAtEnd(): return False
        return self.peek().type == type

    def consume(self, type, message):
        if self.check(type): return self.advance()

        raise ParserError(message)

    def peek(self):
        return self.tokens[self.current]

    def previous(self):
        return self.tokens[self.current - 1]

    def isAtEnd(self):
        return self.current >= self.length

    def advance(self):
        if not self.isAtEnd(): self.current+=1
        return self.previous()

    def match(self, *types):
        for type in types:
            if self.check(type):
                self.advance()
                return True

        return False

    # Synchronization for error detection
    def synchronize(self):
        if self.isAtEnd():
            return
        self.advance()

        while not self.isAtEnd():
            if self.previous().type == TokenType.NEWLINE:
                return

            match self.advance().type:
                case TokenType.NEWLINE:
                    return
                
