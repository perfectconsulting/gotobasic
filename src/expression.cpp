/*
 *
 *    expression.cpp
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

#include "expression.hpp"

namespace Basic {

namespace {

// Shared List() formatting for "name" or "name(idx, idx, ...)", used by
// both MakeArray and AssignTarget::List.
std::string FormatNameIndices(const std::string& name, const std::vector<ExpressionPtr>& indices) {
    if (indices.empty()) return name;
    std::string s = name + "(";
    for (size_t i = 0; i < indices.size(); ++i) {
        if (i > 0) s += ", ";
        s += indices[i]->List();
    }
    return s + ")";
}

std::vector<int> ToIntegers(const std::vector<Variant>& args) {
    std::vector<int> idx;
    idx.reserve(args.size());
    for (auto& v : args)
        idx.push_back(v.GetInteger());
    return idx;
}

} // namespace

std::string AssignTarget::List() const {
    return FormatNameIndices(name, indices);
}

LambdaExpression::LambdaExpression(std::vector<ExpressionPtr> operands, LambdaFn fn, ExprListFn listFn)
    : m_operands(std::move(operands)), m_fn(std::move(fn)), m_listFn(std::move(listFn)) {}

Variant LambdaExpression::Evaluate(Environment& env) const {
    std::vector<Variant> args;
    args.reserve(m_operands.size());
    for (auto& operand : m_operands)
        args.push_back(operand->Evaluate(env));
    return m_fn(args, env);
}

std::string LambdaExpression::List() const {
    return m_listFn(m_operands);
}

// ── Constants ─────────────────────────────────────────────────────────────────
ExpressionPtr MakeLiteral(const Variant& value) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{},
        [value](const std::vector<Variant>&, Environment&) -> Variant {
            return value;
        },
        [value](const std::vector<ExpressionPtr>&) -> std::string {
            return value.PPrint(true);
        });
}

ExpressionPtr MakeStringLiteral(const std::string& value) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{},
        [value](const std::vector<Variant>&, Environment&) -> Variant {
            return Variant(value);
        },
        [value](const std::vector<ExpressionPtr>&) -> std::string {
            return "\"" + value + "\"";
        });
}

// ── Variable and array reads ──────────────────────────────────────────────────
ExpressionPtr MakeVariable(const std::string& name) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{},
        [name](const std::vector<Variant>&, Environment& env) -> Variant {
            return env.GetVariable(name);
        },
        [name](const std::vector<ExpressionPtr>&) -> std::string {
            return name;
        });
}

ExpressionPtr MakeArray(std::string name, std::vector<ExpressionPtr> indices) {
    return std::make_shared<LambdaExpression>(std::move(indices),
        [name](const std::vector<Variant>& args, Environment& env) -> Variant {
            return env.GetArray(name, ToIntegers(args));
        },
        [name](const std::vector<ExpressionPtr>& ops) -> std::string {
            return FormatNameIndices(name, ops);
        });
}

// ── User-defined functions ────────────────────────────────────────────────────
ExpressionPtr MakeUserFn(std::string name, ExpressionPtr arg) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(arg)},
        [name](const std::vector<Variant>& args, Environment& env) -> Variant {
            return env.CallFunction(name, args[0]);
        },
        [name](const std::vector<ExpressionPtr>& ops) -> std::string {
            return name + "(" + ops[0]->List() + ")";
        });
}

} // namespace Basic
