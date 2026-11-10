/*
 *
 *    assign_target.hpp
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
#include <string>
#include <vector>
#include "forward_references.hpp"

namespace Basic {

// The target of an assignment-like operation: a variable name plus optional
// index expressions, e.g. A, A$, or A(I, J+1). Used by LET, INPUT, READ and
// FOR. This is a purely syntactic reference; the index expressions are only
// evaluated (and the name resolved to real storage) when it is passed to
// Environment::Assign().
//
// List() is defined in expression.cpp, where Expression is a complete type.
struct AssignTarget {
    std::string                name;
    std::vector<ExpressionPtr> indices;  // empty -> scalar

    bool        isString() const { return !name.empty() && name.back() == '$'; }
    std::string List() const;
};

} // namespace Basic
