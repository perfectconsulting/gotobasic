%{
#include "basic.tab.hpp"
#include <cstdlib>
#include <string>
#include <cctype>

std::string yyinput_line;
int yycolumn = 1;

#define YY_USER_ACTION \
    yycolumn += yyleng;

static std::string to_upper(const char* s) {
    std::string r(s);
    for (auto& c : r) c = toupper((unsigned char)c);
    return r;
}

static int keyword_token(const std::string& w) {
    if      (w == "LET")       return KW_LET;
    else if (w == "PRINT")     return KW_PRINT;
    else if (w == "INPUT")     return KW_INPUT;
    else if (w == "IF")        return KW_IF;
    else if (w == "THEN")      return KW_THEN;
    else if (w == "ELSE")      return KW_ELSE;
    else if (w == "GOTO")      return KW_GOTO;
    else if (w == "GOSUB")     return KW_GOSUB;
    else if (w == "RETURN")    return KW_RETURN;
    else if (w == "FOR")       return KW_FOR;
    else if (w == "TO")        return KW_TO;
    else if (w == "STEP")      return KW_STEP;
    else if (w == "NEXT")      return KW_NEXT;
    else if (w == "END")       return KW_END;
    else if (w == "STOP")      return KW_STOP;
    else if (w == "LIST")      return KW_LIST;
    else if (w == "RUN")       return KW_RUN;
    else if (w == "DELETE")    return KW_DELETE;
    else if (w == "EXIT")      return KW_EXIT;
    else if (w == "LOAD")      return KW_LOAD;
    else if (w == "SAVE")      return KW_SAVE;
    else if (w == "DIM")       return KW_DIM;
    else if (w == "DEF")       return KW_DEF;
    else if (w == "ON")        return KW_ON;
    else if (w == "DATA")      return KW_DATA;
    else if (w == "READ")      return KW_READ;
    else if (w == "RESTORE")   return KW_RESTORE;
    else if (w == "RANDOMIZE") return KW_RANDOMIZE;
    else if (w == "CLEAR")     return KW_CLEAR;
    else if (w == "NEW")       return KW_NEW;
    else if (w == "CLS")       return KW_CLS;
    else if (w == "WHILE")     return KW_WHILE;
    else if (w == "WEND")      return KW_WEND;
    else if (w == "TAB")       return FN_TAB;
    else if (w == "SPC")       return FN_SPC;
    else if (w == "AND")       return KW_AND;
    else if (w == "OR")        return KW_OR;
    else if (w == "NOT")       return KW_NOT;
    else if (w == "ABS")       return FN_ABS;
    else if (w == "ATN")       return FN_ATN;
    else if (w == "COS")       return FN_COS;
    else if (w == "EXP")       return FN_EXP;
    else if (w == "INT")       return FN_INT;
    else if (w == "LOG")       return FN_LOG;
    else if (w == "RND")       return FN_RND;
    else if (w == "SGN")       return FN_SGN;
    else if (w == "SIN")       return FN_SIN;
    else if (w == "SQR")       return FN_SQR;
    else if (w == "TAN")       return FN_TAN;
    else if (w == "LEN")       return FN_LEN;
    else if (w == "ASC")       return FN_ASC;
    else if (w == "VAL")       return FN_VAL;
    else if (w == "TRON")      return KW_TRON;
    else if (w == "TROFF")     return KW_TROFF;    
    return 0;
}
%}

%option noyywrap
%option always-interactive
%option case-insensitive

%%

"REM"[^\n]*  { yylval.sval = new std::string(yytext + 3); return KW_REM; }

"CHR$"      { return FN_CHR;  }
"STR$"      { return FN_STR;  }
"LEFT$"     { return FN_LEFT; }
"RIGHT$"    { return FN_RIGHT;}
"MID$"      { return FN_MID;  }

[A-Za-z][A-Za-z0-9_]*"$" {
    yylval.sval = new std::string(yytext);
    return STRIDENTIFIER;
}

"FN"[A-Za-z][A-Za-z0-9]* {
    yylval.sval = new std::string(yytext);
    return FNIDENTIFIER;
}

[A-Za-z][A-Za-z0-9_]* {
    std::string w = to_upper(yytext);
    int kw = keyword_token(w);
    if (kw) return kw;
    for (int len = (int)w.size() - 1; len >= 1; --len) {
        kw = keyword_token(w.substr(0, len));
        if (kw) { yyless(len); return kw; }
    }
    yylval.sval = new std::string(yytext);
    return IDENTIFIER;
}

[0-9]+          { yylval.ival = std::stoi(yytext); return INTEGER; }
[0-9]+\.[0-9]+  { yylval.dval = std::stof(yytext); return FLOAT; }
\.[0-9]+        { yylval.dval = std::stof(yytext); return FLOAT; }

\"[^\"]*\" {
    std::string s(yytext + 1, yyleng - 2);
    yylval.sval = new std::string(s);
    return STRING;
}

[ \t]+      ;
\n          { yycolumn = 1; return '\n'; }

"<>"        { return NEQ; }
"<="        { return LEQ; }
">="        { return GEQ; }
"<"         { return '<'; }
">"         { return '>'; }
"="         { return '='; }
"+"         { return '+'; }
"-"         { return '-'; }
"*"         { return '*'; }
"/"         { return '/'; }
"^"         { return '^'; }
"("         { return '('; }
")"         { return ')'; }
","         { return ','; }
";"         { return ';'; }
":"         { return ':'; }

.           { return yytext[0]; }

%%
