/*
 *
 *    statement.hpp
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
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "forward_references.hpp"
#include "expression.hpp"
#include "value.hpp"

namespace Basic {

class Environment;

using StatementFn    = std::function<void(const std::vector<Value>&, Environment&)>;
using StmtListFn     = std::function<std::string(const std::vector<ExpressionPtr>&)>;

// Statement defers Execute()/List() to a captured lambda rather than
// having a fresh class per kind of statement. m_argExprs are evaluated
// once per Execute() into a shared Value list, which is then passed to
// m_statementFn. Every statement in the language is built by a "Make" function
// that bakes the right lambda into a Statement; these live in
// statement_ops.hpp.

class Statement {
public:
    Statement(std::vector<ExpressionPtr> argExprs, StatementFn action,
              StmtListFn listFn);
    explicit Statement(StatementFn action);

    void        Execute(Environment& env);
    std::string List() const;

    void SetNextStatement(StatementPtr next) { m_nextStatement = std::move(next); }
    StatementPtr GetNextStatement() const { return m_nextStatement; }

    int lineNumber = -1;
private:
    std::vector<ExpressionPtr> m_argExprs;
    StatementFn                m_statementFn;
    StmtListFn                 m_listFn;
    StatementPtr               m_nextStatement = nullptr;
};

} // namespace Basic
