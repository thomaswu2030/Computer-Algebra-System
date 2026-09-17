#pragma once
#include "../Structures/asts.hpp"
#include <unordered_map>

// some simplification
using Node = std::unique_ptr<Expr>;
using Environment = std::unordered_map<std::string, double>;

// each function borrows its input tree. Returned nodes have independent ownership.
double evaluate(const Expr& expr, const Environment& values = {});
Node simplify(const Expr& expr, const Environment& known_values = {});
Node differentiate(const Expr& expr, const std::string& variable);
Node clone(const Expr& expr); // deep copy. recursively copy childrens
std::string format(const Expr& expr); // convert a tree into readable text
bool same_expression(const Expr& left, const Expr& right);
bool always_defined(const Expr& expr);

// small construction helpers to improve readability
Node make_number(double value);
Node make_binary(std::string operation, Node left, Node right);
Node make_negative(Node right);
Node make_call(std::string function, Node argument);
Node make_log(Node argument); // Natural logarithm represented as log(arg, e).
void validate_function(const std::string& name, std::size_t count);

// forward-mode automatic differentiation. evaluate a value and one partial derivative.
struct ValueAndDerivative { double value; double derivative; };
ValueAndDerivative autodiff(const Expr& expr, const Environment& values,
                            const std::string& variable);
