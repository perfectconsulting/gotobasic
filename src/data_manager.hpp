/*
 *
 *    data_manager.hpp
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
#include <vector>
#include <string>
#include "value.hpp"

namespace Basic {

class Environment;

using DataValueVector = std::vector<Value>;
using DataLineMap   = std::map<int, DataValueVector>;

class DataManager {
public:
    DataManager(Environment& environment) : m_environment(environment) {}   

    void ClearData() { m_dataMap.clear(); m_currentDataMapit = m_dataMap.end(); m_currentDataVectorit = m_dataMap.end()->second.end(); }
    bool HasData() const { return !m_dataMap.empty(); }
    void AddData(int lineNumber, const std::vector<Value>& values);
    const Value ReadData(Environment& env);
    void RestoreData(int lineNumber = -1, bool bAllowErrors = false);
    std::string ListDataInRange(int first, int last) const;
private:
    DataLineMap m_dataMap;
    DataLineMap::iterator m_currentDataMapit = m_dataMap.end();
    DataValueVector::iterator m_currentDataVectorit = m_dataMap.end()->second.end();
    Environment& m_environment;
};

};