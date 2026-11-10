/*
 *
 *    variant.hpp
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

class Variant {
    public:
        Variant() : m_data(Integer(0)) { }
        Variant(Real r) : m_data(r) { }
        Variant(String s) : m_data(std::move(s)) { }
        Variant(Integer i) : m_data(i) { }
        Variant(const Variant& value) = default;

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

        Variant AddOp(const Variant& value, Environment& env) const;
        Variant SubOp(const Variant& value, Environment& env) const;
        Variant MulOp(const Variant& value, Environment& env) const;
        Variant DivOp(const Variant& value, Environment& env) const;
        Variant PowOp(const Variant& value, Environment& env) const;

        Variant AbsOp(Environment& env) const;
        Variant AtnOp(Environment& env) const;
        Variant CosOp(Environment& env) const;
        Variant SinOp(Environment& env) const;
        Variant TanOp(Environment& env) const;

        Variant ExpOp(Environment& env) const;
        Variant SqrOp(Environment& env) const;
        Variant IntOp(Environment& env) const;
        Variant SgnOp(Environment& env) const;
        Variant LogOp(Environment& env) const;
        Variant RndOp(Environment& env) const;


        int CompareOp(const Variant& value, Environment& env) const;
    private:
        std::variant<Integer, Real, String> m_data;
};

} // namespace Basic
