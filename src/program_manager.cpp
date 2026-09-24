/*
 *
 *    program_manager.cpp
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
#include <fstream>
#include <limits>
#include <ctime>
#include <format>
#include <memory>

#include "program_manager.hpp"
#include "statement.hpp"
#include "environment.hpp"
#include "flex_interface.h"

#ifdef _WIN32
    #include <conio.h>
#else
    #include <termios.h>
    #include <unistd.h>
#endif

namespace Basic {

int WaitForKey()
{
    int key = -1;
#ifdef _WIN32
    key = _getch();
#else
    termios oldt, newt;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);

    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    key = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif

    return key;
}

void ProgramManager::AddStatementToCurrentLine(StatementPtr stmt) {
    if (m_currentParseLineNumber == -1) {
        throw RuntimeError(m_environment, "No current line number set");
    }

    if(GetLastStatement()) {
        GetLastStatement()->SetNextStatement(stmt);
    } else {
        m_programLines[m_currentParseLineNumber] = stmt;
    }
    SetLastStatement(stmt);
}

void ProgramManager::DeleteLines(int first, int last) {
    m_programLines.erase(m_programLines.lower_bound(first), m_programLines.upper_bound(last));
}

bool ProgramManager::HasLine(int lineNumber) const {
    return m_programLines.find(lineNumber) != m_programLines.end();
}

void ProgramManager::ClearProgram() {
   m_programLines.clear();

   while (!m_whileStack.empty()) m_whileStack.pop();
   m_whileFrames.clear();   
}

void ProgramManager::ProcessPendingLoadSave() {
    if (!m_pendingLoad.empty()) {
        Load(m_pendingLoad);
        m_pendingLoad.clear();
    }
    if (!m_pendingSave.empty()) {
        Save(m_pendingSave);
        m_pendingSave.clear();
    }
}

void ProgramManager::Load(const std::string& fileName) {
    FILE* file = std::fopen(fileName.c_str(), "r");
    if (!file) { 
        std::cout << "LOAD: cannot open file '" << fileName << "'\n"; return;
    }

    ClearProgram();
    m_environment.GetDataManager().ClearData();
    
    char buf[4096];
    bool result = true;

    while (std::fgets(buf, sizeof(buf), file)) {
        std::string line(buf);
        if (line.empty() || line == "\n") continue;
        if (line.back() != '\n') line += '\n';

        result = result && ParseLine(line, m_environment);

        if (!result) break;
    }
    std::fclose(file);

    if (!result) {
        std::cout << "Loaded " << fileName << " with errors\n";
    }

}

void ProgramManager::Save(const std::string& fileName) {
    std::ofstream file;
    file.open(fileName);
    ListProgram(file);
    file.close();
    std::cout << "Saved to " << fileName << "\n";
}

void ProgramManager::ListProgram(std::ostream& stream, int first, int last) const {
    int lastLineNum = -1;
    for (const auto& [lineNum, stmt] : m_programLines) {
        if (first != -1 && lineNum < first) continue;
        if (last  != -1 && lineNum > last)  break;

        std::string dataLines = m_environment.GetDataManager().ListDataInRange(lastLineNum, lineNum);
        if (!dataLines.empty()) {
            stream << dataLines;
        }

        stream << lineNum << " " << stmt->List() << "\n";
        lastLineNum = lineNum;
    }

    std::string dataLines = m_environment.GetDataManager().ListDataInRange(lastLineNum, std::numeric_limits<int>::max());
    if (!dataLines.empty()) {
        stream << dataLines;
    }    
}

void ProgramManager::Run() {
    g_break_requested = 0;
    std::srand(std::time({}));
    m_environment.ResetVariablesAndArrays();
    m_environment.GetDataManager().RestoreData(-1, false);
    
    while(!m_gosubStack.empty()) {
        m_gosubStack.pop();
    }

    while(!m_forStack.empty()) {
        m_forStack.pop();
    }

    m_ip.Goto(m_programLines, 0);

    try {

        while (m_ip.Valid(m_programLines)) {
            if (g_break_requested) {
                g_break_requested = 0;
                throw RuntimeError(m_environment, "BREAK");
            }

            StatementPtr currentStatement = m_ip.GetStatement();
            if (!currentStatement) {
                throw RuntimeError(m_environment, "Internal error: current statement no longer exists");
            }

            if(m_debug) {
                m_ip.Debug(m_programLines);
                WaitForKey();
            }            

            currentStatement->Execute(m_environment);


            if(m_proccessNextStatement)
            {
                m_ip.Next(m_programLines);
            }

            m_proccessNextStatement = true;
        }
    } catch (RuntimeError& e) {      
        std::cout << "Runtime error: " << e.what() << "\n";
    }
}

void ProgramManager::New() {
    ClearProgram();
    m_environment.GetDataManager().ClearData();
}

void ProgramManager::Goto(int lineNumber) {
    if(!m_ip.Goto(m_programLines,lineNumber)) {
        throw RuntimeError(m_environment, "GOTO: invalid line number");
    }
    m_proccessNextStatement = false;
}

void ProgramManager::Gosub(int lineNumber)
{
    m_gosubStack.push(m_ip);

    if (!m_ip.Goto(m_programLines, lineNumber))
    {
        throw RuntimeError(m_environment, "GOSUB: invalid line number");
    }

    m_proccessNextStatement = false; 
}

void ProgramManager::Return() {
    if (m_gosubStack.empty()) {
        throw RuntimeError(m_environment, "RETURN: return stack underflow"); 
    }

    RuntimeIP ip = m_gosubStack.top();
    m_gosubStack.pop();

    m_ip = ip;
}

void ProgramManager::End() {
    m_ip.SetLineNumber(0);
    m_ip.SetStatement(nullptr);
    m_proccessNextStatement = false;
}

void ProgramManager::Stop() {
    std::cout << "STOP at line " << GetRuntimeLineNumberAsString() << "\n";
    End();
}

void ProgramManager::For(const std::string& variable, Value& start, Value& limit, Value& step) {
    if (!start.IsNumber() || !limit.IsNumber() || !step.IsNumber()) {
        throw RuntimeError(m_environment, "FOR: expected numeric values");
    }

    if (step.IsZero()){
        throw RuntimeError(m_environment, "FOR: step cannot be zero");
    }

    if (start.IsReal() || limit.IsReal() || step.IsReal()) {
        start.SetReal(start.GetReal());
        limit.SetReal(limit.GetReal());
        step.SetReal(step.GetReal());
    }
    else {
        start.SetInteger(start.GetInteger());
        limit.SetInteger(limit.GetInteger());
        step.SetInteger(step.GetInteger());
    }

    m_environment.SetVariable(variable, start);

    RuntimeIP ip = m_ip;
    ip.Next(m_programLines);
    m_forStack.push(ForFrame(variable, step, limit, ip));
}

void ProgramManager::Next(const std::string& variable) {
    if (m_forStack.empty()) {
        throw RuntimeError(m_environment, "NEXT without FOR");
    }

    ForFrame frame = m_forStack.top();
    if (!variable.empty() && frame.m_variable != variable) {
        throw RuntimeError(m_environment, "NEXT variable does not match FOR variable");
    }

    Value currentValue = m_environment.GetVariable(frame.m_variable);
    Value newValue = currentValue.AddOp(frame.m_step, m_environment);
    m_environment.SetVariable(frame.m_variable, newValue);

    bool loopConditionMet = false;
    if (frame.m_step.IsPositive()) {
        loopConditionMet = newValue.CompareOp(frame.m_limit, m_environment) <= 0;
    } else {
        loopConditionMet = newValue.CompareOp(frame.m_limit, m_environment) >= 0;
    }

    if (loopConditionMet) {
        m_ip = frame.m_ip;
        m_proccessNextStatement = false;
    } else {
        m_forStack.pop();
    }
}

void ProgramManager::While(const Value& condition, RuntimeIP& wendip) {
    if(!condition.GetTruthy()) {
        if (! wendip.Valid(m_programLines)) {
            throw RuntimeError(m_environment, "WHILE: matching WEND not found");
        }

        m_ip = wendip;
        m_proccessNextStatement = true;
   }
}

void ProgramManager::Wend(RuntimeIP& whileip) {
    if (!whileip.Valid(m_programLines)) {
        throw RuntimeError(m_environment, "WEND: matching WHILE not found");
    }

    m_ip = whileip;
    m_proccessNextStatement = false;
}

} // namespace Basic