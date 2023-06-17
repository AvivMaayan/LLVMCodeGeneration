%{
    #include "source.hpp"
    #include "hw3_output.hpp"
    #include "parser.tab.hpp"
%}

%option yylineno
%option noyywrap
%%

void                           yylval=new RetType(yytext); return VOID;
int                            yylval=new Type(yytext); return INT;
byte                           yylval=new Type(yytext); return BYTE;
b                              return B;
bool                           yylval=new Type(yytext); return BOOL;
and                            yylval=new BoolOp(yytext); return AND;
or                             yylval=new BoolOp(yytext); return OR;
not                            return NOT;
true                           yylval=new Exp("bool", yytext); return TRUE;
false                          yylval=new Exp("bool", yytext); return FALSE;
return                         return RETURN;
if                             return IF;
else                           return ELSE;
while                          return WHILE;
break                          return BREAK;
continue                       return CONTINUE;
override                       return OVERRIDE;
;                              return SC;
,                              return COMMA;
\(                             return LPAREN;
\)                             return RPAREN;
\{                             return LBRACE;
\}                             return RBRACE;
=                              return ASSIGN;
==|!=|<|>|<=|>=                yylval=new RelOp(yytext); return RELOP;
\+|\-                          yylval=new BinOp(yytext); return BINSUBSUM;
\*|\/                          yylval=new BinOp(yytext); return BINMULDIV;
[a-zA-Z][a-zA-Z0-9]*           yylval=new Id(yytext); return ID;
0|[1-9][0-9]*                  yylval=new RawNumber(yytext); return NUM;
\"([^\n\r\"\\]|\\[rnt"\\])+\"  yylval=new Exp("string", yytext); return STRING;
\/\/[^\r\n]*[\r|\n|\r\n]?      ;
[\t\n\r ]                      ;
.                              {output::errorLex(yylineno); exit(1);}
%%
