/*
 *
 *    value.cpp
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

#include <stdexcept>
#include <cmath>
#include <type_traits>
#include "value.hpp"
#include "error.h"

namespace Basic {

namespace {
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
}

bool Value::GetTruthy() const {
    if (IsInteger()) return std::get<Integer>(m_data) != 0;
    if (IsReal()) return std::get<Real>(m_data) != 0.0;
    return !std::get<String>(m_data).empty();
}

Integer Value::GetInteger() const {
    if (IsInteger()) return std::get<Integer>(m_data);
    if (IsReal()) return static_cast<Integer>(std::get<Real>(m_data));
    throw std::runtime_error("cannot convert string to number");
}

Real Value::GetReal() const {   
    if (IsReal()) return std::get<Real>(m_data);
    if (IsInteger()) return static_cast<Real>(std::get<Integer>(m_data));
    throw std::runtime_error("cannot convert string to number");
}

String Value::GetString() const {      
    if (IsString()) return std::get<String>(m_data);
    throw std::runtime_error("cannot convert number to string");
}

Integer Value::TryGetInteger(Environment& env, const char* name) const {
    if (!IsNumber())
        throw RuntimeError(env, std::string(name) + ": expected integer argument");
    return GetInteger();
}

String Value::TryGetString(Environment& env, const char* name) const {
    if (!IsString())
        throw RuntimeError(env, std::string(name) + ": expected string argument");
    return GetString();    
};

String Value::PPrint(const bool quoteString) const {   
    if (IsString()) {
        if (quoteString) {
            return "\"" + std::get<String>(m_data) + "\"";
        }
        return std::get<String>(m_data);
    }
    if (IsInteger()) return std::to_string(std::get<Integer>(m_data));
    if (IsReal()) {
        std::string s = std::to_string(std::get<Real>(m_data));

        if (s.find('.') != std::string::npos)
        {
            s.erase(s.find_last_not_of('0') + 1);

            if (!s.empty() && s.back() == '.')
                s.pop_back();
        }

        return s;
    };

    throw std::runtime_error("cannot pretty print");
}

void Value::NormaliseType(bool forceString) {
    if (IsString()){
        if (forceString) return;
        
        bool converted = false;
        String s = std::get<String>(m_data);
        try {
            size_t pos;
            Integer iv = std::stol(s, &pos);
            if (pos == s.size()) { SetInteger(iv); converted = true; }
        }
        catch (...) {}

        try {
            size_t pos;
            Real dv = std::stod(s, &pos);
            if (pos == s.size()) { SetReal(dv); converted = true; }
        } catch (...) {}

        if (!converted) { SetInteger(0); };
    }
    else {
        if (forceString) {
            m_data = PPrint();
        }
    }
}

Value Value::AddOp(const Value& value, Environment& env) const{
    return std::visit(overloaded{
        [](const String& a, const String& b) -> Value { return Value(a + b); },
        [](const Integer& a, const Integer& b) -> Value { return Value(a + b); },
        [&env](const auto& a, const auto& b) -> Value {
            if constexpr (std::is_same_v<std::decay_t<decltype(a)>, String> || std::is_same_v<std::decay_t<decltype(b)>, String>)
                throw RuntimeError(env, "type mismatch: expected numeric or string operands");
            else
                return Value(static_cast<Real>(a) + static_cast<Real>(b));
        }
    }, m_data, value.m_data);
}

Value Value::SubOp(const Value& value, Environment& env) const {
    if (!IsNumber()|| ! value.IsNumber())
        throw RuntimeError(env, "type mismatch: expected numeric operands");

    if (IsReal() || value.IsReal())
    {
        return Value(GetReal() - value.GetReal());
    }

    return Value(GetInteger() - value.GetInteger());
}

Value Value::MulOp(const Value& value, Environment& env) const {
    if (!IsNumber()|| ! value.IsNumber())
        throw RuntimeError(env, "type mismatch: expected numeric operands");

    if (IsReal() || value.IsReal())
    {
        return Value(GetReal() * value.GetReal());
    }

    return Value(GetInteger() * value.GetInteger());;
}

Value Value::DivOp(const Value& value, Environment& env) const {
    if (!IsNumber()|| ! value.IsNumber())
        throw RuntimeError(env, "type mismatch: expected numeric operands");

    if (IsReal() || value.IsReal())
    {
        if (value.GetReal() == 0.0)
            throw RuntimeError(env, "division by zero");

        return Value(GetReal() / value.GetReal());
    }

    if (value.GetInteger() == 0)
        throw RuntimeError(env, "division by zero");

    return Value(GetInteger() / value.GetInteger());
}

int Value::CompareOp(const Value& value, Environment& env) const {
    return std::visit(overloaded{
        [](const String& a, const String& b) -> int { return a.compare(b); },
        [&env](const auto& a, const auto& b) -> int {
            if constexpr (std::is_same_v<std::decay_t<decltype(a)>, String> || std::is_same_v<std::decay_t<decltype(b)>, String>)
                throw RuntimeError(env, "unsupported value type in comparison");
            else
                return a < b ? -1 : a > b ? 1 : 0;
        }
    }, m_data, value.m_data);
}    

Value Value::PowOp(const Value& value, Environment& env) const {
    if (!IsNumber()|| ! value.IsNumber()){
        throw RuntimeError(env, "type mismatch: expected numeric operands");
    }

    return static_cast<int>(std::pow(GetReal(), value.GetReal()));
}   

Value Value::AbsOp(Environment& env) const {
    if (!IsNumber()){
        throw RuntimeError(env, "type mismatch: expected numeric operands");
    }

    if (IsReal()) {
        return Value(std::abs(GetReal()));
    }

    return Value(std::abs(GetInteger()));
}   

Value Value::AtnOp(Environment& env) const {
    if (!IsNumber()){
        throw RuntimeError(env, "type mismatch: expected numeric operands");
    }

    return Value(std::atan(GetReal()));
}   

Value Value::CosOp(Environment& env) const {
    if (!IsNumber()){
        throw RuntimeError(env, "type mismatch: expected numeric operands");
    }

    return Value(std::cos(GetReal()));
}   

Value Value::SinOp(Environment& env) const {
    if (!IsNumber()){
        throw RuntimeError(env, "type mismatch: expected numeric operands");
    }

    return Value(std::sin(GetReal()));
}   

Value Value::TanOp(Environment& env) const {
    if (!IsNumber()){
        throw RuntimeError(env, "type mismatch: expected numeric operands");
    }

    return Value(std::tan(GetReal()));
} 

Value Value::ExpOp(Environment& env) const {
    if (!IsNumber()){
        throw RuntimeError(env, "type mismatch: expected numeric operands");
    }

    return Value(std::exp(GetReal()));
}

Value Value::SqrOp(Environment& env) const {
    if (!IsNumber()){
        throw RuntimeError(env, "type mismatch: expected numeric operands");
    }

    if (IsZero()){
        throw RuntimeError(env, "argument can't be zero");
    }

    return Value(std::sqrt(GetReal()));
}

Value Value::IntOp(Environment& env) const {
    if (!IsNumber()){
        throw RuntimeError(env, "type mismatch: expected numeric operands");
    }


    return Value(std::floor(GetReal()));
}

Value Value::SgnOp(Environment& env) const {
    if (!IsNumber()){
        throw RuntimeError(env, "type mismatch: expected numeric operands");
    }

    return Value(CompareOp(Value(0), env));
}

Value Value::LogOp(Environment& env) const {
    if (!IsNumber()){
        throw RuntimeError(env, "type mismatch: expected numeric operands");
    }

    if (!IsPositive()){
        throw RuntimeError(env, "argument must be positive");
    }


    return Value(std::log(GetReal()));
}

Value Value::RndOp(Environment& env) const {
    Real n = (static_cast<double>(rand()) / RAND_MAX) * GetInteger();
    return Value(n);
}

} // namespace Basic