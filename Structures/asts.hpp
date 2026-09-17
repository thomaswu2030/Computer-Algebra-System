// c++ version of asts

#pragma once
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <variant>

struct Expr {
    virtual ~Expr() = default;
};

struct Binary : Expr {
    std::string operation;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
};

struct Unary : Expr {
    char operation;
    std::unique_ptr<Expr> right;
};

struct Grouping : Expr {
    std::unique_ptr<Expr> expr;
};

struct Symbol : Expr {
    std::string name;
};

struct Literal : Expr {
    std::variant<double> value;
};

struct Call : Expr {
    std::string function;
    std::vector<std::unique_ptr<Expr>> arguments;
};