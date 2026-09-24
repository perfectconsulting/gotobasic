/*
 *
 *    data_manager.cpp
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

#include <iostream>
#include "data_manager.hpp"

namespace Basic {

void DataManager::AddData(int lineNumber, const std::vector<Value>& values) {
    m_dataMap[lineNumber] = std::move(values);
}

const Value DataManager::ReadData(Environment& env) {
    if (!HasData()) {
        throw RuntimeError(env, "READ: no DATA available");
    }

    if (m_currentDataVectorit == m_currentDataMapit->second.end()) {
        m_currentDataMapit++;
        if (m_currentDataMapit == m_dataMap.end()) {
            throw RuntimeError(env, "READ: out of data");
        }
        m_currentDataVectorit = m_currentDataMapit->second.begin();
    }
    return *m_currentDataVectorit++;
}

void  DataManager::RestoreData(int lineNumber, bool bAllowErrors){
    if (!HasData()) {
        DataLineMap::iterator m_currentDataMapit = m_dataMap.end();
        DataValueVector::iterator m_currentDataVectorit = m_dataMap.end()->second.end();
        return;
    };

    if (lineNumber == -1) {
        lineNumber = m_dataMap.begin()->first;
    }

    m_currentDataMapit = m_dataMap.find(lineNumber);
    if (m_currentDataMapit == m_dataMap.end()) {
        if (bAllowErrors) {
            throw RuntimeError(m_environment, "RESTORE: no DATA at or after line " + std::to_string(lineNumber));
        }
    }

    m_currentDataVectorit = m_currentDataMapit->second.begin();
}

std::string DataManager::ListDataInRange(int first, int last) const {
    std::string result;
    for (auto it = m_dataMap.lower_bound(first); it != m_dataMap.end() && it->first <= last; ++it) {
        result += std::to_string(it->first) + " DATA ";
        int n = it->second.size();
        for (const auto& value : it->second) {
            result += value.PPrint(true);
            result += (n-- > 1 ? ", " : "");
        }

        if (!result.empty())
        {
            result += "\n";
        }
    }

    return result;
}
} // namespace Basic