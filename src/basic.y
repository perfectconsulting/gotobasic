%code requires {
    #include <vector>
    #include <string>
    #include <memory>
    #include "expression.hpp"
    #include "statement.hpp"
    #include "value.hpp"
    #include "environment.hpp"

    namespace Basic {

    // One array declared by a DIM statement: its name and the expressions
    // giving the size of each dimension.
    struct ArrayDecl {
        std::string                name;
        std::vector<ExpressionPtr> dims;
    };

    // One item in a PRINT list, plus the separator that follows it.
    struct PrintItem {
        ExpressionPtr expr;
        bool          semicolon;
        bool          comma;
        bool          tab;
    };

    // A complete PRINT argument list, and whether PRINT ends with a newline
    // (it doesn't when the list ends in ';' or ',').
    struct PrintArgs {
        std::vector<PrintItem> items;
        bool                   newline;
    };

    } // namespace Basic
}

%{
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <stdexcept>
#include "environment.hpp"
#include "error.hpp"
#include "expression.hpp"
#include "program_manager.hpp"
#include "runtime_ip.hpp"
#include "statement.hpp"

using namespace Basic;

inline Basic::StatementPtr* new_stmt(Basic::StatementPtr p) { return new Basic::StatementPtr(std::move(p)); }
inline Basic::ExpressionPtr* new_expr(Basic::ExpressionPtr p) { return new Basic::ExpressionPtr(std::move(p)); }
inline Basic::StatementPtr take_stmt(Basic::StatementPtr* p) { Basic::StatementPtr s = std::move(*p); delete p; return s; }
inline Basic::ExpressionPtr take_expr(Basic::ExpressionPtr* p) { Basic::ExpressionPtr e = std::move(*p); delete p; return e; }

extern int yylex(void);
void yyerror(Basic::Environment& env, const char* s);

extern std::string yyinput_line;
extern int yycolumn;

struct ParseErrorReported {};
%}

%union {
    int         ival;
    float       dval;
    std::string* sval;
    Basic::ExpressionPtr*  expr;
    Basic::StatementPtr*  stmt;
    std::vector<Basic::ExpressionPtr>*  exprlist;
    std::vector<int>* intlist;
    std::vector<Basic::ArrayDecl>*  dimlist;
    std::vector<Basic::PrintItem>* printitems;
    Basic::PrintArgs*  printargs;
    Basic::LValue*  lval;
    std::vector<Basic::LValue>*  lvallist;
    std::vector<Basic::Value>*  vallist;
}

%parse-param { Basic::Environment& env }

%token <ival> INTEGER
%token <dval> FLOAT
%token <sval> STRING IDENTIFIER STRIDENTIFIER FNIDENTIFIER

%token KW_LET KW_PRINT KW_INPUT KW_IF KW_THEN KW_ELSE
%token KW_GOTO KW_GOSUB KW_RETURN KW_FOR KW_TO KW_STEP
%token KW_NEXT KW_END KW_STOP KW_AND KW_OR KW_NOT
%token KW_LIST KW_RUN KW_EXIT KW_LOAD KW_SAVE KW_DELETE KW_TRON KW_TROFF
%token KW_DIM KW_DEF KW_ON
%token KW_DATA KW_READ KW_RESTORE KW_RANDOMIZE KW_CLEAR KW_NEW KW_CLS KW_WHILE KW_WEND
%token <sval> KW_REM
%token NEQ LEQ GEQ
%token FN_ABS FN_ATN FN_COS FN_EXP FN_INT FN_LOG FN_RND FN_SGN FN_SIN FN_SQR FN_TAN
%token FN_LEN FN_ASC FN_CHR FN_STR FN_VAL FN_LEFT FN_RIGHT FN_MID FN_TAB FN_SPC

%type <expr>       expr
%type <stmt>       stmt statement command stmtlist
%type <exprlist>   exprlist dimexprlist
%type <intlist>    intlist
%type <dimlist>    dimlist
%type <printitems> printlist
%type <printargs>  printargs
%type <sval>       inputprompt
%type <lval>       lval
%type <lvallist>   lvallist
%type <vallist>    constlist

%left KW_OR
%left KW_AND
%right KW_NOT
%left '=' NEQ '<' '>' LEQ GEQ
%left '+' '-'
%left '*' '/'
%right '^'
%right UMINUS

/* The 3 remaining shift/reduce conflicts are the classic "dangling else"
 * family, arising because an IF's THEN/ELSE clause is a colon-separated
 * stmtlist rather than a single statement:
 *   - IF a THEN x . ':' y          -> does ':' extend the IF's own body?
 *   - IF a THEN (IF b THEN x) . ELSE y  -> which IF does ELSE bind to?
 *   - same ':' ambiguity after the ELSE branch
 * Bison's default (shift) resolves all three the conventional way: colon
 * continues extending the innermost clause, and ELSE binds to the nearest
 * unmatched IF. This is intentional; %expect documents the count so a
 * genuine new conflict introduced later still gets flagged. */
%expect 3

%%
input
    : line
    ;

line
    : '\n'
    | INTEGER stmtlist '\n' {
        env.GetProgramManager().DeleteLines((int)$1, (int)$1);
        env.GetProgramManager().AddStatementToCurrentLine(take_stmt($2));
    }
    | command '\n' {
        take_stmt($1)->Execute(env);
    }
    | stmt '\n' {
        take_stmt($1)->Execute(env);
    }
    | INTEGER KW_DATA constlist '\n' {
        env.GetDataManager().AddData((int)$1, *$3);
        delete $3;
    }    
    ;

stmtlist
    : stmt { $$ = $1; }
    | stmtlist ':' stmt {
        Basic::StatementPtr tail = *$1;
        while (tail->GetNextStatement())
            tail = tail->GetNextStatement();
        tail->SetNextStatement(take_stmt($3));
        $$ = $1;
    }
    ;

stmt
    : statement { $$ = $1; }
    ;

/* ── Interactive commands (not stored in numbered programs) ─────────────── */
/* Each action builds its Statement directly: a lambda bakes in what to do
 * when the command executes. Commands have no list-lambda, so List() on
 * them yields an empty string. */
command
    : KW_LIST {
        $$ = new_stmt(std::make_shared<Statement>(
            [](const std::vector<Value>& args, Environment& env) {
                env.GetProgramManager().ListProgram(std::cout, -1, -1);
            }));
    }
    | KW_LIST INTEGER ',' INTEGER {
        int first = $2, last = $4;
        $$ = new_stmt(std::make_shared<Statement>(
            [first, last](const std::vector<Value>& args, Environment& env) {
                env.GetProgramManager().ListProgram(std::cout, first, last);
            }));
    }
    | KW_RUN {
        $$ = new_stmt(std::make_shared<Statement>(
            [](const std::vector<Value>& args, Environment& env) {
                env.GetProgramManager().Run();
            }));
    }
    | KW_NEW {
        $$ = new_stmt(std::make_shared<Statement>(
            [](const std::vector<Value>& args, Environment& env) {
                env.GetProgramManager().New();
            }));
    }
    | KW_DELETE INTEGER {
        int first = $2, last = $2;
        $$ = new_stmt(std::make_shared<Statement>(
            [first, last](const std::vector<Value>& args, Environment& env) {
                env.GetProgramManager().DeleteLines(first, last);
            }));
    }
    | KW_DELETE INTEGER ',' INTEGER {
        int first = $2, last = $4;
        $$ = new_stmt(std::make_shared<Statement>(
            [first, last](const std::vector<Value>& args, Environment& env) {
                env.GetProgramManager().DeleteLines(first, last);
            }));
    }
    | KW_LOAD STRING {
        std::string filename = *$2;
        delete $2;
        $$ = new_stmt(std::make_shared<Statement>(
            [filename](const std::vector<Value>& args, Environment& env) {
                env.GetProgramManager().SetPendingLoad(filename);
            }));
    }
    | KW_SAVE STRING {
        std::string filename = *$2;
        delete $2;
        $$ = new_stmt(std::make_shared<Statement>(
            [filename](const std::vector<Value>& args, Environment& env) {
                env.GetProgramManager().SetPendingSave(filename);
            }));
    }
    | KW_TRON {
        $$ = new_stmt(std::make_shared<Statement>(
            [](const std::vector<Value>& args, Environment& env) {
                env.GetProgramManager().Tron();
            }));
    }
    | KW_TROFF {
        $$ = new_stmt(std::make_shared<Statement>(
            [](const std::vector<Value>& args, Environment& env) {
                env.GetProgramManager().Troff();
            }));
    }
    ;

/* ── Unified l-value: scalar variable or subscripted array ──────────────── */
lval
    : IDENTIFIER {
        $$ = new Basic::LValue{*$1, {}};
        delete $1;
    }
    | STRIDENTIFIER {
        $$ = new Basic::LValue{*$1, {}};
        delete $1;
    }
    | IDENTIFIER '(' exprlist ')' {
        $$ = new Basic::LValue{*$1, std::move(*$3)};
        delete $1; delete $3;
    }
    | STRIDENTIFIER '(' exprlist ')' {
        $$ = new Basic::LValue{*$1, std::move(*$3)};
        delete $1; delete $3;
    }
    ;

/* Comma-separated list of l-values (INPUT, READ) */
lvallist
    : lval {
        $$ = new std::vector<Basic::LValue>();
        $$->push_back(std::move(*$1));
        delete $1;
    }
    | lvallist ',' lval {
        $1->push_back(std::move(*$3));
        delete $3;
        $$ = $1;
    }
    ;

/* ── Constant r-values (literals only, used by DATA) ────────────────────── */
/* Negative literals are handled here so DATA -1 works without the
   expression grammar introducing shift/reduce ambiguity. */
constlist
    : INTEGER {
        $$ = new std::vector<Basic::Value>();
        $$->push_back(Basic::Value((Real)$1));
    }
    | FLOAT {
        $$ = new std::vector<Basic::Value>();
        $$->push_back(Basic::Value($1));
    }
    | STRING {
        $$ = new std::vector<Basic::Value>();
        $$->push_back(Basic::Value(*$1));
        delete $1;
    }
    | '-' INTEGER {
        $$ = new std::vector<Basic::Value>();
        $$->push_back(Basic::Value(-(Integer)$2));
    }
    | '-' FLOAT {
        $$ = new std::vector<Basic::Value>();
        $$->push_back(Basic::Value(-$2));
    }
    | constlist ',' INTEGER {
        $1->push_back(Basic::Value((Integer)$3));
        $$ = $1;
    }
    | constlist ',' FLOAT {
        $1->push_back(Basic::Value($3));
        $$ = $1;
    }
    | constlist ',' STRING {
        $1->push_back(Basic::Value(*$3));
        delete $3;
        $$ = $1;
    }
    | constlist ',' '-' INTEGER {
        $1->push_back(Basic::Value(-(Integer)$4));
        $$ = $1;
    }
    | constlist ',' '-' FLOAT {
        $1->push_back(Basic::Value(-$4));
        $$ = $1;
    }
    ;

/* ── Statements ─────────────────────────────────────────────────────────── */
/* Each action builds its Statement directly. A Statement is made of:
 *   - the argument expressions, evaluated each time it executes,
 *   - a lambda that does the work, given those evaluated arguments, and
 *   - a lambda that renders it back to text for LIST.
 * Anything fixed at parse time (line numbers, names, branches) is captured
 * by value into the lambdas. */
statement
    /* LET with optional keyword */
    : KW_LET lval '=' expr {
        LValue target = std::move(*$2);
        delete $2;
        $$ = new_stmt(std::make_shared<Statement>(std::vector<ExpressionPtr>{take_expr($4)},
            [target](const std::vector<Value>& args, Environment& env) {
                env.Assign(target, args[0]);
            },
            [target](const std::vector<ExpressionPtr>& args) {
                return "LET " + target.List() + " = " + args[0]->List();
            }));
    }
    | lval '=' expr {
        LValue target = std::move(*$1);
        delete $1;
        $$ = new_stmt(std::make_shared<Statement>(std::vector<ExpressionPtr>{take_expr($3)},
            [target](const std::vector<Value>& args, Environment& env) {
                env.Assign(target, args[0]);
            },
            [target](const std::vector<ExpressionPtr>& args) {
                return "LET " + target.List() + " = " + args[0]->List();
            }));
    }
    /* PRINT – the item list and trailing separator are parsed by printargs */
    | KW_PRINT printargs {
        std::vector<PrintItem> items = std::move($2->items);
        bool newline = $2->newline;
        delete $2;

        std::vector<ExpressionPtr> argExprs;
        argExprs.reserve(items.size());
        for (auto& item : items)
            argExprs.push_back(item.expr);

        $$ = new_stmt(std::make_shared<Statement>(std::move(argExprs),
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
            },
            [items, newline](const std::vector<ExpressionPtr>& args) {
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
            }));
    }
    /* INPUT – prompt is optional (see inputprompt); variables use lvallist */
    | KW_INPUT inputprompt lvallist {
        std::string prompt = std::move(*$2);
        std::vector<LValue> targets = std::move(*$3);
        delete $2; delete $3;
        $$ = new_stmt(std::make_shared<Statement>(std::vector<ExpressionPtr>{},
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
            },
            [prompt, targets](const std::vector<ExpressionPtr>&) {
                std::string s = "INPUT";
                if (prompt != "? ")
                    s += " \"" + prompt + "\",";
                for (size_t i = 0; i < targets.size(); ++i)
                    s += (i == 0 ? " " : ", ") + targets[i].List();
                return s;
            }));
    }
    /* IF */
    | KW_IF expr KW_THEN stmtlist {
        StatementPtr thenBranch = take_stmt($4);
        StatementPtr elseBranch = nullptr;
        $$ = new_stmt(std::make_shared<Statement>(std::vector<ExpressionPtr>{take_expr($2)},
            [thenBranch, elseBranch](const std::vector<Value>& args, Environment& env) {
                if (args[0].GetTruthy())
                    env.GetProgramManager().SetRuntimeStatement(thenBranch);
                else if (elseBranch)
                    env.GetProgramManager().SetRuntimeStatement(elseBranch);
            },
            [thenBranch, elseBranch](const std::vector<ExpressionPtr>& args) {
                std::string s = "IF " + args[0]->List() + " THEN " + thenBranch->List();
                if (elseBranch) s += " ELSE " + elseBranch->List();
                return s;
            }));
    }
    | KW_IF expr KW_THEN stmtlist KW_ELSE stmtlist {
        StatementPtr thenBranch = take_stmt($4);
        StatementPtr elseBranch = take_stmt($6);
        $$ = new_stmt(std::make_shared<Statement>(std::vector<ExpressionPtr>{take_expr($2)},
            [thenBranch, elseBranch](const std::vector<Value>& args, Environment& env) {
                if (args[0].GetTruthy())
                    env.GetProgramManager().SetRuntimeStatement(thenBranch);
                else if (elseBranch)
                    env.GetProgramManager().SetRuntimeStatement(elseBranch);
            },
            [thenBranch, elseBranch](const std::vector<ExpressionPtr>& args) {
                std::string s = "IF " + args[0]->List() + " THEN " + thenBranch->List();
                if (elseBranch) s += " ELSE " + elseBranch->List();
                return s;
            }));
    }
    | KW_IF expr KW_THEN INTEGER {
        // "IF cond THEN 100" is shorthand for "IF cond THEN GOTO 100"
        int targetLine = (int)$4;
        StatementPtr thenBranch = std::make_shared<Statement>(std::vector<ExpressionPtr>{},
            [targetLine](const std::vector<Value>&, Environment& env) {
                env.GetProgramManager().Goto(targetLine);
            },
            [targetLine](const std::vector<ExpressionPtr>&) {
                return "GOTO " + std::to_string(targetLine);
            });
        StatementPtr elseBranch = nullptr;
        $$ = new_stmt(std::make_shared<Statement>(std::vector<ExpressionPtr>{take_expr($2)},
            [thenBranch, elseBranch](const std::vector<Value>& args, Environment& env) {
                if (args[0].GetTruthy())
                    env.GetProgramManager().SetRuntimeStatement(thenBranch);
                else if (elseBranch)
                    env.GetProgramManager().SetRuntimeStatement(elseBranch);
            },
            [thenBranch, elseBranch](const std::vector<ExpressionPtr>& args) {
                std::string s = "IF " + args[0]->List() + " THEN " + thenBranch->List();
                if (elseBranch) s += " ELSE " + elseBranch->List();
                return s;
            }));
    }
    /* GOTO / GOSUB / RETURN */
    | KW_GOTO INTEGER {
        int lineNumber = (int)$2;
        $$ = new_stmt(std::make_shared<Statement>(std::vector<ExpressionPtr>{},
            [lineNumber](const std::vector<Value>&, Environment& env) {
                env.GetProgramManager().Goto(lineNumber);
            },
            [lineNumber](const std::vector<ExpressionPtr>&) {
                return "GOTO " + std::to_string(lineNumber);
            }));
    }
    | KW_GOSUB INTEGER {
        int lineNumber = (int)$2;
        $$ = new_stmt(std::make_shared<Statement>(std::vector<ExpressionPtr>{},
            [lineNumber](const std::vector<Value>&, Environment& env) {
                env.GetProgramManager().Gosub(lineNumber);
            },
            [lineNumber](const std::vector<ExpressionPtr>&) {
                return "GOSUB " + std::to_string(lineNumber);
            }));
    }
    | KW_ON expr KW_GOTO intlist {
        std::vector<int> lines = std::move(*$4);
        delete $4;
        $$ = new_stmt(std::make_shared<Statement>(std::vector<ExpressionPtr>{take_expr($2)},
            [lines](const std::vector<Value>& args, Environment& env) {
                Integer idx = args[0].GetInteger() - 1;
                if (idx >= 0 && idx < static_cast<Integer>(lines.size()))
                    env.GetProgramManager().Goto(lines[idx]);
            },
            [lines](const std::vector<ExpressionPtr>& args) {
                std::string s = "ON " + args[0]->List() + " GOTO ";
                for (size_t i = 0; i < lines.size(); ++i) {
                    if (i > 0) s += ", ";
                    s += std::to_string(lines[i]);
                }
                return s;
            }));
    }
    | KW_ON expr KW_GOSUB intlist {
        std::vector<int> lines = std::move(*$4);
        delete $4;
        $$ = new_stmt(std::make_shared<Statement>(std::vector<ExpressionPtr>{take_expr($2)},
            [lines](const std::vector<Value>& args, Environment& env) {
                Integer idx = args[0].GetInteger() - 1;
                if (idx >= 0 && idx < static_cast<Integer>(lines.size()))
                    env.GetProgramManager().Gosub(lines[idx]);
            },
            [lines](const std::vector<ExpressionPtr>& args) {
                std::string s = "ON " + args[0]->List() + " GOSUB ";
                for (size_t i = 0; i < lines.size(); ++i) {
                    if (i > 0) s += ", ";
                    s += std::to_string(lines[i]);
                }
                return s;
            }));
    }
    | KW_RETURN {
        $$ = new_stmt(std::make_shared<Statement>(std::vector<ExpressionPtr>{},
            [](const std::vector<Value>&, Environment& env) {
                env.GetProgramManager().Return();
            },
            [](const std::vector<ExpressionPtr>&) { return "RETURN"; }));
    }
    /* FOR / NEXT – loop variable is an lval. Uses Value throughout so
       fractional STEP values work correctly. */
    | KW_FOR lval '=' expr KW_TO expr {
        LValue target = std::move(*$2);
        delete $2;
        $$ = new_stmt(std::make_shared<Statement>(
            std::vector<ExpressionPtr>{take_expr($4), take_expr($6)},
            [target](const std::vector<Value>& args, Environment& env) {
                Value startVal = args[0];
                Value limitVal = args[1];
                Value stepVal  = Value(1);

                env.GetProgramManager().For(target.name, startVal, limitVal, stepVal);
            },
            [target](const std::vector<ExpressionPtr>& args) {
                return "FOR " + target.List() + " = " + args[0]->List() + " TO " + args[1]->List();
            }));
    }
    | KW_FOR lval '=' expr KW_TO expr KW_STEP expr {
        LValue target = std::move(*$2);
        delete $2;
        $$ = new_stmt(std::make_shared<Statement>(
            std::vector<ExpressionPtr>{take_expr($4), take_expr($6), take_expr($8)},
            [target](const std::vector<Value>& args, Environment& env) {
                Value startVal = args[0];
                Value limitVal = args[1];
                Value stepVal  = args[2];

                env.GetProgramManager().For(target.name, startVal, limitVal, stepVal);
            },
            [target](const std::vector<ExpressionPtr>& args) {
                return "FOR " + target.List() + " = " + args[0]->List() + " TO " + args[1]->List()
                       + " STEP " + args[2]->List();
            }));
    }
    | KW_NEXT {
        $$ = new_stmt(std::make_shared<Statement>(std::vector<ExpressionPtr>{},
            [](const std::vector<Value>&, Environment& env) {
                env.GetProgramManager().Next("");
            },
            [](const std::vector<ExpressionPtr>&) { return std::string("NEXT"); }));
    }
    | KW_NEXT IDENTIFIER {
        std::string variable = *$2;
        delete $2;
        $$ = new_stmt(std::make_shared<Statement>(std::vector<ExpressionPtr>{},
            [variable](const std::vector<Value>&, Environment& env) {
                env.GetProgramManager().Next(variable);
            },
            [variable](const std::vector<ExpressionPtr>&) {
                return "NEXT " + variable;
            }));
    }
    /* DIM */
    | KW_DIM dimlist {
        std::vector<ArrayDecl> decls = std::move(*$2);
        delete $2;
        $$ = new_stmt(std::make_shared<Statement>(std::vector<ExpressionPtr>{},
            [decls](const std::vector<Value>&, Environment& env) {
                for (auto& decl : decls) {
                    std::vector<int> dims;
                    for (auto& dimExpr : decl.dims)
                        dims.push_back(dimExpr->Evaluate(env).GetInteger());
                    env.DimArray(decl.name, dims);
                }
            },
            [decls](const std::vector<ExpressionPtr>&) {
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
            }));
    }
    /* DEF FN */
    | KW_DEF FNIDENTIFIER '(' IDENTIFIER ')' '=' expr {
        std::string name  = *$2;
        std::string param = *$4;
        ExpressionPtr body = take_expr($7);
        delete $2; delete $4;
        $$ = new_stmt(std::make_shared<Statement>(std::vector<ExpressionPtr>{},
            [name, param, body](const std::vector<Value>&, Environment& env) {
                env.DefFunction(name, param, body);
            },
            [name, param, body](const std::vector<ExpressionPtr>&) {
                return "DEF " + name + "(" + param + ") = " + body->List();
            }));
    }
    /* END / STOP / EXIT / REM */
    | KW_END {
        $$ = new_stmt(std::make_shared<Statement>(std::vector<ExpressionPtr>{},
            [](const std::vector<Value>&, Environment& env) {
                env.GetProgramManager().End();
            },
            [](const std::vector<ExpressionPtr>&) { return "END"; }));
    }
    | KW_STOP {
        $$ = new_stmt(std::make_shared<Statement>(std::vector<ExpressionPtr>{},
            [](const std::vector<Value>&, Environment& env) {
                env.GetProgramManager().Stop();
            },
            [](const std::vector<ExpressionPtr>&) { return "STOP"; }));
    }
    /* EXIT triggers a clean shutdown via the exit flag rather than std::exit(). */
    | KW_EXIT {
        $$ = new_stmt(std::make_shared<Statement>(std::vector<ExpressionPtr>{},
            [](const std::vector<Value>&, Environment& env) {
                env.GetProgramManager().SetExitFlag(true);
            },
            [](const std::vector<ExpressionPtr>&) { return "EXIT"; }));
    }
    | KW_REM {
        std::string text = *$1;
        delete $1;
        $$ = new_stmt(std::make_shared<Statement>(std::vector<ExpressionPtr>{},
            [](const std::vector<Value>&, Environment&) {},
            [text](const std::vector<ExpressionPtr>&) { return "REM" + text; }));
    }
    /* READ / RESTORE */
    | KW_READ lvallist {
        std::vector<LValue> targets = std::move(*$2);
        delete $2;
        $$ = new_stmt(std::make_shared<Statement>(std::vector<ExpressionPtr>{},
            [targets](const std::vector<Value>&, Environment& env) {
                for (auto& lv : targets) {
                    Value val = env.GetDataManager().ReadData(env);
                    val.NormaliseType(lv.isString());
                    env.Assign(lv, val);
                }
            },
            [targets](const std::vector<ExpressionPtr>&) {
                std::string s = "READ ";
                for (size_t i = 0; i < targets.size(); ++i) {
                    if (i > 0) s += ", ";
                    s += targets[i].List();
                }
                return s;
            }));
    }
    | KW_RESTORE {
        $$ = new_stmt(std::make_shared<Statement>(std::vector<ExpressionPtr>{},
            [](const std::vector<Value>&, Environment& env) {
                env.GetDataManager().RestoreData(-1, true);
            },
            [](const std::vector<ExpressionPtr>&) { return "RESTORE"; }));
    }
    | KW_RESTORE INTEGER {
        int lineNumber = (int)$2;
        $$ = new_stmt(std::make_shared<Statement>(std::vector<ExpressionPtr>{},
            [lineNumber](const std::vector<Value>&, Environment& env) {
                env.GetDataManager().RestoreData(lineNumber, true);
            },
            [lineNumber](const std::vector<ExpressionPtr>&) {
                return "RESTORE " + std::to_string(lineNumber);
            }));
    }
    /* RANDOMIZE / CLEAR / CLS */
    | KW_RANDOMIZE {
        $$ = new_stmt(std::make_shared<Statement>(std::vector<ExpressionPtr>{},
            [](const std::vector<Value>&, Environment& env) {
                env.Randomize(-1);
            },
            [](const std::vector<ExpressionPtr>&) { return "RANDOMIZE"; }));
    }
    | KW_RANDOMIZE expr {
        $$ = new_stmt(std::make_shared<Statement>(std::vector<ExpressionPtr>{take_expr($2)},
            [](const std::vector<Value>& args, Environment& env) {
                env.Randomize(static_cast<long>(args[0].GetInteger()));
            },
            [](const std::vector<ExpressionPtr>& args) {
                return "RANDOMIZE " + args[0]->List();
            }));
    }
    | KW_CLEAR {
        $$ = new_stmt(std::make_shared<Statement>(std::vector<ExpressionPtr>{},
            [](const std::vector<Value>&, Environment& env) { env.ClearVariables(); },
            [](const std::vector<ExpressionPtr>&) { return "CLEAR"; }));
    }
    | KW_CLS {
        $$ = new_stmt(std::make_shared<Statement>(std::vector<ExpressionPtr>{},
            [](const std::vector<Value>&, Environment&) {
#ifdef _WIN32
                std::system("cls");
#else
                std::system("clear");
#endif
            },
            [](const std::vector<ExpressionPtr>&) { return "CLS"; }));
    }
    /* WHILE / WEND */
    | KW_WHILE expr {
        // The id this WHILE will be registered under is just the table's
        // current size (we're about to append to it) - computing it up front,
        // before building the statement, lets the action capture it directly.
        int whileId = env.GetProgramManager().WhileFrameCount();

        StatementPtr whileStatement = std::make_shared<Statement>(
            std::vector<ExpressionPtr>{take_expr($2)},
            [whileId](const std::vector<Value>& args, Environment& env) {
                env.GetProgramManager().While(args[0], env.GetProgramManager().GetWhileFrame(whileId).m_wendip);
            },
            [](const std::vector<ExpressionPtr>& args) {
                return "WHILE " + args[0]->List();
            });

        int lineNumber = env.GetProgramManager().GetCurrentParseLineNumber();
        env.GetProgramManager().RegisterWhileFrame(RuntimeIP(lineNumber, whileStatement));
        env.GetProgramManager().PushWhileId(whileId);
        $$ = new_stmt(whileStatement);
    }
    | KW_WEND {
        if (env.GetProgramManager().WhileStackEmpty()) {
            throw std::runtime_error("WEND without matching WHILE");
        }

        int whileId = env.GetProgramManager().PopWhileId();

        StatementPtr wendStatement = std::make_shared<Statement>(std::vector<ExpressionPtr>{},
            [whileId](const std::vector<Value>&, Environment& env) {
                env.GetProgramManager().Wend(env.GetProgramManager().GetWhileFrame(whileId).m_whileip);
            },
            [](const std::vector<ExpressionPtr>&) { return "WEND"; });

        int lineNumber = env.GetProgramManager().GetCurrentParseLineNumber();
        env.GetProgramManager().GetWhileFrame(whileId).m_wendip = RuntimeIP(lineNumber, wendStatement);

        $$ = new_stmt(wendStatement);
    }
    ;

/* ── PRINT item list ────────────────────────────────────────────────────── */
printlist
    : expr {
        $$ = new std::vector<PrintItem>();
        $$->push_back({take_expr($1), false, false, false});
    }
    | FN_TAB '(' expr ')' {
        $$ = new std::vector<PrintItem>();
        $$->push_back({take_expr($3), false, false, true});
    }
    | printlist ';' expr {
        $1->back().semicolon = true;
        $1->push_back({take_expr($3), false, false, false});
        $$ = $1;
    }
    | printlist ',' expr {
        $1->back().comma = true;
        $1->push_back({take_expr($3), false, false, false});
        $$ = $1;
    }
    | printlist ';' FN_TAB '(' expr ')' {
        $1->back().semicolon = true;
        $1->push_back({take_expr($5), false, false, true});
        $$ = $1;
    }
    | printlist ',' FN_TAB '(' expr ')' {
        $1->back().comma = true;
        $1->push_back({take_expr($5), false, false, true});
        $$ = $1;
    }
    ;

/* ── Misc lists ─────────────────────────────────────────────────────────── */
intlist
    : INTEGER {
        $$ = new std::vector<int>();
        $$->push_back((int)$1);
    }
    | intlist ',' INTEGER {
        $1->push_back((int)$3);
        $$ = $1;
    }
    ;

dimlist
    : IDENTIFIER '(' dimexprlist ')' {
        $$ = new std::vector<ArrayDecl>();
        $$->push_back({*$1, std::move(*$3)});
        delete $1; delete $3;
    }
    | STRIDENTIFIER '(' dimexprlist ')' {
        $$ = new std::vector<ArrayDecl>();
        $$->push_back({*$1, std::move(*$3)});
        delete $1; delete $3;
    }
    | dimlist ',' IDENTIFIER '(' dimexprlist ')' {
        $1->push_back({*$3, std::move(*$5)});
        delete $3; delete $5;
        $$ = $1;
    }
    | dimlist ',' STRIDENTIFIER '(' dimexprlist ')' {
        $1->push_back({*$3, std::move(*$5)});
        delete $3; delete $5;
        $$ = $1;
    }
    ;

dimexprlist
    : expr {
        $$ = new std::vector<Basic::ExpressionPtr>();
        $$->push_back(take_expr($1));
    }
    | dimexprlist ',' expr {
        $1->push_back(take_expr($3));
        $$ = $1;
    }
    ;

exprlist
    : expr {
        $$ = new std::vector<Basic::ExpressionPtr>();
        $$->push_back(take_expr($1));
    }
    | exprlist ',' expr {
        $1->push_back(take_expr($3));
        $$ = $1;
    }
    ;

/* A PRINT's whole argument list: nothing, or a list optionally followed by
 * ';' or ',' (which suppress the newline; ',' also tabs to the next zone). */
printargs
    : %empty {
        $$ = new PrintArgs{{}, true};
    }
    | printlist {
        $$ = new PrintArgs{std::move(*$1), true};
        delete $1;
    }
    | printlist ';' {
        $$ = new PrintArgs{std::move(*$1), false};
        delete $1;
    }
    | printlist ',' {
        if (!$1->empty()) $1->back().comma = true;
        $$ = new PrintArgs{std::move(*$1), false};
        delete $1;
    }
    ;

/* INPUT's optional prompt string; defaults to "? " */
inputprompt
    : %empty {
        $$ = new std::string("? ");
    }
    | STRING ',' {
        $$ = $1;
    }
    ;

/* ── Expression grammar ─────────────────────────────────────────────────── */
/* Leaf expressions (literals, variables, array reads, user-defined function
 * calls) come from the Make* functions in expression.cpp. Operators and
 * built-in functions are built right here: each action wraps its operands
 * in a LambdaExpression, supplying a lambda that computes the value from the
 * evaluated operands and a lambda that renders the expression back to text
 * for LIST. */
expr
    : INTEGER      { $$ = new_expr(MakeLiteral(Value((Integer)$1))); }
    | FLOAT        { $$ = new_expr(MakeLiteral(Value((Real)$1))); }
    | STRING       { $$ = new_expr(MakeStringLiteral(*$1)); delete $1; }
    | IDENTIFIER   { $$ = new_expr(MakeVariable(*$1)); delete $1; }
    | STRIDENTIFIER{ $$ = new_expr(MakeVariable(*$1)); delete $1; }
    | IDENTIFIER '(' exprlist ')' {
        $$ = new_expr(MakeArray(*$1, std::move(*$3)));
        delete $1; delete $3;
    }
    | STRIDENTIFIER '(' exprlist ')' {
        $$ = new_expr(MakeArray(*$1, std::move(*$3)));
        delete $1; delete $3;
    }
    | FNIDENTIFIER '(' expr ')' {
        $$ = new_expr(MakeUserFn(*$1, take_expr($3)));
        delete $1;
    }
    | expr '+' expr {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($1), take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                return args[0].AddOp(args[1], env);
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return ops[0]->List() + " + " + ops[1]->List();
            }));
    }
    | expr '-' expr {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($1), take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                return args[0].SubOp(args[1], env);
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return ops[0]->List() + " - " + ops[1]->List();
            }));
    }
    | expr '*' expr {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($1), take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                return args[0].MulOp(args[1], env);
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return ops[0]->List() + " * " + ops[1]->List();
            }));
    }
    | expr '/' expr {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($1), take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                return args[0].DivOp(args[1], env);
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return ops[0]->List() + " / " + ops[1]->List();
            }));
    }
    | expr '^' expr {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($1), take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                return args[0].PowOp(args[1], env);
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return ops[0]->List() + " ^ " + ops[1]->List();
            }));
    }
    | expr '=' expr {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($1), take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                return Value(args[0].CompareOp(args[1], env) == 0);
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return ops[0]->List() + " = " + ops[1]->List();
            }));
    }
    | expr NEQ expr {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($1), take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                return Value(args[0].CompareOp(args[1], env) != 0);
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return ops[0]->List() + " <> " + ops[1]->List();
            }));
    }
    | expr '<' expr {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($1), take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                return Value(args[0].CompareOp(args[1], env) < 0);
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return ops[0]->List() + " < " + ops[1]->List();
            }));
    }
    | expr LEQ expr {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($1), take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                return Value(args[0].CompareOp(args[1], env) <= 0);
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return ops[0]->List() + " <= " + ops[1]->List();
            }));
    }
    | expr '>' expr {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($1), take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                return Value(args[0].CompareOp(args[1], env) > 0);
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return ops[0]->List() + " > " + ops[1]->List();
            }));
    }
    | expr GEQ expr {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($1), take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                return Value(args[0].CompareOp(args[1], env) >= 0);
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return ops[0]->List() + " >= " + ops[1]->List();
            }));
    }
    | expr KW_AND expr {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($1), take_expr($3)},
            [](const std::vector<Value>& args, Environment&) -> Value {
                return Value(static_cast<Integer>(args[0].GetTruthy() && args[1].GetTruthy() ? 1 : 0));
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return ops[0]->List() + " AND " + ops[1]->List();
            }));
    }
    | expr KW_OR expr {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($1), take_expr($3)},
            [](const std::vector<Value>& args, Environment&) -> Value {
                return Value(static_cast<Integer>(args[0].GetTruthy() || args[1].GetTruthy() ? 1 : 0));
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return ops[0]->List() + " OR " + ops[1]->List();
            }));
    }
    | KW_NOT expr {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($2)},
            [](const std::vector<Value>& args, Environment&) -> Value {
                return Value(static_cast<Integer>(args[0].GetTruthy() ? 0 : 1));
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return "NOT " + ops[0]->List();
            }));
    }
    | '(' expr ')'     { $$ = $2; }
    | FN_ABS '(' expr ')' {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                return Value(args[0].AbsOp(env));
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return "ABS(" + ops[0]->List() + ")";
            }));
    }
    | FN_ATN '(' expr ')' {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                return Value(args[0].AtnOp(env));
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return "ATN(" + ops[0]->List() + ")";
            }));
    }
    | FN_COS '(' expr ')' {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                return Value(args[0].CosOp(env));
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return "COS(" + ops[0]->List() + ")";
            }));
    }
    | FN_EXP '(' expr ')' {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                return Value(args[0].ExpOp(env));
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return "EXP(" + ops[0]->List() + ")";
            }));
    }
    | FN_INT '(' expr ')' {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                return Value(args[0].IntOp(env));
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return "INT(" + ops[0]->List() + ")";
            }));
    }
    | FN_LOG '(' expr ')' {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                return Value(args[0].LogOp(env));
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return "LOG(" + ops[0]->List() + ")";
            }));
    }
    | FN_RND '(' expr ')' {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                return Value(args[0].RndOp(env));
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return "RND(" + ops[0]->List() + ")";
            }));
    }
    | FN_SGN '(' expr ')' {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                return Value(args[0].SgnOp(env));
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return "SGN(" + ops[0]->List() + ")";
            }));
    }
    | FN_SIN '(' expr ')' {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                return Value(args[0].SinOp(env));
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return "SIN(" + ops[0]->List() + ")";
            }));
    }
    | FN_SQR '(' expr ')' {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                return Value(args[0].SqrOp(env));
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return "SQR(" + ops[0]->List() + ")";
            }));
    }
    | FN_TAN '(' expr ')' {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                return Value(args[0].TanOp(env));
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return "TAN(" + ops[0]->List() + ")";
            }));
    }
    | FN_LEN '(' expr ')' {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                return Value(static_cast<Integer>(args[0].TryGetString(env, "LEN").size()));
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return "LEN(" + ops[0]->List() + ")";
            }));
    }
    | FN_ASC '(' expr ')' {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                const std::string& s = args[0].TryGetString(env, "ASC");
                if (s.empty()) throw RuntimeError(env, "ASC: empty string");
                return Value(static_cast<Integer>(static_cast<unsigned char>(s[0])));
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return "ASC(" + ops[0]->List() + ")";
            }));
    }
    | FN_CHR '(' expr ')' {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                Integer code = static_cast<Integer>(args[0].TryGetInteger(env, "CHR$"));
                return Value(std::string(1, static_cast<char>(code)));
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return "CHR$(" + ops[0]->List() + ")";
            }));
    }
    | FN_STR '(' expr ')' {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                const Value& v = args[0];
                if (!v.IsNumber())
                    throw RuntimeError(env, "STR$: expected numeric argument");
                return Value(v.PPrint());
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return "STR$(" + ops[0]->List() + ")";
            }));
    }
    | FN_VAL '(' expr ')' {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                const std::string& s = args[0].TryGetString(env, "VAL");

                Basic::Value val(s);
                val.NormaliseType();

                return val;
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return "VAL(" + ops[0]->List() + ")";
            }));
    }
    | FN_SPC '(' expr ')' {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($3)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                long n = static_cast<long>(args[0].TryGetInteger(env, "SPC"));
                return Value(std::string(std::max(0L, n), ' '));
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return "SPC(" + ops[0]->List() + ")";
            }));
    }
    | FN_LEFT '(' expr ',' expr ')' {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($3), take_expr($5)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                std::string s = args[0].TryGetString(env, "LEFT$");
                long n = args[1].TryGetInteger(env, "LEFT$");
                return Value(s.substr(0, std::min((size_t)std::max(0L, n), s.size())));
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return "LEFT$(" + ops[0]->List() + ", " + ops[1]->List() + ")";
            }));
    }
    | FN_RIGHT '(' expr ',' expr ')' {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($3), take_expr($5)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                std::string s = args[0].TryGetString(env, "RIGHT$");
                long n = args[1].TryGetInteger(env, "RIGHT$");
                long start = (long)s.size() - n;
                if (start < 0) start = 0;
                return Value(s.substr(start));
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return "RIGHT$(" + ops[0]->List() + ", " + ops[1]->List() + ")";
            }));
    }
    | FN_MID '(' expr ',' expr ')' {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($3), take_expr($5)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                const Value& sv = args[0];
                if (!sv.IsString())
                    throw RuntimeError(env, "MID$: expected string argument");

                const std::string s = sv.TryGetString(env, "MID$");

                const Value& startv = args[1];
                if (!startv.IsNumber())
                    throw RuntimeError(env, "MID$: expected numeric argument");

                Integer start = startv.TryGetInteger(env, "MID$");

                if (start <= 0) start = 1;
                if (start > (long)s.size()) return Value(std::string(""));
                return Value(s.substr(start - 1));
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return "MID$(" + ops[0]->List() + ", " + ops[1]->List() + ")";
            }));
    }
    | FN_MID '(' expr ',' expr ',' expr ')' {
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{take_expr($3), take_expr($5), take_expr($7)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                const Value& sv = args[0];
                if (!sv.IsString())
                    throw RuntimeError(env, "MID$: expected string argument");

                const std::string s = sv.TryGetString(env, "MID$");

                const Value& startv = args[1];
                if (!startv.IsNumber())
                    throw RuntimeError(env, "MID$: expected numeric argument");

                Integer start = startv.TryGetInteger(env, "MID$");

                const Value& lenv = args[2];
                if (!lenv.IsNumber())
                    throw RuntimeError(env, "MID$: expected numeric argument");

                Integer len = lenv.TryGetInteger(env, "MID$");

                if (start <= 0) start = 1;
                if (start > (long)s.size()) return Value(std::string(""));
                return Value(s.substr(start - 1, len));
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return "MID$(" + ops[0]->List() + ", " + ops[1]->List() + ", " + ops[2]->List() + ")";
            }));
    }
    | '-' expr %prec UMINUS {
        // Unary minus is built as (0 - operand), so it lists as "0 - x".
        $$ = new_expr(std::make_shared<LambdaExpression>(
            std::vector<ExpressionPtr>{MakeLiteral(Value(0)), take_expr($2)},
            [](const std::vector<Value>& args, Environment& env) -> Value {
                return args[0].SubOp(args[1], env);
            },
            [](const std::vector<ExpressionPtr>& ops) {
                return ops[0]->List() + " - " + ops[1]->List();
            }));
    }
    ;

%%

void yyerror(Basic::Environment& env, const char* s) {
    std::string display = yyinput_line;
    if (!display.empty() && display.back() == '\n')
        display.pop_back();
    int col = yycolumn - 1;
    if (col < 1) col = 1;
    fprintf(stderr, "Error: %s\n", s);
    if (!display.empty()) {
        fprintf(stderr, "  %s\n", display.c_str());
        fprintf(stderr, "  %*s^\n", col - 1, "");
    }
    throw ParseErrorReported{};
}
