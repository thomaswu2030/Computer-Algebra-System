#include "../../Structures/asts.hpp"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <fstream>
#include <unordered_set>

static const std::unordered_set<std::string> valid_operations{
    "+", "-", "*", "/", "^",
    "=", "!=", ">", ">=", "<", "<="
};


std::unique_ptr<Expr> decoding(const nlohmann::json& data){
    // The kind field selects a node type. Recursive calls reconstruct children
    // from the same JSON structure produced by the Python encoder.

    std::string kind = data.at("kind").get<std::string>();

    if (kind == "number") {
        double number = data.at("value").get<double>();
        auto node = std::make_unique<Literal>();
        node->value = number;
        return node;
    }

    if (kind == "symbol") {
        auto node = std::make_unique<Symbol>();
        node->name = data.at("name").get<std::string>();
        return node;
    }

    if (kind == "grouping") {
        auto node = std::make_unique<Grouping>();
        node->expr = decoding(data.at("expression"));
        return node;
    }

    if (kind == "binary") {
        auto node = std::make_unique<Binary>();
        const std::string operation = data.at("operator").get<std::string>();
        if (!valid_operations.contains(operation)) {
            throw std::runtime_error("Unsupported binary operator: " + operation);
        }
        node->operation = operation;
        node->left = decoding(data.at("left"));
        node->right = decoding(data.at("right"));
        return node;
    }

    if (kind == "unary") {
        auto node = std::make_unique<Unary>();
        const std::string operation = data.at("operator").get<std::string>();
        if (operation != "-") {
            throw std::runtime_error("Unsupported unary operator: " + operation);
        }
        node->operation = operation[0];
        node->right = decoding(data.at("right"));
        return node;
    }

    if (kind == "call") {
        auto node = std::make_unique<Call>();
        node->function = data.at("operator").get<std::string>();
        const auto& arguments = data.at("arguments");
        if (!arguments.is_array()) {
            throw std::runtime_error("Call arguments must be a JSON array");
        }
        for (const auto& argument : arguments) {
            node->arguments.push_back(decoding(argument));
        }
        return node;
    }

    throw std::runtime_error("Unsupported AST kind: " + kind);
}

std::unique_ptr<Expr> decode_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open AST file: " + path);
    }
    return decoding(nlohmann::json::parse(file));
}
