%{
    #include "lex.yy.c"
    #include "ast.h"
    //#include "semantic.cpp"
    #include "ir_codegen.hpp"
    #include "ir_optimizer.hpp"
    #include "ir_inliner.hpp"
    int errorstatus = 0;
    int errlineno = 0;
    void yyerror(const char*);
    AST* root = NULL;
%}

%union {
    struct AbstractSyntaxTree *val;
    int type;
}

%token TYPE
%token ID INT FLOAT CHAR
%token STRUCT
%token IF THEN ELSE DO WHILE FOR
%token CONTINUE BREAK RETURN
%token DOT SEMI COMMA
%token ASSIGN NOT UMINUS
%token LT LE GT GE NE EQ
%token PLUS MINUS MUL DIV
%token AND OR
%token LP RP LB RB LC RC

%right ASSIGN
%left OR
%left AND
%left LT LE GT GE NE EQ
%left PLUS MINUS
%left MUL DIV
%right UMINUS NOT
%left ID
%left LP RP LB RB DOT
%right THEN ELSE

%type <type> TYPE
%type <val> ID INT FLOAT CHAR
%type <val> Program ExtDefList ExtDef ExtDecList Specifier StructSpecifier
%type <val> VarDec FunDec VarList ParamDec CompSt
%type <val> StmtList Stmt DefList Def DecList Dec Exp OptionalExp Args
%%

TopLevel
    : Program { root = $1; }
    ;
Program
    : ExtDefList { $$ = makeAST(PROGRAM, @$.first_line); insertChild($$, $1); }
    ;
ExtDefList
    : ExtDef ExtDefList { $$ = makeAST(EXTDEFLIST, @$.first_line); insertChild($$, $1); insertChild($$, $2); }
    | %empty { $$ = NULL; }
    ;
ExtDef
    : Specifier ExtDecList SEMI {
        $$ = makeAST(EXTDEF, @$.first_line);
        insertChild($$, $1);
        insertChild($$, $2);
        insertChild($$, makeSign(SEMI_SIGN));
    }
    | Specifier SEMI {
        $$ = makeAST(EXTDEF, @$.first_line);
        insertChild($$, $1);
        insertChild($$, makeSign(SEMI_SIGN));
    }
    | Specifier FunDec CompSt {
        $$ = makeAST(EXTDEF, @$.first_line);
        insertChild($$, $1);
        insertChild($$, $2);
        insertChild($$, $3);
    }
    ;
ExtDecList
    : VarDec { $$ = makeAST(EXTDECLIST, @$.first_line); insertChild($$, $1); }
    | VarDec COMMA ExtDecList {
        $$ = makeAST(EXTDECLIST, @$.first_line);
        insertChild($$, $1);
        insertChild($$, makeSign(COMMA_SIGN));
        insertChild($$, $3);
    }
    ;

Specifier
    : TYPE { $$ = makeAST(SPECIFIER, @$.first_line); $$->val = $1; }
    | StructSpecifier { $$ = makeAST(SPECIFIER, @$.first_line); insertChild($$, $1); }
    ;
StructSpecifier
    : STRUCT ID LC DefList RC {
        $$ = makeAST(STRUCTSPECIFIER, @$.first_line);
        insertChild($$, makeSign(STRUCT_SIGN));
        insertChild($$, $2);
        insertChild($$, makeSign(LC_SIGN));
        insertChild($$, $4);
        insertChild($$, makeSign(RC_SIGN));
    }
    | STRUCT ID {
        $$ = makeAST(STRUCTSPECIFIER, @$.first_line);
        insertChild($$, makeSign(STRUCT_SIGN));
        insertChild($$, $2);
    }
    ;

VarDec
    : ID { $$ = makeAST(VARDEC, @$.first_line); insertChild($$, $1); }
    | VarDec LB INT RB {
        $$ = makeAST(VARDEC, @$.first_line);
        insertChild($$, $1);
        insertChild($$, makeSign(LB_SIGN));
        insertChild($$, $3);
        insertChild($$, makeSign(RB_SIGN));
    }
FunDec
    : ID LP VarList RP {
        $$ = makeAST(FUNDEC, @$.first_line);
        insertChild($$, $1);
        insertChild($$, makeSign(LP_SIGN));
        insertChild($$, $3);
        insertChild($$, makeSign(RP_SIGN));
    }
    | ID LP RP { 
        $$ = makeAST(FUNDEC, @$.first_line);
        insertChild($$, $1);
        insertChild($$, makeSign(LP_SIGN));
        insertChild($$, makeSign(RP_SIGN));
    }
    ;
VarList
    : ParamDec COMMA VarList {
        $$ = makeAST(VARLIST, @$.first_line);
        insertChild($$, $1);
        insertChild($$, makeSign(COMMA_SIGN));
        insertChild($$, $3);
    }
    | ParamDec { $$ = makeAST(VARLIST, @$.first_line); insertChild($$, $1); }
    ;
ParamDec
    : Specifier VarDec {
        $$ = makeAST(PARAMDEC, @$.first_line);
        insertChild($$, $1);
        insertChild($$, $2);
    }
    ;

CompSt
    : LC DefList StmtList RC {
        $$ = makeAST(COMPST, @$.first_line);
        insertChild($$, makeSign(LC_SIGN));
        insertChild($$, $2);
        insertChild($$, $3);
        insertChild($$, makeSign(RC_SIGN));
    }
    ;
StmtList
    : Stmt StmtList { $$ = makeAST(STMTLIST, @$.first_line); insertChild($$, $1); insertChild($$, $2); }
    | %empty { $$ = NULL; }
    ;
Stmt
    : Exp SEMI { $$ = makeAST(STMT, @$.first_line); insertChild($$, $1); insertChild($$, makeSign(SEMI_SIGN)); }
    | CompSt { $$ = makeAST(STMT, @$.first_line); insertChild($$, $1); }
    | CONTINUE SEMI {
        $$ = makeAST(STMT, @$.first_line);
        insertChild($$, makeSign(CONTINUE_SIGN));
        insertChild($$, makeSign(SEMI_SIGN));
    }
    | BREAK SEMI {
        $$ = makeAST(STMT, @$.first_line);
        insertChild($$, makeSign(BREAK_SIGN));
        insertChild($$, makeSign(SEMI_SIGN));
    }
    | RETURN Exp SEMI {
        $$ = makeAST(STMT, @$.first_line);
        insertChild($$, makeSign(RETURN_SIGN));
        insertChild($$, $2);
        insertChild($$, makeSign(SEMI_SIGN));
    }
    | IF LP Exp RP Stmt %prec THEN {
        $$ = makeAST(STMT, @$.first_line);
        insertChild($$, makeSign(IF_SIGN));
        insertChild($$, makeSign(LP_SIGN));
        insertChild($$, $3);
        insertChild($$, makeSign(RP_SIGN));
        insertChild($$, $5);
    }
    | IF LP Exp RP Stmt ELSE Stmt {
        $$ = makeAST(STMT, @$.first_line);
        insertChild($$, makeSign(IF_SIGN));
        insertChild($$, makeSign(LP_SIGN));
        insertChild($$, $3);
        insertChild($$, makeSign(RP_SIGN));
        insertChild($$, $5);
        insertChild($$, makeSign(ELSE_SIGN));
        insertChild($$, $7);
    }
    | DO Stmt WHILE LP Exp RP SEMI {
        $$ = makeAST(STMT, @$.first_line);
        insertChild($$, makeSign(DO_SIGN));
        insertChild($$, $2);
        insertChild($$, makeSign(WHILE_SIGN));
        insertChild($$, makeSign(LP_SIGN));
        insertChild($$, $5);
        insertChild($$, makeSign(RP_SIGN));
        insertChild($$, makeSign(SEMI_SIGN));
    }
    | WHILE LP Exp RP Stmt {
        $$ = makeAST(STMT, @$.first_line);
        insertChild($$, makeSign(WHILE_SIGN));
        insertChild($$, makeSign(LP_SIGN));
        insertChild($$, $3);
        insertChild($$, makeSign(RP_SIGN));
        insertChild($$, $5);
    }
    | FOR LP OptionalExp SEMI OptionalExp SEMI OptionalExp RP Stmt {
        $$ = makeAST(STMT, @$.first_line);
        insertChild($$, makeSign(FOR_SIGN));
        insertChild($$, makeSign(LP_SIGN));
        insertChild($$, $3);
        insertChild($$, makeSign(SEMI_SIGN));
        insertChild($$, $5);
        insertChild($$, makeSign(SEMI_SIGN));
        insertChild($$, $7);
        insertChild($$, makeSign(RP_SIGN));
        insertChild($$, $9);
    }
    ;

DefList
    : Def DefList { $$ = makeAST(DEFLIST, @$.first_line); insertChild($$, $1); insertChild($$, $2); }
    | %empty { $$ = NULL; }
    ;
Def
    : Specifier DecList SEMI {
        $$ = makeAST(DEF, @$.first_line);
        insertChild($$, $1);
        insertChild($$, $2);
        insertChild($$, makeSign(SEMI_SIGN));
    }
    ;
DecList
    : Dec { $$ = makeAST(DECLIST, @$.first_line); insertChild($$, $1); }
    | Dec COMMA DecList {
        $$ = makeAST(DECLIST, @$.first_line);
        insertChild($$, $1);
        insertChild($$, makeSign(COMMA_SIGN));
        insertChild($$, $3);
    }
    ;
Dec
    : VarDec { $$ = makeAST(DEC, @$.first_line); insertChild($$, $1); }
    | VarDec ASSIGN Exp {
        $$ = makeAST(DEC, @$.first_line);
        insertChild($$, $1);
        insertChild($$, makeSign(ASSIGN_OP));
        insertChild($$, $3);
    }
    ;

OptionalExp
    : Exp { $$ = $1; }
    | %empty { $$ = makeSign(NOP_SIGN); }
    ;
Exp
    : Exp ASSIGN Exp {
        $$ = makeAST(EXP, @$.first_line);
        insertChild($$, $1);
        insertChild($$, makeSign(ASSIGN_OP));
        insertChild($$, $3);
    }
    | Exp AND Exp {
        $$ = makeAST(EXP, @$.first_line);
        insertChild($$, $1);
        insertChild($$, makeSign(AND_OP));
        insertChild($$, $3);
    }
    | Exp OR Exp {
        $$ = makeAST(EXP, @$.first_line);
        insertChild($$, $1);
        insertChild($$, makeSign(OR_OP));
        insertChild($$, $3);
    }
    | Exp LT Exp {
        $$ = makeAST(EXP, @$.first_line);
        insertChild($$, $1);
        insertChild($$, makeSign(LT_OP));
        insertChild($$, $3);
    }
    | Exp LE Exp {
        $$ = makeAST(EXP, @$.first_line);
        insertChild($$, $1);
        insertChild($$, makeSign(LE_OP));
        insertChild($$, $3);
    }
    | Exp GT Exp {
        $$ = makeAST(EXP, @$.first_line);
        insertChild($$, $1);
        insertChild($$, makeSign(GT_OP));
        insertChild($$, $3);
    }
    | Exp GE Exp {
        $$ = makeAST(EXP, @$.first_line);
        insertChild($$, $1);
        insertChild($$, makeSign(GE_OP));
        insertChild($$, $3);
    }
    | Exp NE Exp {
        $$ = makeAST(EXP, @$.first_line);
        insertChild($$, $1);
        insertChild($$, makeSign(NE_OP));
        insertChild($$, $3);
    }
    | Exp EQ Exp {
        $$ = makeAST(EXP, @$.first_line);
        insertChild($$, $1);
        insertChild($$, makeSign(EQ_OP));
        insertChild($$, $3);
    }
    | Exp PLUS Exp {
        $$ = makeAST(EXP, @$.first_line);
        insertChild($$, $1);
        insertChild($$, makeSign(PLUS_OP));
        insertChild($$, $3);
    }
    | Exp MINUS Exp {
        $$ = makeAST(EXP, @$.first_line);
        insertChild($$, $1);
        insertChild($$, makeSign(MINUS_OP));
        insertChild($$, $3);
    }
    | Exp MUL Exp {
        $$ = makeAST(EXP, @$.first_line);
        insertChild($$, $1);
        insertChild($$, makeSign(MUL_OP));
        insertChild($$, $3);
    }
    | Exp DIV Exp {
        $$ = makeAST(EXP, @$.first_line);
        insertChild($$, $1);
        insertChild($$, makeSign(DIV_OP));
        insertChild($$, $3);
    }
    | LP Exp RP {
        $$ = makeAST(EXP, @$.first_line);
        insertChild($$, makeSign(LP_SIGN));
        insertChild($$, $2);
        insertChild($$, makeSign(RP_SIGN));
    }
    | MINUS Exp %prec UMINUS {
        $$ = makeAST(EXP, @$.first_line);
        insertChild($$, makeSign(MINUS_OP));
        insertChild($$, $2);
    }
    | NOT Exp {
        $$ = makeAST(EXP, @$.first_line);
        insertChild($$, makeSign(NOT_OP));
        insertChild($$, $2);
    }
    | ID LP Args RP {
        $$ = makeAST(EXP, @$.first_line);
        insertChild($$, $1);
        insertChild($$, makeSign(LP_SIGN));
        insertChild($$, $3);
        insertChild($$, makeSign(RP_SIGN));
    }
    | ID LP RP {
        $$ = makeAST(EXP, @$.first_line);
        insertChild($$, $1);
        insertChild($$, makeSign(LP_SIGN));
        insertChild($$, makeSign(RP_SIGN));
    }
    | Exp LB Exp RB {
        $$ = makeAST(EXP, @$.first_line);
        insertChild($$, $1);
        insertChild($$, makeSign(LB_SIGN));
        insertChild($$, $3);
        insertChild($$, makeSign(RB_SIGN));
    }
    | Exp DOT ID { 
        $$ = makeAST(EXP, @$.first_line);
        insertChild($$, $1);
        insertChild($$, makeSign(DOT_SIGN));
        insertChild($$, $3);
    }
    | ID { $$ = makeAST(EXP, @$.first_line); insertChild($$, $1); }
    | INT { $$ = makeAST(EXP, @$.first_line); insertChild($$, $1); }
    | FLOAT { $$ = makeAST(EXP, @$.first_line); insertChild($$, $1); }
    | CHAR { $$ = makeAST(EXP, @$.first_line); insertChild($$, $1); }
    ;
Args
    : Exp COMMA Args {
        $$ = makeAST(ARGS, @$.first_line);
        insertChild($$, $1);
        insertChild($$, makeSign(COMMA_SIGN));
        insertChild($$, $3);
    }
    | Exp { 
        $$ = makeAST(ARGS, @$.first_line);
        insertChild($$, $1);
    }
    ;

%%

void yyerror(const char* msg) {
    if (errlineno && yylineno > errlineno) {
        printf("Error type B at Line %d: syntax error\n", errlineno);
        errlineno = yylineno;
    }
    if (msg && strcmp(msg, "syntax error")) {
        printf("Error type B at Line %d: %s\n", yylineno, msg);
        errlineno = 0;
    } else {
        errlineno = yylineno;
    }
    errorstatus = 1;
}

int main(int argc, char** argv) {
    if(argc != 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        exit(-1);
    }
    else if(!(yyin = fopen(argv[1], "r"))) {
        perror(argv[1]);
        exit(-1);
    }
    yyparse();
    if (errorstatus) {
        yylineno++;
        yyerror(NULL);
    }
    if (!errorstatus && root) {
        //printAST(root, 0);
        //char* output_path = (char*) calloc(strlen(argv[1]) + 4, sizeof(char));
        //strcpy(output_path, argv[1]);
        //char* ext_ptr = strrchr(output_path, '.');
        //*ext_ptr = '\0';
        //strcat(output_path, ".out");
        //if (!freopen(output_path, "w", stdout)) {
        //    perror(output_path);
        //    exit(-1);
        //}
        //initHandlers();
        //visitNode(root);
        Code* head = translateCode(root);
        irOptimize(head);
        irInline(head);
        irOptimize(head);
        irPrint(head);
    }
    return 0;
}
