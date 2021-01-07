%{
    #define YY_USER_ACTION yylloc.first_line = yylloc.last_line = yylineno;
    #include "syntax.tab.h"
    #include "ast.h"
    #include <cstdio>
    #include <cstdlib>
    
    extern void yyerror(const char*);
    extern int errorstatus;
    void blockComments();
    int filedepth = 0;
%}

oct [0-7]
dec [0-9]
nzdec [1-9]
letter [a-zA-Z_]
alphabet [a-zA-Z_0-9]
hex [a-fA-F0-9]
nzhex [a-fA-F1-9]
hexpref (0[xX])
empty [ \t\v\r\n\f]

%option yylineno noyywrap

%%
(\#include[ ]*["].*["]) {
    char* buffer = strdup(yytext);
    int start = 0, end = strlen(yytext);
    for (int i = start; i < end; i++) {
        if (buffer[i] == '"') {
            start = i + 1;
            break;
        }
    }
    for (int i = end; i > start; i--) {
        if (buffer[i] == '"') {
            end = i;
            break;
        }
    }
    buffer[end] = '\0';
    char* filename = strdup(buffer + start);
    free(buffer);
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: included file %s not found\n", filename);
        errorstatus = 1;
    } else {
        filedepth++;
        yypush_buffer_state(yy_create_buffer(file, YY_BUF_SIZE));
    }
}
<<EOF>> { if (filedepth--) yypop_buffer_state(); else yyterminate(); }
"/*" { blockComments(); }
"//".* { }
"int" { yylval.type = TYPE_INT; return TYPE; }
"float" { yylval.type = TYPE_FLOAT; return TYPE; }
"char" { yylval.type = TYPE_CHAR; return TYPE; }
"struct" { return STRUCT; }
"if" { return IF; }
"else" { return ELSE; }
"do" { return DO; }
"while" { return WHILE; }
"for" { return FOR; }
"continue" { return CONTINUE; }
"break" { return BREAK; }
"return" { return RETURN; }
"." { return DOT; }
";" { return SEMI; }
"," { return COMMA; }
"=" { return ASSIGN; }
"<" { return LT; }
"<=" { return LE; }
">" { return GT; }
">=" { return GE; }
"!=" { return NE; }
"==" { return EQ; }
"+" { return PLUS; }
"-" { return MINUS; }
"*" { return MUL; }
"/" { return DIV; }
"&&" { return AND; }
"||" { return OR; }
"!" { return NOT; }
"(" { return LP; }
")" { return RP; }
"[" { return LB; }
"]" { return RB; }
"{" { return LC; }
"}" { return RC; }

{letter}{alphabet}* { yylval.val = makeSymbol(yytext, yylineno); return ID; }
({nzdec}{dec}*|"0") { yylval.val = makeInt(strtol(yytext, NULL, 10), yylineno); return INT; }
({hexpref}{nzhex}{hex}+|{hexpref}"0") { yylval.val = makeInt(strtol(yytext, NULL, 16), yylineno); return INT; }
"'"([^'\\\n]|\\x{hex}{hex})"'" { yylval.val = makeChar(yytext, yylineno); return CHAR; }

{dec}+"."{dec}+ { yylval.val = makeFloat(yytext, yylineno); return FLOAT; }

{empty}+ {}
{dec}+ { printf("Error type A at Line %d: ill-formed integer literal \'%s\'\n", yylineno, yytext); errorstatus = 1; return INT; }
({hexpref}{alphabet}+) { printf("Error type A at Line %d: ill-formed integer literal \'%s\'\n", yylineno, yytext); errorstatus = 1; return INT; }
"'"([^'\\\n]{2,}|\\x{alphabet}*)"'" { printf("Error type A at Line %d: ill-formed char literal %s\n", yylineno, yytext); errorstatus = 1; return CHAR; }
(({dec}+".")|("."{dec}+)) { printf("Error type A at Line %d: ill-formed float literal %s\n", yylineno, yytext); errorstatus = 1; return FLOAT; }
. { printf("Error type A at Line %d: unknown lexeme \'%s\'\n", yylineno, yytext); errorstatus = 1; return ID; }

%%

void blockComments() {
	int c;
	while ((c = yyinput()) != EOF) {
		if (c == '*') {
			while ((c = yyinput()) == '*') ;
			if (c == '/') return;
			if (c == EOF) break;
		}
	}
	printf("Error type A at Line %d: unterminated comment\n", yylineno);
	errorstatus = 1;
}
