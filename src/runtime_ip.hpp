/*
 *
 *    runtime_ip.hpp
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
 #include "forward_references.hpp"

 // RuntimeIP is an "instruction pointer": a (line number, statement) pair
// that can be copied onto the GOSUB/FOR/WHILE stacks and resumed later.
// The Statement it refers to is owned by ProgramManager's m_programLines
// (via the shared_ptr chain formed by Statement::SetNextStatement), so
// RuntimeIP only ever *observes* it. It holds that observation as a
// weak_ptr rather than a raw pointer: a saved RuntimeIP can easily outlive
// the statement it points to (e.g. a GOSUB is still on the stack when the
// user types NEW, or edits/deletes the very lines it was suspended in),
// and a weak_ptr turns what would otherwise be a dangling-pointer crash
// into a clean, detectable "this line no longer exists" condition via
// Valid()/GetStatement().

namespace Basic {

class RuntimeIP {
public:
    RuntimeIP() = default;
    RuntimeIP(int lineNumber, const StatementPtr& statement) : m_lineNumber(lineNumber), m_statement(statement) {}

    int  GetLineNumber() const { return m_lineNumber; }
    void SetLineNumber(int lineNumber) { m_lineNumber = lineNumber; }

    // Returns nullptr if the statement has since been deleted (program
    // edited, NEW, or a fresh LOAD) rather than dereferencing a dangling
    // pointer.
    StatementPtr GetStatement() const { return m_statement.lock(); }
    void SetStatement(const StatementPtr& stmt) { m_statement = stmt; }

    bool Valid(ProgramLineMap& programLines) const;
    bool Next(ProgramLineMap& programLines);
    bool Goto(ProgramLineMap& programLines, int lineNumber);
    void Debug(ProgramLineMap& programLines) const;

private:
    int m_lineNumber = -1;
    std::weak_ptr<Statement> m_statement;
};

} // namespace Basic
