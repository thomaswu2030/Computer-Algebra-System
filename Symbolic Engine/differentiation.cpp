#include "engine.hpp"
#include <stdexcept>
#include <utility>

// Builds a new tree.
// Other symbols are treated as constants with respect to the selected variable
// This function builds the raw derivative,. the runner calls simplify() afterward
Node differentiate(const Expr& expr, const std::string& variable) {
    if (dynamic_cast<const Literal*>(&expr)) return make_number(0);
    if (const auto* symbol = dynamic_cast<const Symbol*>(&expr)) {
        if (symbol->name == variable) {
            return make_number(1);
        }
        return make_number(0);
    }
    if (const auto* grouping = dynamic_cast<const Grouping*>(&expr))
        return differentiate(*grouping->expr,variable);
    if (const auto* unary = dynamic_cast<const Unary*>(&expr)) {
        if (unary->operation != '-') throw std::runtime_error("Unsupported unary derivative");
        return make_negative(differentiate(*unary->right,variable));
    }
    if (const auto* binary = dynamic_cast<const Binary*>(&expr)) {
        const Expr& u = *binary->left;
        const Expr& v = *binary->right;
        const auto& op = binary->operation;
        if (op == "+" || op == "-")
            return make_binary(op,differentiate(u,variable),differentiate(v,variable));
        if (op == "*") {
            // (u*v)' = u'*v + u*v'
            auto first = make_binary("*",differentiate(u,variable),clone(v));
            auto second = make_binary("*",clone(u),differentiate(v,variable));
            return make_binary("+",std::move(first),std::move(second));
        }
        if (op == "/") {
            // (u/v)' = (u'*v - u*v') / v^2
            auto first = make_binary("*",differentiate(u,variable),clone(v));
            auto second = make_binary("*",clone(u),differentiate(v,variable));
            auto numerator = make_binary("-",std::move(first),std::move(second));
            return make_binary("/",std::move(numerator),make_binary("^",clone(v),make_number(2)));
        }
        if (op == "^") {
            // Simplify a numeric exponent such as (1+2) before selecting the rule.
            auto exponent = simplify(v);
            if (const auto* literal = dynamic_cast<const Literal*>(exponent.get())) {
                double n = std::get<double>(literal->value);
                if (n == 0) return make_number(0); // Only on the original domain.
                if (n == 1) return differentiate(u,variable);
                // Avoid introducing 0^0 in the derivative of x^2 at x=0.
                Node power;
                if (n == 2) {
                    power = clone(u);
                } else {
                    power = make_binary("^", clone(u), make_number(n-1));
                }
                auto factor = make_binary("*",make_number(n),std::move(power));
                return make_binary("*",std::move(factor),differentiate(u,variable));
            }
            // General real-valued rule: u^v * (v'*ln(u) + v*u'/u), requiring u>0.
            auto first = make_binary("*",differentiate(v,variable),make_log(clone(u)));
            auto second = make_binary("*",clone(v),make_binary("/",differentiate(u,variable),clone(u)));
            return make_binary("*",clone(expr),make_binary("+",std::move(first),std::move(second)));
        }
        throw std::runtime_error("Differentiation does not support comparisons");
    }
    if (const auto* call = dynamic_cast<const Call*>(&expr)) {
        validate_function(call->function,call->arguments.size());
        const Expr& u = *call->arguments[0];
        auto du = differentiate(u,variable);
        if (call->function == "sin")
            return make_binary("*",make_call("cos",clone(u)),std::move(du));
        if (call->function == "cos")
            return make_binary("*",make_negative(make_call("sin",clone(u))),std::move(du));
        if (call->function == "tan")
            return make_binary("/",std::move(du),make_binary("^",make_call("cos",clone(u)),make_number(2)));
        if (call->function == "sqrt")
            return make_binary("/",std::move(du),make_binary("*",make_number(2),make_call("sqrt",clone(u))));
        if (call->function == "log") {
            // log(u,b) = ln(u)/ln(b)
            const Expr& base = *call->arguments[1];
            auto first = make_binary("*",make_binary("/",std::move(du),clone(u)),make_log(clone(base)));
            auto second = make_binary("*",make_log(clone(u)),
                                      make_binary("/",differentiate(base,variable),clone(base)));
            auto numerator = make_binary("-",std::move(first),std::move(second));
            return make_binary("/",std::move(numerator),make_binary("^",make_log(clone(base)),make_number(2)));
        }
        throw std::runtime_error("Derivative not implemented for " + call->function);
    }
    throw std::runtime_error("Unknown derivative node");
}
