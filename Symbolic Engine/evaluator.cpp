#include "engine.hpp"
#include <cmath>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

using BinaryOperation = double (*)(double, double);
using FunctionOperation = double (*)(const std::vector<double>&);

static const std::unordered_map<std::string, BinaryOperation> binary_operations{
    {"+", [](double left, double right) { return left + right; }},
    {"-", [](double left, double right) { return left - right; }},
    {"*", [](double left, double right) { return left * right; }},
    {"/", [](double left, double right) {
        if (right == 0.0) throw std::runtime_error("Division by zero");
        return left / right;
    }},
    {"^", [](double left, double right) {
        if ((left == 0.0 && right <= 0.0) ||
            (left < 0.0 && std::trunc(right) != right)) {
            throw std::runtime_error("Power is undefined in the supported real-number domain");
        }
        return std::pow(left, right);
    }},
    // The evaluator returns double, so comparisons produce 1 or 0.
    {"=",  [](double left, double right) -> double { return left == right; }},
    {"!=", [](double left, double right) -> double { return left != right; }},
    {">",  [](double left, double right) -> double { return left > right; }},
    {">=", [](double left, double right) -> double { return left >= right; }},
    {"<",  [](double left, double right) -> double { return left < right; }},
    {"<=", [](double left, double right) -> double { return left <= right; }}
};

static void require_arguments(const std::vector<double>& arguments,
                              std::size_t expected, const std::string& function) {
    if (arguments.size() != expected) {
        throw std::runtime_error(function + " expects " + std::to_string(expected) +
                                 " arguments, got " + std::to_string(arguments.size()));
    }
}

static const std::unordered_map<std::string, FunctionOperation> function_operations{
    {"sin", [](const std::vector<double>& arguments) {
        require_arguments(arguments, 1, "sin");
        return std::sin(arguments[0]);
    }},
    {"cos", [](const std::vector<double>& arguments) {
        require_arguments(arguments, 1, "cos");
        return std::cos(arguments[0]);
    }},
    {"tan", [](const std::vector<double>& arguments) {
        require_arguments(arguments, 1, "tan");
        return std::tan(arguments[0]);
    }},
    {"sqrt", [](const std::vector<double>& arguments) {
        require_arguments(arguments, 1, "sqrt");
        if (arguments[0] < 0.0) throw std::runtime_error("sqrt requires a nonnegative argument");
        return std::sqrt(arguments[0]);
    }},
    // Convention: log(value, base).
    {"log", [](const std::vector<double>& arguments) {
        require_arguments(arguments, 2, "log");
        const double value = arguments[0];
        const double base = arguments[1];
        if (value <= 0.0 || base <= 0.0 || base == 1.0) {
            throw std::runtime_error("log requires value > 0, base > 0, and base != 1");
        }
        return std::log(value) / std::log(base);
    }},
    // Floating-point remainder; its sign follows the first argument.
    {"mod", [](const std::vector<double>& arguments) {
        require_arguments(arguments, 2, "mod");
        if (arguments[1] == 0.0) throw std::runtime_error("mod divisor cannot be zero");
        return std::fmod(arguments[0], arguments[1]);
    }}
};

static double finite_result(double value) {
    if (!std::isfinite(value)) {
        throw std::runtime_error("Evaluation produced a non-finite result");
    }
    return value;
}

// Evaluate children before their parent. Every symbol encountered must have a value.
double evaluate(const Expr& expr, const Environment& values) {
    if (const auto* number = dynamic_cast<const Literal*>(&expr)) {
        return finite_result(std::get<double>(number->value));
    } else if (const auto* binary = dynamic_cast<const Binary*>(&expr)) {
        const auto operation = binary_operations.find(binary->operation);
        if (operation == binary_operations.end()) {
            throw std::runtime_error("Unsupported binary operator: " + binary->operation);
        }
        const double left = evaluate(*binary->left, values);
        const double right = evaluate(*binary->right, values);
        return finite_result(operation->second(left, right));
    } else if (const auto* unary = dynamic_cast<const Unary*>(&expr)) {
        if (unary->operation != '-') {
            throw std::runtime_error(std::string("Unsupported unary operator: ") + unary->operation);
        }
        return -evaluate(*unary->right, values);
    } else if (const auto* grouping = dynamic_cast<const Grouping*>(&expr)) {
        return evaluate(*grouping->expr, values);
    } else if (const auto* call = dynamic_cast<const Call*>(&expr)) {
        const auto operation = function_operations.find(call->function);
        if (operation == function_operations.end()) {
            throw std::runtime_error("Unsupported numeric function: " + call->function);
        }
        std::vector<double> arguments;
        arguments.reserve(call->arguments.size());
        for (const auto& argument : call->arguments) {
            arguments.push_back(evaluate(*argument, values));
        }
        return finite_result(operation->second(arguments));
    } else if (const auto* symbol = dynamic_cast<const Symbol*>(&expr)) {
        const auto found = values.find(symbol->name);
        if (found == values.end()) {
            throw std::runtime_error("No numeric value supplied for symbol: " + symbol->name);
        }
        return finite_result(found->second);
    }
    throw std::runtime_error("Unsupported expression type");
}
