#include "engine.hpp"
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

// Constructor helpers. Just to make code more readable
Node make_number(double value) {
    if (!std::isfinite(value)){
        throw std::runtime_error("Number must be finite");
    } 
    auto result = std::make_unique<Literal>();
    result->value = value;
    return result;
}

// Children are passed by value to transfer exclusive ownership into the new parent.
// These helpers construct syntax only
Node make_binary(std::string operation, Node left, Node right) {
    auto result = std::make_unique<Binary>();
    result->operation = std::move(operation);
    result->left = std::move(left);
    result->right = std::move(right);
    return result;
}

Node make_negative(Node right) {
    auto result = std::make_unique<Unary>();
    result->operation = '-';
    result->right = std::move(right);
    return result;
}

// Convenience helper for one argument
Node make_call(std::string function, Node argument) {
    auto result = std::make_unique<Call>();
    result->function = std::move(function);
    result->arguments.push_back(std::move(argument));
    return result;
}

// The engine uses log(value, base), so natural logarithms need an explicit base e.
Node make_log(Node argument) {
    auto result = std::make_unique<Call>();
    result->function = "log";
    result->arguments.push_back(std::move(argument));
    result->arguments.push_back(make_number(std::exp(1.0)));
    return result;
}

void validate_function(const std::string& name, std::size_t count) {
    static const std::unordered_map<std::string, std::size_t> arities{
        {"sin",1}, {"cos",1}, {"tan",1}, {"sqrt",1}, {"log",2}, {"mod",2}
    };
    const auto found = arities.find(name);
    if (found == arities.end()) throw std::runtime_error("Unsupported function: " + name);
    if (found->second != count) throw std::runtime_error("Wrong argument count for " + name);
}

// deep copy: each recursive call allocates independent children.
Node clone(const Expr& expr) {
    if (const auto* n = dynamic_cast<const Literal*>(&expr))
        return make_number(std::get<double>(n->value));
    if (const auto* s = dynamic_cast<const Symbol*>(&expr)) {
        auto result = std::make_unique<Symbol>();
        result->name = s->name;
        return result;
    }
    if (const auto* b = dynamic_cast<const Binary*>(&expr))
        return make_binary(b->operation, clone(*b->left), clone(*b->right));
    if (const auto* u = dynamic_cast<const Unary*>(&expr)) {
        if (u->operation != '-') throw std::runtime_error("Unsupported unary operator");
        return make_negative(clone(*u->right));
    }
    if (const auto* g = dynamic_cast<const Grouping*>(&expr)) {
        auto result = std::make_unique<Grouping>();
        result->expr = clone(*g->expr);
        return result;
    }
    if (const auto* c = dynamic_cast<const Call*>(&expr)) {
        auto result = std::make_unique<Call>();
        result->function = c->function;
        for (const auto& arg : c->arguments) result->arguments.push_back(clone(*arg));
        return result;
    }
    throw std::runtime_error("Unknown expression type");
}

// compare tree structure, including operand order. ex. x+y does not match y+x.
bool same_expression(const Expr& a, const Expr& b) {
    if (const auto* x = dynamic_cast<const Literal*>(&a)) {
        const auto* y = dynamic_cast<const Literal*>(&b);
        return y && x->value == y->value;
    }
    if (const auto* x = dynamic_cast<const Symbol*>(&a)) {
        const auto* y = dynamic_cast<const Symbol*>(&b);
        return y && x->name == y->name;
    }
    if (const auto* x = dynamic_cast<const Binary*>(&a)) {
        const auto* y = dynamic_cast<const Binary*>(&b);
        return y && x->operation == y->operation && same_expression(*x->left,*y->left)
                 && same_expression(*x->right,*y->right);
    }
    if (const auto* x = dynamic_cast<const Unary*>(&a)) {
        const auto* y = dynamic_cast<const Unary*>(&b);
        return y && x->operation == y->operation && same_expression(*x->right,*y->right);
    }
    if (const auto* x = dynamic_cast<const Grouping*>(&a)) {
        const auto* y = dynamic_cast<const Grouping*>(&b);
        return y && same_expression(*x->expr,*y->expr);
    }
    if (const auto* x = dynamic_cast<const Call*>(&a)) {
        const auto* y = dynamic_cast<const Call*>(&b);
        if (!y || x->function != y->function || x->arguments.size() != y->arguments.size()) return false;
        for (std::size_t i=0; i<x->arguments.size(); ++i)
            if (!same_expression(*x->arguments[i],*y->arguments[i])) return false;
        return true;
    }
    return false;
}

// A conservative domain check, not a general theorem prover.
// Assumes mathematical real arithmetic, evaluate() separately checks double overflow.
bool always_defined(const Expr& expr) {
    if (dynamic_cast<const Literal*>(&expr) || dynamic_cast<const Symbol*>(&expr)) return true;
    if (const auto* g = dynamic_cast<const Grouping*>(&expr)) return always_defined(*g->expr);
    if (const auto* u = dynamic_cast<const Unary*>(&expr)) return always_defined(*u->right);
    if (const auto* b = dynamic_cast<const Binary*>(&expr)) {
        if (b->operation == "+" || b->operation == "-" || b->operation == "*")
            return always_defined(*b->left) && always_defined(*b->right);
        if (const auto* exponent = dynamic_cast<const Literal*>(b->right.get())) {
            const double n = std::get<double>(exponent->value);
            return b->operation == "^" && n > 0 && std::trunc(n) == n && always_defined(*b->left);
        }
    }
    return false;
}

// parenthesize every binary operation to preserve the tree structure in displayed text.
std::string format(const Expr& expr) {
    if (const auto* n = dynamic_cast<const Literal*>(&expr)) {
        std::ostringstream stream;
        stream << std::setprecision(15) << std::get<double>(n->value);
        return stream.str();
    }
    if (const auto* s = dynamic_cast<const Symbol*>(&expr)) return s->name;
    if (const auto* b = dynamic_cast<const Binary*>(&expr))
        return "(" + format(*b->left) + " " + b->operation + " " + format(*b->right) + ")";
    if (const auto* u = dynamic_cast<const Unary*>(&expr)) return "(-" + format(*u->right) + ")";
    if (const auto* g = dynamic_cast<const Grouping*>(&expr)) return "(" + format(*g->expr) + ")";
    if (const auto* c = dynamic_cast<const Call*>(&expr)) {
        std::string result = c->function + "(";
        for (std::size_t i=0; i<c->arguments.size(); ++i) {
            if (i) result += ", ";
            result += format(*c->arguments[i]);
        }
        return result + ")";
    }
    throw std::runtime_error("Unknown expression type");
}
