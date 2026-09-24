/*
 *
 *    value.hpp
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
#include <variant>
#include <string>
#include <vector>
#include <memory>
#include "forward_references.hpp"
#include "error.hpp"

namespace Basic {

using Real = float;
using Integer = int;
using String = std::string;

class Environment;

class Value {
    public:
        Value() : m_data(Integer(0)) { }
        Value(Real r) : m_data(r) { }
        Value(String s) : m_data(std::move(s)) { }
        Value(Integer i) : m_data(i) { }
        Value(const Value& value) = default;

        void SetInteger(Integer i) { m_data = i; }
        void SetReal(Real r) { m_data = r; }
        void SetString(String s) { m_data = std::move(s); }

        bool IsInteger() const { return std::holds_alternative<Integer>(m_data); }
        bool IsReal() const { return std::holds_alternative<Real>(m_data); }
        bool IsString() const { return std::holds_alternative<String>(m_data); }
        bool IsNumber() const { return !IsString(); }
        bool IsZero() const { return (IsInteger() && std::get<Integer>(m_data) == 0) || (IsReal() && std::get<Real>(m_data) == 0.0); }
        bool IsPositive() const { return (IsInteger() && std::get<Integer>(m_data) > 0) || (IsReal() && std::get<Real>(m_data) > 0.0); }

        Real GetReal() const;
        Integer GetInteger() const;
        String GetString() const;
        bool GetTruthy() const;
        
        Integer TryGetInteger(Environment& env, const char* name) const;
        String TryGetString(Environment& env, const char* name) const;
    
        String PPrint(const bool quoteString = false) const;  
        void NormaliseType(bool forceString = false);

        Value AddOp(const Value& value, Environment& env) const;
        Value SubOp(const Value& value, Environment& env) const;
        Value MulOp(const Value& value, Environment& env) const;
        Value DivOp(const Value& value, Environment& env) const;
        Value PowOp(const Value& value, Environment& env) const;

        Value AbsOp(Environment& env) const;
        Value AtnOp(Environment& env) const;
        Value CosOp(Environment& env) const;
        Value SinOp(Environment& env) const;
        Value TanOp(Environment& env) const;

        Value ExpOp(Environment& env) const;
        Value SqrOp(Environment& env) const;
        Value IntOp(Environment& env) const;
        Value SgnOp(Environment& env) const;
        Value LogOp(Environment& env) const;
        Value RndOp(Environment& env) const;


        int CompareOp(const Value& value, Environment& env) const;
    private:
        std::variant<Integer, Real, String> m_data;
};

// ── Unified l-value: a variable name plus optional index expressions. ──
// Used by LET, INPUT, READ, and FOR – all assignment targets share this type.
struct LValue {
    std::string          name;
    std::vector<ExpressionPtr> indices;  // empty → scalar

    bool isString()  const { return !name.empty() && name.back() == '$'; }
    std::string List() const;
};


} // namespace Basic
