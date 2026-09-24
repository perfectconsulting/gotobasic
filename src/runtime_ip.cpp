/*
 *
 *    runtime_ip.cpp
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

#include <cstdio>
#include <iostream>
#include "forward_references.hpp"
#include "statement.hpp"
#include "runtime_ip.hpp"

namespace Basic {

    bool RuntimeIP::Valid(ProgramLineMap& programLines) const
{
    ProgramLineMap::iterator it = programLines.find(m_lineNumber);
    return it != programLines.end() && !m_statement.expired();
}

bool RuntimeIP::Next(ProgramLineMap& programLines) {
    if (StatementPtr stmt = m_statement.lock()) {
        m_statement = stmt->GetNextStatement();
    } else {
        m_statement.reset();
    }

    if (m_statement.expired()) {
        ProgramLineMap::iterator it = programLines.find(m_lineNumber);
        if (it == programLines.end() || ++it == programLines.end()) {
            m_lineNumber = -1;
            m_statement.reset();
            return false;
        }
        m_lineNumber = it->first;
        m_statement = it->second;
    }

    return Valid(programLines);
}

bool RuntimeIP::Goto(ProgramLineMap& programLines, int lineNumber)
{
    if (lineNumber <= 0)
    {
        if (programLines.empty()) return false;
        lineNumber = programLines.begin()->first;
    }

    ProgramLineMap::iterator it = programLines.find(lineNumber);
    if (it == programLines.end())
    {
        return false;
    }

    m_lineNumber = it->first;
    m_statement = it->second;

    return Valid(programLines);
}

void RuntimeIP::Debug(ProgramLineMap& programLines) const {
    if (Valid(programLines)) {
        if (StatementPtr stmt = m_statement.lock()) {
            std::cout << m_lineNumber << ": " << stmt->List() << "\n";
        }
    }
}

} // namespace Basic