/*
 *
 *    program_manager.hpp
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

#include <map>
#include <stack>
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include "forward_references.hpp"
#include "runtime_ip.hpp"
#include "value.hpp"

namespace Basic {

using ProgramLineMap = std::map<int, StatementPtr>;

class Environment;

class ProgramManager {
public:
    ProgramManager(Environment& environment) : m_environment(environment) {}
    StatementPtr GetLastStatement() const { return m_lastStatement; }
    void SetLastStatement(StatementPtr stmt) { m_lastStatement = stmt; }
    void ClearLastStatement() { m_lastStatement = nullptr;}

    int  GetCurrentParseLineNumber() const { return m_currentParseLineNumber; }
    void SetCurrentParseLineNumber(int lineNumber) { m_currentParseLineNumber = lineNumber; ClearLastStatement(); }

    void AddStatementToCurrentLine(StatementPtr stmt);
    void DeleteLines(int first, int last);
    bool HasLine(int lineNumber) const;

    int GetRuntimeLineNumber() { return m_ip.GetLineNumber(); }
    std::string GetRuntimeLineNumberAsString() { return GetRuntimeLineNumber() > 0 ? std::to_string(GetRuntimeLineNumber()) : ""; }
    void ClearProgram();

    // Non-owning "jump to this statement next" used by IF/THEN: the
    // statement jumped to is already kept alive for the program's whole
    // lifetime by the IF statement's own captured StatementPtr, so the
    // weak_ptr here is for uniformity/safety rather than a live risk.
    StatementPtr GetRuntimeStatement() { return m_ip.GetStatement(); }
    void SetRuntimeStatement(const StatementPtr& stmt) { m_ip.SetStatement(stmt); m_proccessNextStatement = false; }

    void ProcessPendingLoadSave();
    void SetPendingLoad(const std::string& filename) { m_pendingLoad = filename; }
    void SetPendingSave(const std::string& filename) { m_pendingSave = filename; }

    void Load(const std::string& fileName);
    void Save(const std::string& fileName);
    void ListProgram(std::ostream& stream, int first = -1, int last = -1) const;
    void Run();
    void New();
    void Tron() { m_debug = true; }
    void Troff() { m_debug = false; }
    void Goto(int lineNumber);
    void Gosub(int lineNumber);
    void Return();
    void End();
    void Stop();

    void For(const std::string& variable, Value& start, Value& limit, Value& step);
    void Next(const std::string& variable);
    
    void While(const Value& condition, RuntimeIP& wendip);
    void Wend(RuntimeIP& whileip);

    // WHILE/WEND matching table. Populated at *parse* time (one entry per
    // WHILE, matched up with its WEND as the parser reaches it) and
    // consulted at run time by While()/Wend() above. This used to live as
    // file-scope `static` globals in statement_ops.hpp, which had two
    // problems: every translation unit that included the header got its
    // own separate copy, and the table was never cleared on NEW/LOAD, so
    // stale entries (with weak_ptrs into an already-replaced program)
    // piled up across every run. It now lives here, alongside the rest of
    // the program's parse/runtime state, and ClearProgram() resets it.
    struct WhileFrame {
        RuntimeIP m_whileip;
        RuntimeIP m_wendip;
    };

    int WhileFrameCount() const { return static_cast<int>(m_whileFrames.size()); }
    int RegisterWhileFrame(RuntimeIP whileip) {
        m_whileFrames.push_back(WhileFrame{std::move(whileip), RuntimeIP()});
        return static_cast<int>(m_whileFrames.size()) - 1;
    }
    WhileFrame& GetWhileFrame(int whileId) { return m_whileFrames.at(static_cast<size_t>(whileId)); }
    void PushWhileId(int whileId) { m_whileStack.push(whileId); }
    int  PopWhileId() { int id = m_whileStack.top(); m_whileStack.pop(); return id; }
    bool WhileStackEmpty() const { return m_whileStack.empty(); }

    void SetExitFlag(bool flag) { m_exit = flag; }
    bool GetExitFlag() const { return m_exit; }

private:
    struct ForFrame {
        ForFrame(const std::string& variable, const Value& step, const Value& limit, RuntimeIP& ip) : m_variable(variable), m_step(step), m_limit(limit), m_ip(ip) {}

        std::string m_variable;
        Value m_step;
        Value m_limit;
        RuntimeIP m_ip;
    };

    RuntimeIP m_ip;

    int m_currentParseLineNumber = -1;
    StatementPtr m_lastStatement;
    bool m_proccessNextStatement = true;

    ProgramLineMap m_programLines;
    Environment& m_environment;

    std::stack<RuntimeIP> m_gosubStack;
    std::stack<ForFrame> m_forStack;

    std::vector<WhileFrame> m_whileFrames;
    std::stack<int> m_whileStack;

    std::string m_pendingLoad; 
    std::string m_pendingSave;

    bool m_exit = false;
    bool m_debug = false;
};

} // namespace Basic
