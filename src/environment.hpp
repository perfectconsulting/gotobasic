/*
 *
 *    environment.hpp
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

#include <map>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>
#include "forward_references.hpp"
#include "value.hpp"
#include "data_manager.hpp"
#include "program_manager.hpp"

namespace Basic {

class Value;

class Environment {
public:
    Environment() : m_dataManager(*this), m_programManager(*this) {}

    void  SetVariable(const std::string& name, const Value& value);
    Value GetVariable(const std::string& name) const;

    void  DimArray(const std::string& name, const std::vector<int>& dims);
    void  SetArray(const std::string& name, const std::vector<int>& indices, const Value& value);
    Value GetArray(const std::string& name, const std::vector<int>& indices) const;

    // Assign to an LValue (handles both scalar and array).
    void  Assign(const LValue& lv, const Value& val);

    void ClearVariables();
    void ResetVariablesAndArrays();

    void  DefFunction(const std::string& name, const std::string& param, ExpressionPtr body);
    Value CallFunction(const std::string& name, const Value& arg);

    void  Randomize(long seed = -1);
    DataManager& GetDataManager() { return m_dataManager; }
    ProgramManager& GetProgramManager() { return m_programManager; }

private:
    struct ArrayData {
        std::vector<int>   dims;
        std::vector<Value> data;
        bool               isString;
        int flatIndex(const std::vector<int>& indices) const;
    };

    struct FnDef {
        std::string   param;
        ExpressionPtr body;
    };

    static ArrayData MakeArrayData(const std::string& name, const std::vector<int>& dims);
    ArrayData& ResolveArray(const std::string& name, size_t rank);
    void ValidateIndices(const std::string& name, const ArrayData& arr, const std::vector<int>& indices) const;

    std::unordered_map<std::string, Value>     m_variables;
    std::unordered_map<std::string, ArrayData> m_arrays;
    std::unordered_map<std::string, FnDef>     m_functions;

    DataManager m_dataManager;
    ProgramManager m_programManager;
};

} // namespace Basic
