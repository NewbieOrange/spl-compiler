#pragma once

#include <functional>
#include <unordered_set>
#include <unordered_map>

#include "ast.h"
#include "ir.hpp"

#define ENABLE_OPT 100

void disableInst(Code* code) {
    if (code->prev) {
        code->prev->next = code->next;
    }
    if (code->next) {
        code->next->prev = code->prev;
    }
}

IROpCode rev_relop(IROpCode opcode) {
    switch (opcode) {
        case IR_LT: return IR_GE;
        case IR_LE: return IR_GT;
        case IR_GT: return IR_LE;
        case IR_GE: return IR_LT;
        case IR_NE: return IR_EQ;
        case IR_EQ: return IR_NE;
        default: return IR_NOP;
    }
}

bool irIsAssign(IROpCode opcode) {
    switch (opcode) {
        case IR_CALL:
        case IR_MOVE:
        case IR_ADD:
        case IR_MINUS:
        case IR_MUL:
        case IR_DIV:
        case IR_ALLOC:
        case IR_LOAD:
        case IR_LOADADDR:
            return true;
        default:
            return false;
    }
}

void irUnusedValueOpt(Code* code) {
    std::unordered_set<Value*> usedValues;
    
    Code* root = code;
    while (code) {
        switch (code->opcode) {
            case IR_CALL:
            case IR_IFGOTO:
            case IR_GOTO:
            case IR_ARG:
            case IR_RETURN:
            case IR_READ:
            case IR_WRITE:
            case IR_STORE:
                usedValues.insert(code->result);
            default:
                usedValues.insert(code->arg1);
                usedValues.insert(code->arg2);
            case IR_LABEL:
            case IR_NOP:
                ;
        }
        code = code->next;
    }
    
    code = root;
    while (code) {
        if ((code->opcode == IR_LABEL || irIsAssign(code->opcode)) && usedValues.find(code->result) == usedValues.end()) {
            disableInst(code);
        }
        code = code->next;
    }
}

void irLabelOpt(Code* code) {
    std::unordered_map<Value*, Value*> remapping;
    
    Code* root = code;
    while (code) {
        Code* next = code->next;
        if (code->opcode == IR_LABEL && next && next->opcode == IR_LABEL) {
            remapping[next->result] = code->result;
            disableInst(next);
        }
        code = next;
    }
    
    code = root;
    while (code) {
        if (code->opcode == IR_IFGOTO || code->opcode == IR_GOTO) {
            Value* label = code->result;
            while (true) {
                auto iter = remapping.find(label);
                if (iter == remapping.end()) {
                    break;
                } else {
                    label = iter->second;
                }
            }
            code->result = label;
        }
        code = code->next;
    }
}

void irConstantPropOpt(Code* code) {
    std::unordered_map<Value*, int> constants;
    std::unordered_map<Value*, int> assignments;
    
    Code* root = code;
    while (code) {
        Value* arg1 = code->arg1;
        Value* arg2 = code->arg2;
        Value* result = code->result;
        switch (code->opcode) {
            case IR_CALL:
            case IR_MOVE: {
                if (isConstant(arg1)) {
                    constants[result] = arg1->val;
                } else {
                    assignments[result]++;
                }
                break;
            }
            case IR_ADD:
            case IR_MINUS:
            case IR_MUL:
            case IR_DIV: {
                if (isConstant(arg1) && isConstant(arg2)) {
                    if (code->opcode == IR_ADD) {
                        code->arg1 = makeCV(arg1->val + arg2->val);
                    } else if (code->opcode == IR_MINUS) {
                        code->arg1 = makeCV(arg1->val - arg2->val);
                    } else if (code->opcode == IR_MUL) {
                        code->arg1 = makeCV(arg1->val * arg2->val);
                    } else if (code->opcode == IR_DIV) {
                        code->arg1 = makeCV(arg1->val / arg2->val);
                    }
                    code->opcode = IR_MOVE;
                    code->arg2 = nullptr;
                } else if (code->opcode == IR_ADD) {
                    if (isConstant(arg1, 0) || isConstant(arg2, 0)) {
                        code->opcode = IR_MOVE;
                        code->arg1 = isConstant(code->arg1) ? code->arg2 : code->arg1;
                        code->arg2 = nullptr;
                    }
                } else if (code->opcode == IR_MINUS) {
                    if (isConstant(arg2, 0)) {
                        code->opcode = IR_MOVE;
                        code->arg2 = nullptr;
                    } else if (arg1 == arg2) {
                        code->opcode = IR_MOVE;
                        code->arg1 = makeCV(0);
                        code->arg2 = nullptr;
                    }
                } else if (code->opcode == IR_MUL) {
                    if (isConstant(arg1, 1)) {
                        code->opcode = IR_MOVE;
                        code->arg1 = arg2;
                        code->arg2 = nullptr;
                    } else if (isConstant(arg2, 1)) {
                        code->opcode = IR_MOVE;
                        code->arg2 = nullptr;
                    } else if (isConstant(arg1, 0) || isConstant(arg2, 0)) {
                        code->opcode = IR_MOVE;
                        code->arg1 = makeCV(0);
                        code->arg2 = nullptr;
                    }
                } else if (code->opcode == IR_DIV) {
                    if (isConstant(arg2, 1)) {
                        code->opcode = IR_MOVE;
                        code->arg2 = nullptr;
                    } else if (arg1 == arg2) {
                        code->opcode = IR_MOVE;
                        code->arg1 = makeCV(1);
                        code->arg2 = nullptr;
                    }
                }
                assignments[result]++;
                break;
            }
            default:
                ;
        }
        code = code->next;
    }
    
    code = root;
    while (code) {
        std::vector<Value**> vec { &code->arg1, &code->arg2 };
        switch (code->opcode) {
            case IR_ARG:
            case IR_RETURN:
            case IR_WRITE:
                vec.push_back(&code->result);
        }
        
        for (Value** val : vec) {
            if (*val) {
                auto iter = constants.find(*val);
                auto iter3 = assignments.find(*val);
                if (iter != constants.end() && iter3 == assignments.end()) {
                    *val = makeCV(iter->second);
                }
            }
        }
        
        code = code->next;
    }
}

void irPeepholeOpt(Code* code) {
    while (code) {
        IROpCode opcode = code->opcode;
        Value* arg1 = code->arg1;
        Value* arg2 = code->arg2;
        Value* result = code->result;
        Code* next = code->next;
        Code* next2 = next ? next->next : nullptr;
        switch (opcode) {
            case IR_MOVE: {
                if (arg1 == result) {
                    disableInst(code);
                }
                break;
            }
            case IR_IFGOTO:
            case IR_GOTO: {
                if (opcode == IR_IFGOTO && next && next2) {
                    if (next->opcode == IR_GOTO && next2->opcode == IR_LABEL && result == next2->result) {
                        code->relop = rev_relop(code->relop);
                        code->result = next->result;
                        disableInst(next);
                    }
                }
                if (next && next->opcode == IR_LABEL && result == next->result) {
                    disableInst(code);
                }
                break;
            }
            case IR_ADD:
                if (isConstant(code->arg1) && !isConstant(code->arg2)) {
                    std::swap(code->arg1, code->arg2);
                } // fall through
            case IR_MINUS: {
                Code* code2 = code->prev;
                if (code2 && code->arg1 == code2->result && (code2->opcode == IR_ADD || code2->opcode == IR_MINUS)) {
                    if (opcode == IR_MINUS && code->arg2 == code2->arg1 && isConstant(code2->arg2)) {
                        code->opcode = IR_MOVE;
                        code->arg1 = makeCV((code2->opcode == IR_ADD ? 1 : -1) * code2->arg2->val);
                        code->arg2 = nullptr;
                    }
                }
                if (code2 && (code2->opcode == IR_ADD || code2->opcode == IR_MINUS)) {
                    Value* baseVar;
                    int baseline = 0;
                    if (code2->opcode != IR_MOVE && isConstant(code2->arg2)) {
                        baseline = code2->arg2->val;
                        baseVar = code2->arg1;
                    } else {
                        break;
                    }
                    if (code2->opcode == IR_MINUS) {
                        baseline = -baseline;
                    }
                    int offset = 0;
                    if (isConstant(code->arg2)) {
                        offset = code->arg2->val;
                    } else {
                        break;
                    }
                    if (code->opcode == IR_MINUS) {
                        offset = -offset;
                    }
                    
                    if (code2->result == code->result) {
                        disableInst(code2);
                    }
                    int result = baseline + offset;
                    if (result == 0) {
                        code->opcode = IR_MOVE;
                        code->arg1 = baseVar ? baseVar : makeCV(result);
                        code->arg2 = nullptr;
                    } else if (result > 0) {
                        code->opcode = IR_ADD;
                        code->arg1 = baseVar ? baseVar : makeCV(result);
                        code->arg2 = baseVar ? makeCV(result) : nullptr;
                    } else if (result < 0) {
                        code->opcode = IR_MINUS;
                        code->arg1 = baseVar ? baseVar : makeCV(result);
                        code->arg2 = baseVar ? makeCV(-result) : nullptr;
                    }
                }
                break;
            }
        }
        if (code->prev && code->prev->opcode == IR_MOVE) {
            std::vector<Value**> vec { &code->arg1, &code->arg2 };
            switch (code->opcode) {
                case IR_ARG:
                case IR_RETURN:
                case IR_WRITE:
                    vec.push_back(&code->result);
            }
            for (Value** var : vec) {
                if (*var && *var == code->prev->result) {
                    *var = code->prev->arg1;
                }
            }
        }
        code = next;
    }
}

void irOptimize(Code* code) {
    for (int i = 0; i < ENABLE_OPT; i++) {
        irFixPrev(code);
        irPeepholeOpt(code);
        irUnusedValueOpt(code);
        irLabelOpt(code);
        irConstantPropOpt(code);
    }
}
