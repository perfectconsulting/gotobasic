%code requires {
    #include <vector>
    #include <string>
    #include <memory>
    #include "expression.hpp"
    #include "statement.hpp"
    #include "value.hpp"
    #include "environment.hpp"
    #include "expression_ops.hpp"
    #include "statement_ops.hpp"
}

%{
#include <cstdio>
#include <memory>
#include <vector>
#include <string>
#include <stdexcept>
#include "environment.hpp"
#include "expression.hpp"
#include "statement.hpp"
#include "expression_ops.hpp"
#include "statement_ops.hpp"

using namespace Basic;
using namespace Basic::Ops;

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
    std::vector<Basic::Ops::ArrayDecl>*  dimlist;
    std::vector<Basic::Ops::PrintItem>* printitems;
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
/* Built via the Make* functions in command_ops.hpp, the same way expressions
 * are built via MakeAdd/MakeSub/... in maths_ops.hpp, rather than naming a
 * concrete Command subclass at each call site. */
command
    : KW_LIST                           { $$ = new_stmt(MakeList()); }
    | KW_LIST INTEGER ',' INTEGER       { $$ = new_stmt(MakeList($2, $4)); }
    | KW_RUN                            { $$ = new_stmt(MakeRun()); }
    | KW_NEW                            { $$ = new_stmt(MakeNew()); }
    | KW_DELETE INTEGER                 { $$ = new_stmt(MakeDelete($2, $2)); }
    | KW_DELETE INTEGER ',' INTEGER     { $$ = new_stmt(MakeDelete($2, $4)); }
    | KW_LOAD STRING {
        $$ = new_stmt(MakeLoad(*$2));
        delete $2;
    }
    | KW_SAVE STRING {
        $$ = new_stmt(MakeSave(*$2));
        delete $2;
    }
    | KW_TRON                            { $$ = new_stmt(MakeTron()); }
    | KW_TROFF                           { $$ = new_stmt(MakeTroff()); }
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
statement
    /* LET with optional keyword */
    : KW_LET lval '=' expr {
        $$ = new_stmt(MakeLetStatement(std::move(*$2), take_expr($4)));
        delete $2;
    }
    | lval '=' expr {
        $$ = new_stmt(MakeLetStatement(std::move(*$1), take_expr($3)));
        delete $1;
    }
    /* PRINT */
    | KW_PRINT {
        $$ = new_stmt(MakePrintStatement(std::vector<PrintItem>{}, true));
    }
    | KW_PRINT printlist {
        $$ = new_stmt(MakePrintStatement(std::move(*$2), true));
        delete $2;
    }
    | KW_PRINT printlist ';' {
        $$ = new_stmt(MakePrintStatement(std::move(*$2), false));
        delete $2;
    }
    | KW_PRINT printlist ',' {
        auto items = std::move(*$2); delete $2;
        if (!items.empty()) items.back().comma = true;
        $$ = new_stmt(MakePrintStatement(std::move(items), false));
    }
    /* INPUT – prompt is optional; variable list uses the unified lvallist */
    | KW_INPUT lvallist {
        $$ = new_stmt(MakeInputStatement("? ", std::move(*$2)));
        delete $2;
    }
    | KW_INPUT STRING ',' lvallist {
        $$ = new_stmt(MakeInputStatement(*$2, std::move(*$4)));
        delete $2; delete $4;
    }
    /* IF */
    | KW_IF expr KW_THEN stmtlist {
        $$ = new_stmt(MakeIfStatement(take_expr($2), take_stmt($4)));
    }
    | KW_IF expr KW_THEN stmtlist KW_ELSE stmtlist {
        $$ = new_stmt(MakeIfStatement(take_expr($2), take_stmt($4), take_stmt($6)));
    }
    | KW_IF expr KW_THEN INTEGER {
        $$ = new_stmt(MakeIfStatement(take_expr($2), MakeGotoStatement((int)$4)));
    }
    /* GOTO / GOSUB / RETURN */
    | KW_GOTO INTEGER   { $$ = new_stmt(MakeGotoStatement((int)$2));  }
    | KW_GOSUB INTEGER  { $$ = new_stmt(MakeGosubStatement((int)$2)); }
    | KW_ON expr KW_GOTO intlist {
        $$ = new_stmt(MakeOnGotoStatement(take_expr($2), std::move(*$4)));
        delete $4;
    }
    | KW_ON expr KW_GOSUB intlist {
        $$ = new_stmt(MakeOnGosubStatement(take_expr($2), std::move(*$4)));
        delete $4;
    }
    | KW_RETURN         { $$ = new_stmt(MakeReturnStatement()); }
    /* FOR / NEXT – loop variable is an lval */
    | KW_FOR lval '=' expr KW_TO expr {
        $$ = new_stmt(MakeForStatement(std::move(*$2), take_expr($4), take_expr($6)));
        delete $2;
    }
    | KW_FOR lval '=' expr KW_TO expr KW_STEP expr {
        $$ = new_stmt(MakeForStatement(std::move(*$2), take_expr($4), take_expr($6), take_expr($8)));
        delete $2;
    }
    | KW_NEXT           { $$ = new_stmt(MakeNextStatement());   }
    | KW_NEXT IDENTIFIER {
        $$ = new_stmt(MakeNextStatement(*$2));
        delete $2;
    }
    /* DIM */
    | KW_DIM dimlist {
        $$ = new_stmt(MakeDimStatement(std::move(*$2)));
        delete $2;
    }
    /* DEF FN */
    | KW_DEF FNIDENTIFIER '(' IDENTIFIER ')' '=' expr {
        $$ = new_stmt(MakeDefStatement(*$2, *$4, take_expr($7)));
        delete $2; delete $4;
    }
    /* END / STOP / EXIT / REM */
    | KW_END   { $$ = new_stmt(MakeEndStatement());   }
    | KW_STOP  { $$ = new_stmt(MakeStopStatement());  }
    | KW_EXIT  { $$ = new_stmt(MakeExitStatement());  }
    | KW_REM {
        $$ = new_stmt(MakeRemStatement(*$1));
        delete $1;
    }
    | KW_READ lvallist {
        $$ = new_stmt(MakeRead(std::move(*$2)));
        delete $2;
    }
    | KW_RESTORE {
        $$ = new_stmt(MakeRestoreStatement());
    }
    | KW_RESTORE INTEGER {
        $$ = new_stmt(MakeRestoreStatement((int)$2));
    }
    /* RANDOMIZE / CLEAR / CLS */
    | KW_RANDOMIZE {
        $$ = new_stmt(MakeRandomizeStatement());
    }
    | KW_RANDOMIZE expr {
        $$ = new_stmt(MakeRandomizeStatement(take_expr($2)));
    }
    | KW_CLEAR {
        $$ = new_stmt(MakeClearStatement());
    }
    | KW_CLS {
        $$ = new_stmt(MakeClsStatement());
    }
    /* WHILE / WEND */
    | KW_WHILE expr {
        $$ = new_stmt(MakeWhileStatement(take_expr($2), env));
    }
    | KW_WEND {
        $$ = new_stmt(MakeWendStatement(env));
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
        $$ = new std::vector<Ops::ArrayDecl>();
        $$->push_back({*$1, std::move(*$3)});
        delete $1; delete $3;
    }
    | STRIDENTIFIER '(' dimexprlist ')' {
        $$ = new std::vector<Ops::ArrayDecl>();
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

/* ── Expression grammar ─────────────────────────────────────────────────── */
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
    | expr '+' expr { $$ = new_expr(MakeAdd(take_expr($1), take_expr($3))); }
    | expr '-' expr { $$ = new_expr(MakeSub(take_expr($1), take_expr($3))); }
    | expr '*' expr { $$ = new_expr(MakeMul(take_expr($1), take_expr($3))); }
    | expr '/' expr { $$ = new_expr(MakeDiv(take_expr($1), take_expr($3))); }
    | expr '^' expr { $$ = new_expr(MakePow(take_expr($1), take_expr($3))); }
    | expr '=' expr { $$ = new_expr(MakeEqual(take_expr($1),        take_expr($3))); }
    | expr NEQ expr { $$ = new_expr(MakeNotEqual(take_expr($1),     take_expr($3))); }
    | expr '<' expr { $$ = new_expr(MakeLessThan(take_expr($1),     take_expr($3))); }
    | expr LEQ expr { $$ = new_expr(MakeLessEqual(take_expr($1),    take_expr($3))); }
    | expr '>' expr { $$ = new_expr(MakeGreaterThan(take_expr($1),  take_expr($3))); }
    | expr GEQ expr { $$ = new_expr(MakeGreaterEqual(take_expr($1), take_expr($3))); }
    | expr KW_AND expr { $$ = new_expr(MakeAnd(take_expr($1), take_expr($3))); }
    | expr KW_OR  expr { $$ = new_expr(MakeOr(take_expr($1),  take_expr($3))); }
    | KW_NOT expr      { $$ = new_expr(MakeNot(take_expr($2))); }
    | '(' expr ')'     { $$ = $2; }
    | FN_ABS '(' expr ')' { $$ = new_expr(MakeAbs(take_expr($3))); }
    | FN_ATN '(' expr ')' { $$ = new_expr(MakeAtn(take_expr($3))); }
    | FN_COS '(' expr ')' { $$ = new_expr(MakeCos(take_expr($3))); }
    | FN_EXP '(' expr ')' { $$ = new_expr(MakeExp(take_expr($3))); }
    | FN_INT '(' expr ')' { $$ = new_expr(MakeInt(take_expr($3))); }
    | FN_LOG '(' expr ')' { $$ = new_expr(MakeLog(take_expr($3))); }
    | FN_RND '(' expr ')' { $$ = new_expr(MakeRnd(take_expr($3))); }
    | FN_SGN '(' expr ')' { $$ = new_expr(MakeSgn(take_expr($3))); }
    | FN_SIN '(' expr ')' { $$ = new_expr(MakeSin(take_expr($3))); }
    | FN_SQR '(' expr ')' { $$ = new_expr(MakeSqr(take_expr($3))); }
    | FN_TAN '(' expr ')' { $$ = new_expr(MakeTan(take_expr($3))); }
    | FN_LEN '(' expr ')' { $$ = new_expr(MakeLen(take_expr($3))); }
    | FN_ASC '(' expr ')' { $$ = new_expr(MakeAsc(take_expr($3))); }
    | FN_CHR '(' expr ')' { $$ = new_expr(MakeChr(take_expr($3))); }
    | FN_STR '(' expr ')' { $$ = new_expr(MakeStr(take_expr($3))); }
    | FN_VAL '(' expr ')' { $$ = new_expr(MakeVal(take_expr($3))); }
    | FN_SPC '(' expr ')' { $$ = new_expr(MakeSpc(take_expr($3))); }
    | FN_LEFT  '(' expr ',' expr ')' { $$ = new_expr(MakeLeft(take_expr($3), take_expr($5))); }
    | FN_RIGHT '(' expr ',' expr ')' { $$ = new_expr(MakeRight(take_expr($3), take_expr($5))); }
    | FN_MID   '(' expr ',' expr ')' { $$ = new_expr(MakeMid(take_expr($3), take_expr($5))); }
    | FN_MID   '(' expr ',' expr ',' expr ')' { $$ = new_expr(MakeMid(take_expr($3), take_expr($5), take_expr($7))); }
    | '-' expr %prec UMINUS {
        $$ = new_expr(MakeSub(
            MakeLiteral(Value(0)),
            take_expr($2)));
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
