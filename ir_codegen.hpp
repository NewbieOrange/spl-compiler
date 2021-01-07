#pragma once
#include "ast.h"
#include "ir.hpp"

static Value* lookupVariable(char* name) {
    static int counter = 1;
    auto iter = symbol_table.find(name);
    if (iter == symbol_table.end()) {
        Value* ptr = makeVV(counter++);
        symbol_table[name] = ptr;
        return ptr;
    } else {
        return iter->second;
    }
}

static Value* makeTemp() {
    static int counter = 1;
    return makeTV(counter++);
}

static Value* makePointer() {
    static int counter = 1;
    return makePV(counter++);
}

static Value* makeLabel() {
    static int counter = 1;
    return makeLV(counter++);
}

Code* translateExp(AST* exp, Value* &temp);

Code* translateCode(AST* ast, Value* contLabel = nullptr, Value* breakLabel = nullptr);

Code* translateArgs(AST* args, std::vector<Value*>& argList);

Code* translateCondExp(AST* exp, Value* lb_t, Value* lb_f) {
    IROpCode opcode;
    switch (exp->children[1]->op) {
            case AND_OP: opcode = IR_AND; break;
            case OR_OP: opcode = IR_OR; break;
            case LT_OP: opcode = IR_LT; break;
            case LE_OP: opcode = IR_LE; break;
            case GT_OP: opcode = IR_GT; break;
            case GE_OP: opcode = IR_GE; break;
            case NE_OP: opcode = IR_NE; break;
            case EQ_OP: opcode = IR_EQ; break;
            case NOT_OP: opcode = IR_NOT; break;
    }
    if (opcode == IR_AND) {
        Value* lb1 = makeLabel();
        Code* c1 = translateCondExp(exp->children[0], lb1, lb_f);
        Code* c2 = new Code(IR_LABEL, lb1);
        Code* c3 = translateCondExp(exp->children[2], lb_t, lb_f);
        return combineCode(combineCode(c1, c2), c3);
    } else if (opcode == IR_OR) {
        Value* lb1 = makeLabel();
        Code* c1 = translateCondExp(exp->children[0], lb_t, lb1);
        Code* c2 = new Code(IR_LABEL, lb1);
        Code* c3 = translateCondExp(exp->children[2], lb_t, lb_f);
        return combineCode(combineCode(c1, c2), c3);
    } else if (opcode == IR_NOT) {
        return translateCondExp(exp, lb_f, lb_t);
    } else {
        Value *t1 = makeTemp(), *t2 = makeTemp();
        Code* c1 = translateExp(exp->children[0], t1);
        Code* c2 = translateExp(exp->children[2], t2);
        Code* c3 = new Code(IR_IFGOTO, t1, t2, lb_t, opcode);
        Code* c4 = new Code(IR_GOTO, lb_f);
        return combineCode(combineCode(combineCode(c1, c2), c3), c4);
    }
}

Value* lookupVariable(AST* ast) {
    if (ast->num_children == 1) {
        return lookupVariable(ast->children[0]->str);
    } else { // Exp LB Exp RB
        return new Value(VT_COMPLEX, ast);
    }
}

Code* translateArray(AST* exp, Value*& temp, Array** ret_arr = nullptr, int* ret_depth = nullptr) {
    if (exp->num_children == 1) {
        if (exp->children[0]->op == INT_CONST) {
            return new Code(IR_MOVE, makeCV(exp->children[0]->val), temp);
        } else {
            if (ret_arr && ret_depth) {
                *ret_arr = array_table[exp->children[0]->str];
                *ret_depth = 0;
            }
            if ((*ret_arr)->param) {
                return new Code(IR_MOVE, lookupVariable(exp->children[0]->str), temp);
            } else {
                return new Code(IR_LOADADDR, lookupVariable(exp->children[0]->str), temp);
            }
        } 
    }
    Value* addr = makePointer();
    Value* offset = makePointer();
    Array* arr = nullptr;
    int depth = 0;
    Code* c1 = translateArray(exp->children[0], addr, &arr, &depth);
    if (ret_arr && ret_depth) {
        *ret_arr = arr;
        *ret_depth = depth + 1;
    }
    Code* c2 = translateExp(exp->children[2], offset);
    Code* c3 = new Code(IR_MUL, offset, makeCV(arr->sizes[depth]), offset);
    Code* c4 = new Code(IR_ADD, addr, offset, addr);
    Code* c5 = new Code(IR_MOVE, addr, temp);
    return combineCode(combineCode(combineCode(combineCode(c1, c2), c3), c4), c5);
}

Code* translateExp(AST* exp, Value* &temp) {
    std::string repr = exp->to_string();
    if (repr == "Exp_INT") {
        return new Code(IR_MOVE, makeCV(exp->children[0]->val), temp);
    } else if (repr == "Exp_ID") {
        if (temp->type == VT_TEMP) {
            temp = lookupVariable(exp->children[0]->str);
            return nullptr;
        }
        return new Code(IR_MOVE, lookupVariable(exp->children[0]->str), temp);
    } else if (repr == "Exp_ExpASSIGNExp") {
        Value* dest = lookupVariable(exp->children[0]);
        if (dest->type == VT_COMPLEX) {
            Value* addr = makePointer();
            Value* val = makePointer();
            Code* c1 = translateArray(exp->children[0], addr);
            Code* c2 = translateExp(exp->children[2], val);
            Code* c3 = new Code(IR_STORE, val, addr);
            return combineCode(combineCode(c1, c2), c3);
        }
        return translateExp(exp->children[2], dest);
    } else if (exp->num_children == 3 && exp->children[0]->op == EXP && exp->children[2]->op == EXP) {
        IROpCode opcode;
        bool conditional = false;
        switch (exp->children[1]->op) {
            case AND_OP: opcode = IR_AND; conditional = true; break;
            case OR_OP: opcode = IR_OR; conditional = true; break;
            case LT_OP: opcode = IR_LT; conditional = true; break;
            case LE_OP: opcode = IR_LE; conditional = true; break;
            case GT_OP: opcode = IR_GT; conditional = true; break;
            case GE_OP: opcode = IR_GE; conditional = true; break;
            case NE_OP: opcode = IR_NE; conditional = true; break;
            case EQ_OP: opcode = IR_EQ; conditional = true; break;
            case PLUS_OP: opcode = IR_ADD; break;
            case MINUS_OP: opcode = IR_MINUS; break;
            case MUL_OP: opcode = IR_MUL; break;
            case DIV_OP: opcode = IR_DIV; break;
        }
        if (conditional) {
            Value *lb1 = makeLabel(), *lb2 = makeLabel();
            Code* c1 = translateCondExp(exp, lb1, lb2);
            Code* c2 = new Code(IR_LABEL, lb1);
            Code* c3 = new Code(IR_MOVE, makeCV(1), temp);
            Code* c4 = new Code(IR_LABEL, lb2);
            Code* c5 = new Code(IR_MOVE, makeCV(0), temp);
            return combineCode(combineCode(combineCode(combineCode(c1, c2), c3), c4), c5);
        } else {
            Value *t1 = makeTemp(), *t2 = makeTemp();
            Code* c1 = translateExp(exp->children[0], t1);
            Code* c2 = translateExp(exp->children[2], t2);
            Code* c3 = new Code(opcode, t1, t2, temp);
            return combineCode(combineCode(c1, c2), c3);
        }
    } else if (repr == "Exp_MINUSExp") {
        Code* c1 = translateExp(exp->children[1], temp);
        Code* c2 = new Code(IR_MINUS, makeCV(0), temp, temp);
        return combineCode(c1, c2);
    } else if (repr == "Exp_NOTExp") {
        Value *lb1 = makeLabel(), *lb2 = makeLabel();
        Code* c1 = translateCondExp(exp, lb1, lb2);
        Code* c2 = new Code(IR_LABEL, lb1);
        Code* c3 = new Code(IR_MOVE, makeCV(1), temp);
        Code* c4 = new Code(IR_LABEL, lb2);
        Code* c5 = new Code(IR_MOVE, makeCV(0), temp);
        return combineCode(combineCode(combineCode(combineCode(c1, c2), c3), c4), c5);
    } else if (repr == "Exp_IDLPRP") {
        if (!strcmp(exp->children[0]->str, "read")) {
            return new Code(IR_READ, temp);
        } else {
            return new Code(IR_CALL, makeSV(exp->children[0]->str), temp);
        }
    } else if (repr == "Exp_IDLPArgsRP") {
        if (!strcmp(exp->children[0]->str, "write")) {
            Code* c1 = translateExp(exp->children[2]->children[0], temp);
            Code* c2 = new Code(IR_WRITE, temp);
            return combineCode(c1, c2);
        } else {
            std::vector<Value*> argList;
            Code* c1 = translateArgs(exp->children[2], argList);
            Code* c2 = nullptr;
            for (int i = argList.size() - 1; i >= 0; i--) { // reversed arglist
                if (symbol_array_table.find(argList[i]) != symbol_array_table.end()) {
                    Value* addr = makePointer();
                    c2 = combineCode(c2, new Code(IR_LOADADDR, argList[i], addr));
                    c2 = combineCode(c2, new Code(IR_ARG, addr));
                } else
                c2 = combineCode(c2, new Code(IR_ARG, argList[i]));
            }
            Code* c3 = new Code(IR_CALL, makeSV(exp->children[0]->str), temp);
            return combineCode(combineCode(c1, c2), c3);
        }
    } else if (repr == "Exp_ExpLBExpRB") {
        Value* addr = makePointer();
        Code* c1 = translateArray(exp, addr);
        Code* c2 = new Code(IR_LOAD, addr, temp);
        return combineCode(c1, c2);
    } else if (repr == "Exp_LPExpRP") {
        return translateExp(exp->children[1], temp);
    } else {
        return nullptr;
    }
}

Code* translateArgs(AST* args, std::vector<Value*>& argList) {
    if (args->num_children == 1) { // Exp
        Value* t1 = makeTemp();
        Code* c1 = translateExp(args->children[0], t1);
        argList.push_back(t1);
        return c1;
    } else { // Exp COMMA Args
        Value* t1 = makeTemp();
        Code* c1 = translateExp(args->children[0], t1);
        argList.push_back(t1);
        Code* c2 = translateArgs(args->children[2], argList);
        return combineCode(c1, c2);
    }
}

Code* translateVarDec(AST* ast, bool param = false) {
    Array* arr = makeArray(ast);
    if (arr->dimensions.empty()) {
        return nullptr; // not array, nop
    }
    for (int i = 0; i < arr->dimensions.size(); i++) {
        int size = 4;
        for (int j = i + 1; j < arr->dimensions.size(); j++) {
            size *= arr->dimensions[j];
        }
        arr->sizes.push_back(size);
    }
    arr->param = param;
    
    Value* val = lookupVariable(arr->name);
    array_table[arr->name] = arr;
    symbol_array_table[val] = arr;
    
    Code* c = new Code(IR_ALLOC, val);
    c->size = 4;
    for (int dimension : arr->dimensions) {
        c->size *= dimension;
    }
    return c;
}

Code* translateDec(AST* ast) {
    if (ast->num_children == 3) { // VarDec ASSIGN Exp
        Value* result = lookupVariable(ast->children[0]->children[0]->str);
        Code* c1 = translateExp(ast->children[2], result);
        return c1;
    } else { // handle struct / array dec
        return translateVarDec(ast->children[0]); // NOP
    }
}

Code* translateStmt(AST* stmt, Value* contLabel = nullptr, Value* breakLabel = nullptr) {
    if (stmt->num_children == 1 || stmt->num_children == 2) {
        std::string repr = stmt->to_string();
        if (repr == "Stmt_CONTINUESEMI") {
            return new Code(IR_GOTO, contLabel);
        } else if (repr == "Stmt_BREAKSEMI") {
            return new Code(IR_GOTO, breakLabel);
        } else {
            return translateCode(stmt->children[0], contLabel, breakLabel);
        }
    } else if (stmt->children[0]->op == RETURN_SIGN) {
        Value* t1 = makeTemp();
        Code* c1 = translateExp(stmt->children[1], t1);
        Code* c2 = new Code(IR_RETURN, t1);
        return combineCode(c1, c2);
    } else if (stmt->children[0]->op == IF_SIGN && stmt->num_children == 5) {
        Value *lb1 = makeLabel(), *lb2 = makeLabel();
        Code* c1 = translateCondExp(stmt->children[2], lb1, lb2);
        Code* c2 = new Code(IR_LABEL, lb1);
        Code* c3 = translateStmt(stmt->children[4], contLabel, breakLabel);
        Code* c4 = new Code(IR_LABEL, lb2);
        return combineCode(combineCode(combineCode(c1, c2), c3), c4);
    } else if (stmt->children[0]->op == IF_SIGN && stmt->num_children == 7) {
        Value *lb1 = makeLabel(), *lb2 = makeLabel(), *lb3 = makeLabel();
        Code* c1 = translateCondExp(stmt->children[2], lb1, lb2);
        Code* c2 = new Code(IR_LABEL, lb1);
        Code* c3 = translateStmt(stmt->children[4], contLabel, breakLabel);
        Code* c4 = new Code(IR_GOTO, lb3);
        Code* c5 = new Code(IR_LABEL, lb2);
        Code* c6 = translateStmt(stmt->children[6], contLabel, breakLabel);
        Code* c7 = new Code(IR_LABEL, lb3);
        return combineCode(combineCode(combineCode(combineCode(combineCode(combineCode(c1, c2), c3), c4), c5), c6), c7);
    } else if (stmt->children[0]->op == DO_SIGN) { // DO Stmt WHILE LP Exp RP SEMI
        Value *lb1 = makeLabel(), *lb2 = makeLabel(), *lb3 = makeLabel();
        Code* c1 = new Code(IR_LABEL, lb1);
        Code* c2 = translateStmt(stmt->children[1], lb2, lb3);
        Code* c3 = new Code(IR_LABEL, lb2);
        Code* c4 = translateCondExp(stmt->children[4], lb1, lb3);
        Code* c5 = new Code(IR_GOTO, lb1);
        Code* c6 = new Code(IR_LABEL, lb3);
        return combineCode(combineCode(combineCode(combineCode(combineCode(c1, c2), c3), c4), c5), c6);
    } else if (stmt->children[0]->op == WHILE_SIGN) {
        Value *lb1 = makeLabel(), *lb2 = makeLabel(), *lb3 = makeLabel();
        Code* c1 = new Code(IR_LABEL, lb1);
        Code* c2 = translateCondExp(stmt->children[2], lb2, lb3);
        Code* c3 = new Code(IR_LABEL, lb2);
        Code* c4 = translateStmt(stmt->children[4], lb1, lb3);
        Code* c5 = new Code(IR_GOTO, lb1);
        Code* c6 = new Code(IR_LABEL, lb3);
        return combineCode(combineCode(combineCode(combineCode(combineCode(c1, c2), c3), c4), c5), c6);
    } else if (stmt->children[0]->op == FOR_SIGN) { // FOR LP Exp SEMI Exp SEMI Exp RP Stmt
        Value *lb1 = makeLabel(), *lb2 = makeLabel(), *lb3 = makeLabel();
        Code* c0_0 = translateCode(stmt->children[2], contLabel, breakLabel);
        Code* c0_1 = translateCode(stmt->children[6], contLabel, breakLabel);
        Code* c1 = combineCode(c0_0, new Code(IR_LABEL, lb1));
        Code* c2 = stmt->children[4]->op != NOP_SIGN ? translateCondExp(stmt->children[4], lb2, lb3) : nullptr;
        Code* c3 = new Code(IR_LABEL, lb2);
        Code* c4 = combineCode(translateStmt(stmt->children[8], lb1, lb3), c0_1);
        Code* c5 = new Code(IR_GOTO, lb1);
        Code* c6 = new Code(IR_LABEL, lb3);
        return combineCode(combineCode(combineCode(combineCode(combineCode(c1, c2), c3), c4), c5), c6);
    }
    return nullptr; // unreachable
}

Value* findArrayValue(AST* ast) {
    AST* arr = ast;
    bool isArray = false;
    while (ast->children[0]->op != SYMBOL) {
        ast = ast->children[0];
        isArray = true;
    }
    if (isArray) {
        translateVarDec(arr, true);
    }
    return lookupVariable(ast->children[0]->str);
}

Code* translateVarList(AST* varList, std::vector<Value*>& argList) {
    if (varList->num_children == 1) {
        Value* arg = findArrayValue(varList->children[0]->children[1]);
        argList.push_back(arg);
    } else {
        Value* arg = findArrayValue(varList->children[0]->children[1]);
        argList.push_back(arg);
        translateVarList(varList->children[2], argList);
    }
    return nullptr; // ALWAYS NOP
}

Code* translateFunDec(AST* funDec) {
    if (funDec->num_children == 3) {
        return new Code(IR_FUNDEC, makeSV(funDec->children[0]->str));
    } else {
        Code* c1 = new Code(IR_FUNDEC, makeSV(funDec->children[0]->str));
        std::vector<Value*> argList;
        translateVarList(funDec->children[2], argList);
        Code* c2 = nullptr;
        for (int i = 0; i < argList.size(); i++) {
            c2 = combineCode(c2, new Code(IR_PARAM, argList[i]));
        }
        return combineCode(c1, c2);
    }
}

Code* translateCode(AST* ast, Value* contLabel, Value* breakLabel) {
    if (!ast) return nullptr;
    
    Value* t1;
    
    switch (ast->op) {
        case EXP:
            t1 = makeTemp();
            return translateExp(ast, t1);
        case FUNDEC:
            return translateFunDec(ast);
        case DEC:
            return translateDec(ast);
        case STMT:
            return translateStmt(ast, contLabel, breakLabel);
        default:
            Code* code = nullptr;
            for (int i = 0; i < ast->num_children; i++) {
                code = combineCode(code, translateCode(ast->children[i], contLabel, breakLabel));
            }
            return code;
    }
}

const char* ircode_to_string(IROpCode opcode) {
    switch (opcode) {
        case IR_LT: return "<";
        case IR_LE: return "<=";
        case IR_GT: return ">";
        case IR_GE: return ">=";
        case IR_NE: return "!=";
        case IR_EQ: return "==";
        case IR_ADD: return "+";
        case IR_MINUS: return "-";
        case IR_MUL: return "*";
        case IR_DIV: return "/";
        default: return "?";
    }
}

void irFixPrev(Code* head) {
    Code* prev = nullptr;
    while (head) {
        head->prev = prev;
        prev = head;
        head = head->next;
    }
}

void irPrint(Code* head) {
    while (head) {
        switch (head->opcode) {
            case IR_MOVE:
                printf("%s := %s\n", head->result->to_chararray(), head->arg1->to_chararray());
                break;
            case IR_LOADADDR:
                printf("%s := &%s\n", head->result->to_chararray(), head->arg1->to_chararray());
                break;
            case IR_LOAD:
                printf("%s := *%s\n", head->result->to_chararray(), head->arg1->to_chararray());
                break;
            case IR_STORE:
                printf("*%s := %s\n", head->result->to_chararray(), head->arg1->to_chararray());
                break;
            case IR_ADD:
            case IR_MINUS:
            case IR_MUL:
            case IR_DIV:
                printf("%s := %s %s %s\n", head->result->to_chararray(), head->arg1->to_chararray(),
                                           ircode_to_string(head->opcode), head->arg2->to_chararray());
                break;
            case IR_FUNDEC:
                printf("FUNCTION %s :\n", head->result->to_chararray());
                break;
            case IR_LABEL:
                printf("LABEL %s :\n", head->result->to_chararray());
                break;
            case IR_IFGOTO:
                printf("IF %s %s %s GOTO %s\n", head->arg1->to_chararray(), ircode_to_string(head->relop), 
                                                head->arg2->to_chararray(), head->result->to_chararray());
                break;
            case IR_GOTO:
                printf("GOTO %s\n", head->result->to_chararray());
                break;
            case IR_READ:
                printf("READ %s\n", head->result->to_chararray());
                break;
            case IR_WRITE:
                printf("WRITE %s\n", head->result->to_chararray());
                break;
            case IR_CALL:
                printf("%s := CALL %s\n", head->result->to_chararray(), head->arg1->to_chararray());
                break;
            case IR_RETURN:
                printf("RETURN %s\n", head->result->to_chararray());
                break;
            case IR_ARG:
                printf("ARG %s\n", head->result->to_chararray());
                break;
            case IR_PARAM:
                printf("PARAM %s\n", head->result->to_chararray());
                break;
            case IR_ALLOC:
                printf("DEC %s %d\n", head->result->to_chararray(), head->size);
                break;
            default:
                printf("%s\n", head->to_chararray());
        }
        head = head->next;
    }
}
