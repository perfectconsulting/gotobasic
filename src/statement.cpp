/*
 *
 *    statement.cpp
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

#include "statement.hpp"

namespace Basic {

Statement::Statement(std::vector<ExpressionPtr> argExprs, StatementFn action,
                      StmtListFn listFn)
    : m_argExprs(std::move(argExprs)), m_statementFn(std::move(action)),
      m_listFn(std::move(listFn)) {}

Statement::Statement(StatementFn action)
    : m_statementFn(std::move(action)) {}

void Statement::Execute(Environment& env) {
    std::vector<Value> args;
    
    // evalaute all the arguments
    args.reserve(m_argExprs.size());
    for (auto& argExpr : m_argExprs) {
        args.push_back(argExpr->Evaluate(env));
    }

    m_statementFn(args, env);
}

std::string Statement::List() const { 
    std::string s;
    if (m_listFn) s = m_listFn(m_argExprs);

    if (GetNextStatement()) {
        std::string nextList = GetNextStatement()->List();
        if (!nextList.empty()) {
            if (!s.empty()) s += " : ";
            s += nextList;
        }
    }
    return s;
}

} // namespace Basic
