#include "engine.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <stdexcept>

Node decoding(const nlohmann::json& data);

// Serialize result trees using the same node schema accepted by the decoder.
nlohmann::json to_json(const Expr& expr) {
    if (const auto* n=dynamic_cast<const Literal*>(&expr))
        return {{"kind","number"},{"value",std::get<double>(n->value)}};
    if (const auto* s=dynamic_cast<const Symbol*>(&expr)) return {{"kind","symbol"},{"name",s->name}};
    if (const auto* b=dynamic_cast<const Binary*>(&expr))
        return {{"kind","binary"},{"operator",b->operation},{"left",to_json(*b->left)},{"right",to_json(*b->right)}};
    if (const auto* u=dynamic_cast<const Unary*>(&expr))
        return {{"kind","unary"},{"operator",std::string(1,u->operation)},{"right",to_json(*u->right)}};
    if (const auto* g=dynamic_cast<const Grouping*>(&expr))
        return {{"kind","grouping"},{"expression",to_json(*g->expr)}};
    if (const auto* c=dynamic_cast<const Call*>(&expr)) {
        auto args=nlohmann::json::array();
        for (const auto& a:c->arguments) args.push_back(to_json(*a));
        return {{"kind","call"},{"operator",c->function},{"arguments",args}};
    }
    throw std::runtime_error("Unknown output node");
}

// One request per process: JSON on stdin, then a result or error object on stdout.
int main() {
    try {
        auto request=nlohmann::json::parse(std::cin);
        auto tree=decoding(request.at("ast"));
        const std::string mode=request.value("mode","simplify");
        const std::string variable=request.value("variable","x");
        Environment values=request.value("values",Environment{});
        nlohmann::json output;
        if (mode=="evaluate") output["value"]=evaluate(*tree,values);
        else if (mode=="autodiff") {
            auto result=autodiff(*tree,values,variable);
            output={{"value",result.value},{"derivative",result.derivative}};
        } else {
            Node result;
            if (mode=="simplify") result=simplify(*tree,values);
            else if (mode=="differentiate") {
                auto derivative=differentiate(*tree,variable);
                output["raw_derivative"]=format(*derivative);
                result=simplify(*derivative);
                if (request.contains("values")) {
                    // Check original domain and the local derivative rules at this point.
                    (void)autodiff(*tree,values,variable);
                    output["value"]=evaluate(*result,values);
                }
            } else throw std::runtime_error("Unknown mode: " + mode);
            output["text"]=format(*result);
            output["ast"]=to_json(*result);
        }
        std::cout << output.dump() << '\n';
    } catch (const std::exception& error) {
        std::cout << nlohmann::json{{"error",error.what()}}.dump() << '\n';
        return 1;
    }
}
