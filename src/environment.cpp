/*
 *
 *    environment.cpp
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

#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <csignal>
#include "environment.hpp"
#include "error.hpp"
#include "statement.hpp"
#include "expression.hpp"
#include "flex_interface.h"
#include "parse_error.hpp"

namespace Basic {



std::string LValue::List() const {
    if (indices.empty()) return name;
    std::string s = name + "(";
    for (size_t i = 0; i < indices.size(); ++i) {
        if (i > 0) s += ", ";
        s += indices[i]->List();
    }
    return s + ")";
}

static bool IsStringVar(const std::string& name) {
    return !name.empty() && name.back() == '$';
}

void Environment::SetVariable(const std::string& name, const Value& value) {
    bool strVar = IsStringVar(name);
    bool strVal = value.IsString();
    if (strVar && !strVal)
        throw RuntimeError(*this, "type mismatch: cannot assign number to string variable '" + name + "'");
    if (!strVar && strVal)
        throw RuntimeError(*this, "type mismatch: cannot assign string to numeric variable '" + name + "'");
    m_variables[name] = value;
}

Value Environment::GetVariable(const std::string& name) const {
    auto it = m_variables.find(name);
    if (it != m_variables.end()) return it->second;
    return IsStringVar(name) ? Value(std::string("")) : Value(0);
}

int Environment::ArrayData::flatIndex(const std::vector<int>& indices) const {
    int idx = 0, stride = 1;
    for (int i = (int)dims.size() - 1; i >= 0; --i) {
        idx    += indices[i] * stride;
        stride *= (dims[i] + 1);
    }
    return idx;
}

Environment::ArrayData Environment::MakeArrayData(const std::string& name, const std::vector<int>& dims) {
    ArrayData arr;
    arr.dims     = dims;
    arr.isString = IsStringVar(name);
    int total    = 1;
    for (int d : dims) total *= (d + 1);
    arr.data.assign(total, arr.isString ? Value(std::string("")) : Value(0));
    return arr;
}

void Environment::DimArray(const std::string& name, const std::vector<int>& dims) {
    // Microsoft BASIC: once an array has been dimensioned -- whether explicitly
    // via DIM, or implicitly by first use (auto-dim) -- a further DIM on the
    // same name is illegal until a CLEAR/ERASE happens. Re-dimensioning would
    // silently reallocate and wipe live data, which is exactly the kind of
    // bug this check is meant to catch.
    if (m_arrays.count(name))
        throw RuntimeError(*this, "array already dimensioned: '" + name + "'");
    m_arrays[name] = MakeArrayData(name, dims);
}

Environment::ArrayData& Environment::ResolveArray(const std::string& name, size_t rank) {
    auto it = m_arrays.find(name);
    if (it == m_arrays.end())
        it = m_arrays.emplace(name, MakeArrayData(name, std::vector<int>(rank, 10))).first;
    return it->second;
}

void Environment::ValidateIndices(const std::string& name, const ArrayData& arr, const std::vector<int>& indices) const {
    if (indices.size() != arr.dims.size())
        throw RuntimeError(const_cast<Environment&>(*this), "array dimension mismatch for '" + name + "'");
    for (size_t i = 0; i < indices.size(); ++i)
        if (indices[i] < 0 || indices[i] > arr.dims[i])
            throw RuntimeError(const_cast<Environment&>(*this), "array subscript out of range for '" + name + "'");
}

void Environment::SetArray(const std::string& name, const std::vector<int>& indices, const Value& value) {
    ArrayData& arr = ResolveArray(name, indices.size());
    ValidateIndices(name, arr, indices);
    if (arr.isString && !value.IsString())
        throw RuntimeError(*this, "type mismatch: cannot assign number to string array '" + name + "'");
    if (!arr.isString && value.IsString())
        throw RuntimeError(*this, "type mismatch: cannot assign string to numeric array '" + name + "'");
    arr.data[arr.flatIndex(indices)] = value;
}

Value Environment::GetArray(const std::string& name, const std::vector<int>& indices) const {
    ArrayData& arr = const_cast<Environment*>(this)->ResolveArray(name, indices.size());
    ValidateIndices(name, arr, indices);
    return arr.data[arr.flatIndex(indices)];
}

void Environment::Assign(const LValue& lv, const Value& val) {
    if (lv.indices.empty()) {
        SetVariable(lv.name, val);
    } else {
        std::vector<int> idx;
        idx.reserve(lv.indices.size());
        for (auto& e : lv.indices)
            idx.push_back(e->Evaluate(*this).GetInteger());
        SetArray(lv.name, idx, val);
    }
}

void Environment::ClearVariables() {
    m_variables.clear();
    m_arrays.clear();
    m_dataManager.ClearData();
}

// Used by RUN: Microsoft BASIC's RUN is equivalent to "CLEAR: GOTO <line>" --
// it undefines every variable and array so the program starts from a clean
// slate each time, but (unlike the CLEAR statement above) it must NOT touch
// DATA, since DATA items are collected once at load/parse time rather than
// re-executed on every run.
void Environment::ResetVariablesAndArrays() {
    m_variables.clear();
    m_arrays.clear();
}

void Environment::DefFunction(const std::string& name, const std::string& param, ExpressionPtr body) {
    m_functions[name] = { param, std::move(body) };
}

Value Environment::CallFunction(const std::string& name, const Value& arg) {
    auto it = m_functions.find(name);
    if (it == m_functions.end())
        throw RuntimeError(*this, "undefined function '" + name + "'");
    Value saved = GetVariable(it->second.param);
    m_variables[it->second.param] = arg;
    Value result = it->second.body->Evaluate(*this);
    m_variables[it->second.param] = saved;
    return result;
}

void Environment::Randomize(long seed) {
    std::srand(seed < 0 ? static_cast<unsigned>(std::time(nullptr))
                        : static_cast<unsigned>(seed));
}

} // namespace Basic
