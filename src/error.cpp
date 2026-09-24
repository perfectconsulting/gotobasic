/*
 *
 *    error.cpp
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

#include "error.hpp"
#include "environment.hpp"

namespace Basic {

RuntimeError::RuntimeError(Environment& env, std::string message)
    : std::runtime_error(message) {
    std::string line = env.GetProgramManager().GetRuntimeLineNumberAsString();
    m_fullMessage = line.empty() ? message : message + " at line " + line;
}

const char* RuntimeError::what() const noexcept {
    return m_fullMessage.c_str();
}

}
