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

#include <string>
#include <csignal>
#include "environment.hpp"
#include "parse_error.hpp"

extern int yyparse(Basic::Environment& environment);

typedef struct yy_buffer_state* YY_BUFFER_STATE;
extern YY_BUFFER_STATE yy_scan_string(const char* str);
extern void yy_switch_to_buffer(YY_BUFFER_STATE buf);
extern void yy_delete_buffer(YY_BUFFER_STATE buf);

extern std::string yyinput_line;
extern int yycolumn;
extern volatile sig_atomic_t g_break_requested;

static bool ParseLine(const std::string& line, Basic::Environment& env) {
    yyinput_line = line;

    // because the parse is bottom up, you only parse the line number when the rest of the AST is
    // already constrcuted. this is a problem for the while/wend fixups. the work-around is to 
    // extract and store the line number perfor teh line is parsed.
    
    size_t i = line.find_first_not_of(' ');
    int lineNumber = i != std::string::npos && std::isdigit(line[i]) ? std::stoi(line) : -1;

    env.GetProgramManager().SetCurrentParseLineNumber(lineNumber);
    yycolumn     = 1;
    bool result  = true;
    YY_BUFFER_STATE buf = yy_scan_string(line.c_str());
    yy_switch_to_buffer(buf);
    try {
        yyparse(env);
    } catch (ParseErrorReported&) {
        result = false;
    } catch (std::runtime_error& e) {
        std::cout << "Runtime error: " << e.what() << "\n";
        result = false;
    }
    yy_delete_buffer(buf);
    return result;
}