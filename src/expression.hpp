/*
 *
 *    expression.hpp
 *    Version 1.00 (C++17)  
 * 
 *    Copyright 2026 Steven Janes (www.perfectconsulting.co.uk)
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "forward_references.hpp"
#include "environment.hpp"
#include "value.hpp"

namespace Basic {

class Expression {
public:
    virtual ~Expression() = default;
    virtual Value Evaluate(Environment& env) const = 0;
    virtual std::string List() const = 0;
};

using LambdaFn = std::function<Value(const std::vector<Value>&, Environment&)>;
using ExprListFn = std::function<std::string(const std::vector<ExpressionPtr>&)>;

// LambdaExpression is the single concrete Expression: rather than a fresh
// class per kind of expression, it defers Evaluate()/List() to a captured
// LambdaFn/ExprListFn. Every expression in the language - literals,
// variables, array reads, operators, and function calls alike - is built by
// a "Make" function that bakes the right lambda into a LambdaExpression.
// The operator/function "Make"s live in maths_ops.hpp, logic_ops.hpp and
// string_ops.hpp; the leaf expressions (literals, variables, array/l-value
// reads, and user-defined function calls) are declared below and
// implemented in expression.cpp.
class LambdaExpression : public Expression {
public:
    LambdaExpression(std::vector<ExpressionPtr> operands, LambdaFn fn, ExprListFn listFn);
    Value       Evaluate(Environment& env) const override;
    std::string List() const override;
private:
    std::vector<ExpressionPtr> m_operands;
    LambdaFn    m_fn;
    ExprListFn  m_listFn;
};

// ── Leaf expressions ─────────────────────────────────────────────────────────

ExpressionPtr MakeLiteral(const Value& value);
ExpressionPtr MakeStringLiteral(const std::string& value);
ExpressionPtr MakeVariable(const std::string& name);
ExpressionPtr MakeArray(std::string name, std::vector<ExpressionPtr> indices);
ExpressionPtr MakeLValue(LValue lv);
ExpressionPtr MakeUserFn(std::string name, ExpressionPtr arg);

} // namespace Basic
