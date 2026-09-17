#include "engine.hpp"
#include <cmath>
#include <stdexcept>

namespace {
// check syntactic dependence, not just the derivative at the current point
// For example, x*x has derivative zero at x=0 but is still a variable exponent.
bool depends_on(const Expr& expr, const std::string& variable) {
    if (const auto* s=dynamic_cast<const Symbol*>(&expr)) return s->name==variable;
    if (const auto* b=dynamic_cast<const Binary*>(&expr))
        return depends_on(*b->left,variable) || depends_on(*b->right,variable);
    if (const auto* u=dynamic_cast<const Unary*>(&expr)) return depends_on(*u->right,variable);
    if (const auto* g=dynamic_cast<const Grouping*>(&expr)) return depends_on(*g->expr,variable);
    if (const auto* c=dynamic_cast<const Call*>(&expr)) {
        for (const auto& a:c->arguments) if (depends_on(*a,variable)) return true;
    }
    return false;
}
ValueAndDerivative checked(double value, double derivative) {
    if (!std::isfinite(value) || !std::isfinite(derivative))
        throw std::runtime_error("Non-finite value or derivative");
    return {value,derivative};
}
}

// forward mode propagates (value, derivative) pairs, without building derivative trees
ValueAndDerivative autodiff(const Expr& expr, const Environment& values, const std::string& variable) {
    if (dynamic_cast<const Literal*>(&expr)) return {evaluate(expr,values),0};
    if (const auto* s=dynamic_cast<const Symbol*>(&expr)) {
        // seed the selected variable with derivative 1. other symbols are constant
        double derivative = 0.0;
        if (s->name == variable) {
            derivative = 1.0;
        }
        return {evaluate(expr, values), derivative};
    }
    if (const auto* g=dynamic_cast<const Grouping*>(&expr)) return autodiff(*g->expr,values,variable);
    if (const auto* u=dynamic_cast<const Unary*>(&expr)) {
        if (u->operation!='-') throw std::runtime_error("Unsupported unary operator");
        auto right=autodiff(*u->right,values,variable);
        return checked(-right.value,-right.derivative);
    }
    if (const auto* b=dynamic_cast<const Binary*>(&expr)) {
        auto left=autodiff(*b->left,values,variable);
        auto right=autodiff(*b->right,values,variable);
        // Reuse the numerical evaluator's domain checks on a tiny constant node
        auto numeric=make_binary(b->operation,make_number(left.value),make_number(right.value));
        const double value=evaluate(*numeric);
        if (b->operation=="+"){
            return checked(value,left.derivative+right.derivative);
        }
        if (b->operation=="-") {
            return checked(value,left.derivative-right.derivative);
        }
        if (b->operation=="*"){ 
            return checked(value,left.derivative*right.value+left.value*right.derivative);
        }
        if (b->operation=="/"){
            return checked(value,(left.derivative*right.value-left.value*right.derivative)/(right.value*right.value));
        }
        if (b->operation=="^") {
            if (!depends_on(*b->right,variable)) {
                if (right.value==0) return {value,0};
                if (right.value==1) return checked(value,left.derivative);
                if (left.value<0 && std::trunc(right.value)!=right.value)
                    throw std::runtime_error("Real power derivative is undefined");
                return checked(value,right.value*std::pow(left.value,right.value-1)*left.derivative);
            }
            if (left.value<=0) throw std::runtime_error("Variable-exponent autodiff requires a positive base");
            return checked(value,value*(right.derivative*std::log(left.value)+right.value*left.derivative/left.value));
        }
        throw std::runtime_error("Autodiff does not support comparisons");
    }
    if (const auto* c=dynamic_cast<const Call*>(&expr)) {
        validate_function(c->function,c->arguments.size());
        std::vector<ValueAndDerivative> args;
        auto numeric=std::make_unique<Call>();
        numeric->function=c->function;
        for (const auto& arg:c->arguments) {
            args.push_back(autodiff(*arg,values,variable));
            numeric->arguments.push_back(make_number(args.back().value));
        }
        const double value=evaluate(*numeric);
        const double u=args[0].value, du=args[0].derivative;
        if (c->function=="sin") return checked(value,std::cos(u)*du);
        if (c->function=="cos") return checked(value,-std::sin(u)*du);
        if (c->function=="tan") return checked(value,du/(std::cos(u)*std::cos(u)));
        if (c->function=="sqrt") {
            if (u==0) throw std::runtime_error("sqrt derivative requires a positive argument");
            return checked(value,du/(2*std::sqrt(u)));
        }
        if (c->function=="log") {
            const double base=args[1].value, db=args[1].derivative;
            return checked(value,((du/u)*std::log(base)-std::log(u)*(db/base))/(std::log(base)*std::log(base)));
        }
        throw std::runtime_error("Autodiff not implemented for " + c->function);
    }
    throw std::runtime_error("Unknown autodiff node");
}
