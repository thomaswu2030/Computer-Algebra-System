#include "engine.hpp"
#include <stdexcept>
#include <utility>

namespace {
bool is_number(const Expr& expr, double value) {
    const auto* number = dynamic_cast<const Literal*>(&expr);
    return number && std::get<double>(number->value) == value;
}
}

// One recursive pass of local rewrites; this does not expand or factor polynomials.
Node simplify(const Expr& expr, const Environment& known_values) {
    // Rebuild from the leaves upward. Substitution can turn a symbolic subtree
    // into constants, allowing the parent operation to be evaluated next.
    if (dynamic_cast<const Literal*>(&expr)) return clone(expr);
    if (const auto* symbol = dynamic_cast<const Symbol*>(&expr)) {
        const auto found = known_values.find(symbol->name);
        if (found != known_values.end()) return make_number(found->second);
        return clone(expr);
    }
    if (const auto* grouping = dynamic_cast<const Grouping*>(&expr))
        return simplify(*grouping->expr, known_values);

    if (const auto* unary = dynamic_cast<const Unary*>(&expr)) {
        if (unary->operation != '-') throw std::runtime_error("Unsupported unary operator");
        auto right = simplify(*unary->right, known_values);
        if (const auto* number = dynamic_cast<const Literal*>(right.get()))
            return make_number(-std::get<double>(number->value));
        if (const auto* nested = dynamic_cast<const Unary*>(right.get()))
            return clone(*nested->right);
        return make_negative(std::move(right));
    }
    if (const auto* binary = dynamic_cast<const Binary*>(&expr)) {
        // Own the new children independently, leave the input expression intact.
        auto left = simplify(*binary->left, known_values);
        auto right = simplify(*binary->right, known_values);
        const auto& operation = binary->operation;

        // reuse the evaluator only when both simplified operands are numeric.
        if (dynamic_cast<const Literal*>(left.get()) && dynamic_cast<const Literal*>(right.get())) {
            auto numeric = make_binary(operation, std::move(left), std::move(right));
            return make_number(evaluate(*numeric));
        }
        if (operation == "+") {
            if (is_number(*left,0)) return right;
            if (is_number(*right,0)) return left;
            if (same_expression(*left,*right)) return make_binary("*",make_number(2),std::move(left));
        }
        if (operation == "-") {
            if (is_number(*right,0)) return left;
            if (is_number(*left,0)) return make_negative(std::move(right));
            if (same_expression(*left,*right) && always_defined(*left)) return make_number(0);
        }
        if (operation == "*") {
            if (is_number(*left,1)) return right;
            if (is_number(*right,1)) return left;
            // Do not erase a possibly undefined subtree: 0*(1/x) must retain x!=0.
            if ((is_number(*left,0) && always_defined(*right)) || (is_number(*right,0) && always_defined(*left))){
                return make_number(0);
            }
            if (is_number(*left,-1)) return make_negative(std::move(right));
            if (is_number(*right,-1)) return make_negative(std::move(left));
        }
        if ((operation == "/" || operation == "^") && is_number(*right,1)) return left;
        return make_binary(operation,std::move(left),std::move(right));
    }
    if (const auto* call = dynamic_cast<const Call*>(&expr)) {
        validate_function(call->function,call->arguments.size());
        auto result = std::make_unique<Call>();
        result->function = call->function;
        bool all_numeric = true;
        for (const auto& argument : call->arguments) {
            auto reduced = simplify(*argument,known_values);
            if (!dynamic_cast<const Literal*>(reduced.get())) all_numeric = false;
            result->arguments.push_back(std::move(reduced));
        }
        if (all_numeric) return make_number(evaluate(*result));
        return result;
    }
    throw std::runtime_error("Unsupported simplification node");
}
