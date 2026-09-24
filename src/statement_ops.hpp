/*
 *
 *    statement_ops.hpp
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
#include <cstdlib>
#include <string>
#include <vector>
#include "ios"
#include "statement.hpp"
#include "value.hpp"
#include "error.hpp"
#include "expression_ops.hpp"
#include "program_manager.hpp"

namespace Basic::Ops {

// System Statements
// ----------------------------------------------------------------------------------------------------------

inline StatementPtr MakeList(int first = -1, int last = -1) {
    return std::make_shared<Statement>(
        [first, last](const std::vector<Value>& args, Environment& env) { env.GetProgramManager().ListProgram(std::cout, first, last);});
}

inline StatementPtr MakeRun() {
    return std::make_shared<Statement>(
        [](const std::vector<Value>& args, Environment& env) { env.GetProgramManager().Run(); });
}

inline StatementPtr MakeLoad(std::string filename) {
    return std::make_shared<Statement>(
        [filename](const std::vector<Value>& args, Environment& env) { env.GetProgramManager().SetPendingLoad(filename); });
}

inline StatementPtr MakeSave(std::string filename) {
    return std::make_shared<Statement>(
        [filename](const std::vector<Value>& args,Environment& env) { env.GetProgramManager().SetPendingSave(filename); });
}

inline StatementPtr MakeDelete(int first, int last) {
    return std::make_shared<Statement>(
        [first, last](const std::vector<Value>& args,Environment& env) { env.GetProgramManager().DeleteLines(first, last); });
}

inline StatementPtr MakeNew() {
    return std::make_shared<Statement>(
        [](const std::vector<Value>& args, Environment& env) { env.GetProgramManager().New(); });
}

inline StatementPtr MakeTron() {
    return std::make_shared<Statement>(
        [](const std::vector<Value>& args, Environment& env) { env.GetProgramManager().Tron(); });
}

inline StatementPtr MakeTroff() {
    return std::make_shared<Statement>(
        [](const std::vector<Value>& args, Environment& env) { env.GetProgramManager().Troff(); });
}

inline StatementPtr MakeClsStatement() {
    return std::make_shared<Statement>(std::vector<ExpressionPtr>{},
        [](const std::vector<Value>&, Environment&) {
#ifdef _WIN32
            std::system("cls");
#else
            std::system("clear");
#endif
        }, [](const std::vector<ExpressionPtr>&) { return "CLS"; });
}

inline StatementPtr MakeRestoreStatement(int lineNumber = -1) {
    return std::make_shared<Statement>(std::vector<ExpressionPtr>{},
        [lineNumber](const std::vector<Value>&, Environment& env) {
            env.GetDataManager().RestoreData(lineNumber, true);
        }, [lineNumber](const std::vector<ExpressionPtr>&) {
            return lineNumber < 0 ? "RESTORE" : "RESTORE " + std::to_string(lineNumber);
        });
}

inline StatementPtr MakeRandomizeStatement(ExpressionPtr seedExpr = nullptr) {
    bool hasSeed = (seedExpr != nullptr);
    std::vector<ExpressionPtr> argExprs;
    if (hasSeed) argExprs.push_back(std::move(seedExpr));

    return std::make_shared<Statement>(std::move(argExprs),
        [hasSeed](const std::vector<Value>& args, Environment& env) {
            if (hasSeed)
                env.Randomize(static_cast<long>(args[0].GetInteger()));
            else
                env.Randomize(-1);
        }, [hasSeed](const std::vector<ExpressionPtr>& args) {
            return hasSeed ? "RANDOMIZE " + args[0]->List() : "RANDOMIZE";
        });
}

inline StatementPtr MakeClearStatement() {
    return std::make_shared<Statement>(std::vector<ExpressionPtr>{},
        [](const std::vector<Value>&, Environment& env) { env.ClearVariables(); },
        [](const std::vector<ExpressionPtr>&) { return "CLEAR"; });
}

// Assignment Statements
// ----------------------------------------------------------------------------------------------------------

inline StatementPtr MakeLetStatement(LValue target, ExpressionPtr expr) {
    return std::make_shared<Statement>(std::vector<ExpressionPtr>{std::move(expr)},
        [target](const std::vector<Value>& args, Environment& env) {
            env.Assign(target, args[0]);
        }, [target](const std::vector<ExpressionPtr>& args) {
            return "LET " + target.List() + " = " + args[0]->List();
        });
}

// Declaration Statements
// ----------------------------------------------------------------------------------------------------------

struct ArrayDecl {
    std::string          name;
    std::vector<ExpressionPtr> dims;
};

inline StatementPtr MakeDimStatement(std::vector<ArrayDecl> decls) {
    return std::make_shared<Statement>(std::vector<ExpressionPtr>{},
        [decls](const std::vector<Value>&, Environment& env) {
            for (auto& decl : decls) {
                std::vector<int> dims;
                for (auto& dimExpr : decl.dims)
                    dims.push_back(dimExpr->Evaluate(env).GetInteger());
                env.DimArray(decl.name, dims);
            }
        }, [decls](const std::vector<ExpressionPtr>&) {
            std::string s = "DIM ";
            for (size_t i = 0; i < decls.size(); ++i) {
                if (i > 0) s += ", ";
                s += decls[i].name + "(";
                for (size_t j = 0; j < decls[i].dims.size(); ++j) {
                    if (j > 0) s += ", ";
                    s += decls[i].dims[j]->List();
                }
                s += ")";
            }
            return s;
        });
}

inline StatementPtr MakeDefStatement(std::string name, std::string param, ExpressionPtr body) {
    ExpressionPtr bodyForList = body;
    return std::make_shared<Statement>(std::vector<ExpressionPtr>{},
        [name, param, body](const std::vector<Value>&, Environment& env) {
            env.DefFunction(name, param, body);
        }, [name, param, bodyForList](const std::vector<ExpressionPtr>&) {
            return "DEF " + name + "(" + param + ") = " + bodyForList->List();
        });
}

// Flow Statements
// ----------------------------------------------------------------------------------------------------------

inline StatementPtr MakeIfStatement(ExpressionPtr condition, StatementPtr thenBranch, StatementPtr elseBranch = nullptr) {
    return std::make_shared<Statement>(std::vector<ExpressionPtr>{std::move(condition)},
        [thenBranch, elseBranch](const std::vector<Value>& args, Environment& env) {
            if (args[0].GetTruthy())
                env.GetProgramManager().SetRuntimeStatement(thenBranch);
            else if (elseBranch)
                env.GetProgramManager().SetRuntimeStatement(elseBranch);
        }, [thenBranch, elseBranch](const std::vector<ExpressionPtr>& args) {
            std::string s = "IF " + args[0]->List() + " THEN " + thenBranch->List();
            if (elseBranch) s += " ELSE " + elseBranch->List();
            return s;
        });
}

inline StatementPtr MakeGotoStatement(int lineNumber) {
    return std::make_shared<Statement>(std::vector<ExpressionPtr>{},
        [lineNumber](const std::vector<Value>&, Environment& env) {
            env.GetProgramManager().Goto(lineNumber);
        }, [lineNumber](const std::vector<ExpressionPtr>&) {
            return "GOTO " + std::to_string(lineNumber);
        });
}

inline StatementPtr MakeGosubStatement(int lineNumber) {
    return std::make_shared<Statement>(std::vector<ExpressionPtr>{},
        [lineNumber](const std::vector<Value>&, Environment& env) {
            env.GetProgramManager().Gosub(lineNumber);
        }, [lineNumber](const std::vector<ExpressionPtr>&) {
            return "GOSUB " + std::to_string(lineNumber);
        });
}

inline StatementPtr MakeOnGotoStatement(ExpressionPtr expr, std::vector<int> lines) {
    return std::make_shared<Statement>(std::vector<ExpressionPtr>{std::move(expr)},
        [lines](const std::vector<Value>& args, Environment& env) {
            Integer idx = args[0].GetInteger() - 1;
            if (idx >= 0 && idx < static_cast<Integer>(lines.size()))
                env.GetProgramManager().Goto(lines[idx]);
        }, [lines](const std::vector<ExpressionPtr>& args) {
            std::string s = "ON " + args[0]->List() + " GOTO ";
            for (size_t i = 0; i < lines.size(); ++i) {
                if (i > 0) s += ", ";
                s += std::to_string(lines[i]);
            }
            return s;
        });
}

inline StatementPtr MakeOnGosubStatement(ExpressionPtr expr, std::vector<int> lines) {
    return std::make_shared<Statement>(std::vector<ExpressionPtr>{std::move(expr)},
        [lines](const std::vector<Value>& args, Environment& env) {
            Integer idx = args[0].GetInteger() - 1;
            if (idx >= 0 && idx < static_cast<Integer>(lines.size()))
                env.GetProgramManager().Gosub(lines[idx]);
        }, [lines](const std::vector<ExpressionPtr>& args) {
            std::string s = "ON " + args[0]->List() + " GOSUB ";
            for (size_t i = 0; i < lines.size(); ++i) {
                if (i > 0) s += ", ";
                s += std::to_string(lines[i]);
            }
            return s;
        });
}

inline StatementPtr MakeReturnStatement() {
    return std::make_shared<Statement>(std::vector<ExpressionPtr>{},
        [](const std::vector<Value>&, Environment& env) { env.GetProgramManager().Return(); },
        [](const std::vector<ExpressionPtr>&) { return "RETURN"; });
}

// Uses double throughout so fractional STEP values work correctly.
inline StatementPtr MakeForStatement(LValue target, ExpressionPtr start, ExpressionPtr limit, ExpressionPtr step = nullptr) {
    bool hasStep = (step != nullptr);
    std::vector<ExpressionPtr> argExprs{std::move(start), std::move(limit)};
    if (hasStep) argExprs.push_back(std::move(step));

    return std::make_shared<Statement>(std::move(argExprs),
        [target, hasStep](const std::vector<Value>& args, Environment& env) {
            Value startVal = args[0];
            Value limitVal = args[1];
            Value stepVal  = hasStep ? args[2] : Value(1);

            env.GetProgramManager().For(target.name, startVal, limitVal, stepVal);
        }, [target, hasStep](const std::vector<ExpressionPtr>& args) {
            std::string s = "FOR " + target.List() + " = " + args[0]->List() + " TO " + args[1]->List();
            if (hasStep) s += " STEP " + args[2]->List();
            return s;
        });
}

inline StatementPtr MakeNextStatement(std::string variable = "") {
    return std::make_shared<Statement>(std::vector<ExpressionPtr>{},
        [variable](const std::vector<Value>&, Environment& env) {
            env.GetProgramManager().Next(variable);
        }, [variable](const std::vector<ExpressionPtr>&) {
            return variable.empty() ? "NEXT" : "NEXT " + variable;
        });
}

inline StatementPtr MakeWhileStatement(ExpressionPtr condition, Environment& env) {
    // The id this WHILE will be registered under is just the table's
    // current size (we're about to append to it) - computing it up front,
    // before building the statement, lets the action capture it directly.
    int whileId = env.GetProgramManager().WhileFrameCount();

    StatementPtr whileStatement = std::make_shared<Statement>(std::vector<ExpressionPtr>{std::move(condition)},
        [whileId](const std::vector<Value>& args, Environment& env) {
            env.GetProgramManager().While(args[0], env.GetProgramManager().GetWhileFrame(whileId).m_wendip);
        }, [](const std::vector<ExpressionPtr>& args) {
            return "WHILE " + args[0]->List();
        });

    int lineNumber = env.GetProgramManager().GetCurrentParseLineNumber();
    env.GetProgramManager().RegisterWhileFrame(RuntimeIP(lineNumber, whileStatement));
    env.GetProgramManager().PushWhileId(whileId);
    return whileStatement;
}

inline StatementPtr MakeWendStatement(Environment& env) {
    if (env.GetProgramManager().WhileStackEmpty()) {
        throw std::runtime_error("WEND without matching WHILE");
    }

    int whileId = env.GetProgramManager().PopWhileId();

    StatementPtr wendStatement = std::make_shared<Statement>(std::vector<ExpressionPtr>{},
        [whileId](const std::vector<Value>&, Environment& env) {
            env.GetProgramManager().Wend(env.GetProgramManager().GetWhileFrame(whileId).m_whileip);
        }, [](const std::vector<ExpressionPtr>&) { return "WEND"; });

    int lineNumber = env.GetProgramManager().GetCurrentParseLineNumber();
    env.GetProgramManager().GetWhileFrame(whileId).m_wendip = RuntimeIP(lineNumber, wendStatement);

    return wendStatement;
}

inline StatementPtr MakeEndStatement() {
    return std::make_shared<Statement>(std::vector<ExpressionPtr>{},
        [](const std::vector<Value>&, Environment& env) { env.GetProgramManager().End(); },
        [](const std::vector<ExpressionPtr>&) { return "END"; });
}

inline StatementPtr MakeStopStatement() {
    return std::make_shared<Statement>(std::vector<ExpressionPtr>{},
        [](const std::vector<Value>&, Environment& env) { env.GetProgramManager().Stop(); },
        [](const std::vector<ExpressionPtr>&) { return "STOP"; });
}

// EXIT triggers a clean shutdown through End() rather than std::exit().
inline StatementPtr MakeExitStatement() {
    return std::make_shared<Statement>(std::vector<ExpressionPtr>{},
        [](const std::vector<Value>&, Environment& env) { env.GetProgramManager().SetExitFlag(true); },
        [](const std::vector<ExpressionPtr>&) { return "EXIT"; });
}

inline StatementPtr MakeRemStatement(std::string text = "") {
    return std::make_shared<Statement>(std::vector<ExpressionPtr>{},
        [](const std::vector<Value>&, Environment&) {},
        [text](const std::vector<ExpressionPtr>&) { return "REM" + text; });
}

// Data Statements
// ----------------------------------------------------------------------------------------------------------

inline StatementPtr MakeRead(std::vector<LValue> targets) {
    return std::make_shared<Statement>(std::vector<ExpressionPtr>{},
        [targets](const std::vector<Value>&, Environment& env) {
            for (auto& lv : targets) {
                Value val = env.GetDataManager().ReadData(env);
                val.NormaliseType(lv.isString());
                env.Assign(lv, val);
            }
        }, [targets](const std::vector<ExpressionPtr>&) {
            std::string s = "READ ";
            for (size_t i = 0; i < targets.size(); ++i) {
                if (i > 0) s += ", ";
                s += targets[i].List();
            }
            return s;
        });
}

// I/O Statements
// ----------------------------------------------------------------------------------------------------------

struct PrintItem {
    ExpressionPtr expr;
    bool    semicolon;
    bool    comma;
    bool    tab;
};

inline StatementPtr MakePrintStatement(std::vector<PrintItem> items, bool newline) {
    std::vector<ExpressionPtr> argExprs;
    argExprs.reserve(items.size());
    for (auto& item : items)
        argExprs.push_back(item.expr);

    return std::make_shared<Statement>(std::move(argExprs),
        [items, newline](const std::vector<Value>& args, Environment&) {
            int col = 0;
            for (size_t i = 0; i < items.size(); ++i) {
                const auto& item = items[i];
                if (item.tab) {
                    int target = args[i].GetInteger();
                    while (col < target - 1) { std::cout << ' '; ++col; }
                } else {
                    const Value& val = args[i];
                    std::string s = val.PPrint();
                    if (val.IsNumber()) {
                        bool negative = (!s.empty() && s[0] == '-');
                        if (!negative) { std::cout << ' '; ++col; }
                        std::cout << s;
                        col += static_cast<int>(s.size());
                        std::cout << ' ';
                        ++col;
                    } else {
                        std::cout << s;
                        col += static_cast<int>(s.size());
                    }
                    if (item.comma) {
                        int next = ((col / 15) + 1) * 15;
                        while (col < next) { std::cout << ' '; ++col; }
                    }
                }
            }
            if (newline) std::cout << '\n';
        }, [items, newline](const std::vector<ExpressionPtr>& args) {
            std::string s = "PRINT";
            for (size_t i = 0; i < items.size(); ++i) {
                const auto& item = items[i];
                if (item.tab) s += " TAB(" + args[i]->List() + ")";
                else          s += " " + args[i]->List();
                if (item.semicolon) s += ";";
                if (item.comma)     s += ",";
            }
            if (!newline && items.empty()) s += ";";
            return s;
        });
}

inline StatementPtr MakeInputStatement(std::string prompt, std::vector<LValue> targets) {
    return std::make_shared<Statement>(std::vector<ExpressionPtr>{},
        [prompt, targets](const std::vector<Value>&, Environment& env) {
            std::cout << prompt << std::flush;
            std::string line;
            if (!std::getline(std::cin, line))
                throw RuntimeError(env, "INPUT: unexpected end of input");

            std::vector<std::string> parts;
            std::string cur;
            for (char c : line) {
                if (c == ',') { parts.push_back(cur); cur.clear(); }
                else cur += c;
            }
            parts.push_back(cur);

            for (size_t i = 0; i < targets.size(); ++i) {
                std::string s = i < parts.size() ? parts[i] : "";
                while (!s.empty() && s.front() == ' ') s.erase(s.begin());
                while (!s.empty() && s.back()  == ' ') s.pop_back();

                Value val(s);
                val.NormaliseType(targets[i].isString());
                env.Assign(targets[i], val);
            }
        }, [prompt, targets](const std::vector<ExpressionPtr>&) {
            std::string s = "INPUT";
            if (prompt != "? ")
                s += " \"" + prompt + "\",";
            for (size_t i = 0; i < targets.size(); ++i)
                s += (i == 0 ? " " : ", ") + targets[i].List();
            return s;
        });
}

} // namespace Basic::Ops
