/*
 *
 *    main.cpp
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
#include <string>
#include <stdexcept>
#include <cctype>
#include <csignal>
#include "environment.hpp"
#include "error.hpp"
#include "version.h"
#include "flex_interface.h"

volatile sig_atomic_t g_break_requested = 0;

static void onSigInt(int) { g_break_requested = 1; }

#include "parse_error.hpp"

int main(int argc, char* argv[]) {
    std::cout << "GOTO BASIC Interpreter " << GOTOBASIC_VERSION_STRING << "\n";
    std::cout << "An implementation of extended Dartmouth BASIC using C++, FLEX and BISON\n";
    std::cout << "Written by Steven James\n\n";

    std::signal(SIGINT, onSigInt);

    Basic::Environment env;

    if (argc > 1) {
        env.GetProgramManager().Load(argv[1]);
        env.GetProgramManager().Run();
        return 0;
    }

    std::string line;
    while (!env.GetProgramManager().GetExitFlag()) {
        std::cout << ">" << std::flush;
        if (!std::getline(std::cin, line)) break;
        line += "\n";
        ParseLine(line, env);
        env.GetProgramManager().ProcessPendingLoadSave();
        std::cout << "OK\n";
    }
    return 0;
}
