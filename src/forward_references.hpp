/*
 *
 *    forward_reference.hpp
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
#include <memory>
#include <map>

// Forward declarations and pointer-type aliases shared across the AST
// headers (expression.hpp, statement.hpp, value.hpp, environment.hpp,
// program_manager.hpp). Previously each of those headers redeclared these
// independently, which meant five places to keep in sync if the underlying
// pointer type ever changed. Include this instead.

namespace Basic {

class Expression;
class Statement;

using ExpressionPtr = std::shared_ptr<Expression>;
using StatementPtr  = std::shared_ptr<Statement>;
using ProgramLineMap = std::map<int, StatementPtr>;

} // namespace Basic
