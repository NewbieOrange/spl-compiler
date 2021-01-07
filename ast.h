#pragma once
#include <cstdio>
#include <cstring>
#include <string>
#define TYPE_INT 0
#define TYPE_FLOAT 1
#define TYPE_CHAR 2

enum opr {
    PROGRAM,
    EXTDEFLIST,
    EXTDEF,
    EXTDECLIST,
    SPECIFIER,
    STRUCTSPECIFIER,
    VARDEC,
    FUNDEC,
    VARLIST,
    PARAMDEC,
    COMPST,
    STMTLIST,
    STMT,
    DEFLIST,
    DEF,
    DECLIST,
    DEC,
    EXP,
    ARGS,
    SYMBOL,
    STRUCT_SIGN,
    INT_CONST,
    FLOAT_CONST,
    CHAR_CONST,
    IF_SIGN,
    ELSE_SIGN,
    DO_SIGN,
    WHILE_SIGN,
    FOR_SIGN,
    CONTINUE_SIGN,
    BREAK_SIGN,
    RETURN_SIGN,
    AND_OP,
    OR_OP,
    NOT_OP,
    PLUS_OP,
    MINUS_OP,
    MUL_OP,
    DIV_OP,
    ASSIGN_OP,
    LT_OP,
    LE_OP,
    GT_OP,
    GE_OP,
    NE_OP,
    EQ_OP,
    LP_SIGN,
    RP_SIGN,
    LB_SIGN,
    RB_SIGN,
    LC_SIGN,
    RC_SIGN,
    DOT_SIGN,
    SEMI_SIGN,
    COMMA_SIGN,
    NOP_SIGN
};

const char* oprName(enum opr op) {
    switch (op) {
        case PROGRAM:
            return "Program";
        case EXTDEFLIST:
            return "ExtDefList";
        case EXTDEF:
            return "ExtDef";
        case EXTDECLIST:
            return "ExtDecList";
        case SPECIFIER:
            return "Specifier";
        case STRUCTSPECIFIER:
            return "StructSpecifier";
        case VARDEC:
            return "VarDec";
        case FUNDEC:
            return "FunDec";
        case VARLIST:
            return "VarList";
        case PARAMDEC:
            return "ParamDec";
        case COMPST:
            return "CompSt";
        case STMTLIST:
            return "StmtList";
        case STMT:
            return "Stmt";
        case DEFLIST:
            return "DefList";
        case DEF:
            return "Def";
        case DECLIST:
            return "DecList";
        case DEC:
            return "Dec";
        case EXP:
            return "Exp";
        case ARGS:
            return "Args";
        case SYMBOL:
            return "ID";
        case STRUCT_SIGN:
            return "STRUCT";
        case INT_CONST:
            return "INT";
        case FLOAT_CONST:
            return "FLOAT";
        case CHAR_CONST:
            return "CHAR";
        case IF_SIGN:
            return "IF";
        case ELSE_SIGN:
            return "ELSE";
        case DO_SIGN:
            return "DO";
        case WHILE_SIGN:
            return "WHILE";
        case FOR_SIGN:
            return "FOR";
        case CONTINUE_SIGN:
            return "CONTINUE";
        case BREAK_SIGN:
            return "BREAK";
        case RETURN_SIGN:
            return "RETURN";
        case AND_OP:
            return "AND";
        case OR_OP:
            return "OR";
        case NOT_OP:
            return "NOT";
        case PLUS_OP:
            return "PLUS";
        case MINUS_OP:
            return "MINUS";
        case MUL_OP:
            return "MUL";
        case DIV_OP:
            return "DIV";
        case ASSIGN_OP:
            return "ASSIGN";
        case LT_OP:
            return "LT";
        case LE_OP:
            return "LE";
        case GT_OP:
            return "GT";
        case GE_OP:
            return "GE";
        case NE_OP:
            return "NE";
        case EQ_OP:
            return "EQ";
        case LP_SIGN:
            return "LP";
        case RP_SIGN:
            return "RP";
        case LB_SIGN:
            return "LB";
        case RB_SIGN:
            return "RB";
        case LC_SIGN:
            return "LC";
        case RC_SIGN:
            return "RC";
        case DOT_SIGN:
            return "DOT";
        case SEMI_SIGN:
            return "SEMI";
        case COMMA_SIGN:
            return "COMMA";
        default:
            return "Unknown";
    }
}

const char* typeName(int type) {
    switch (type) {
        case TYPE_INT:
            return "int";
        case TYPE_FLOAT:
            return "float";
        case TYPE_CHAR:
            return "char";
        default:
            return "unknown";
    }
}

typedef struct AbstractSyntaxTree {
    int lineno;
    enum opr op;
    int val;
    char* str;
    struct AbstractSyntaxTree** children;
    int num_children;
    
    std::string to_string() {
        std::string result = std::string(oprName(op)) + "_";
        for (int i = 0; i < num_children; i++) {
            result += oprName(children[i]->op);
        }
        return result;
    }
} AST;

AST* makeAST(enum opr op, int lineno) {
    AST* ast = (AST*) malloc(sizeof(AST));
    memset(ast, 0, sizeof(AST));
    ast->lineno = lineno;
    ast->op = op;
    return ast;
}

AST* makeSign(enum opr sign) {
    return makeAST(sign, 0);
}

AST* makeSymbol(char *name, int lineno) {
    AST* ast = makeAST(SYMBOL, lineno);
    ast->str = strdup(name);
    return ast;
}

AST* makeInt(int val, int lineno) {
    AST* ast = makeAST(INT_CONST, lineno);
    ast->val = val;
    return ast;
}

AST* makeFloat(char *str, int lineno) {
    AST* ast = makeAST(FLOAT_CONST, lineno);
    ast->str = strdup(str);
    return ast;
}

AST* makeChar(char *str, int lineno) {
    AST* ast = makeAST(CHAR_CONST, lineno);
    ast->str = strdup(str);
    return ast;
}

AST* insertChild(AST* parent, AST* element) {
    if (element) {
        parent->children = (AST**) realloc(parent->children, ++parent->num_children * sizeof(AST));
        parent->children[parent->num_children - 1] = element;
    }
}

void printIndent(int depth) {
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
}

void printAST(const AST* ast, int depth) {
    printIndent(depth);
    if (ast->op == SYMBOL) {
        printf("ID: %s\n", ast->str);
    } else {
        if (ast->op == INT_CONST) {
            printf("INT: %d\n", ast->val);
        }
        else if (ast->op == FLOAT_CONST) {
            printf("FLOAT: %s\n", ast->str);
        }
        else if (ast->op == CHAR_CONST) {
            printf("CHAR: %s\n", ast->str);
        }
        else if (ast->lineno) {
            printf("%s (%d)\n", oprName(ast->op), ast->lineno);
        } else {
            printf("%s\n", oprName(ast->op));
        }
        if (ast->op == SPECIFIER && !ast->children) {
            printIndent(depth + 1);
            printf("TYPE: %s\n", typeName(ast->val));
        }
    }
    for (int i = 0; i < ast->num_children; i++) {
        printAST(ast->children[i], depth + 1);
    }
}
