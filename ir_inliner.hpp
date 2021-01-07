#pragma once

#include <algorithm>
#include <unordered_map>
#include <string>
#include <vector>

#include "ast.h"
#include "ir.hpp"
#include "ir_optimizer.hpp"

#define ENABLE_INLINE 100

struct IRFunction {
    Code* entry;
    
    std::vector<Value*> params;
};

std::unordered_map<std::string, IRFunction*> functions;

std::vector<Value*> irFindParams(Code* code) {
    std::vector<Value*> params;
    while (code && code->opcode == IR_PARAM) {
        params.push_back(code->result);
        code = code->next;
    }
    return params;
}

void irFindAllFunctions(Code* code) {
    while (code) {
        if (code->opcode == IR_FUNDEC) {
            IRFunction* function = new IRFunction();
            Code* entry = code->next;
            while (entry && entry->opcode == IR_PARAM) {
                entry = entry->next;
            }
            function->entry = entry;
            function->params = irFindParams(code->next);
            functions[code->result->to_string()] = function;
        }
        code = code->next;
    }
}

bool irCanInline(IRFunction* function) {
    Code* code = function->entry;
    while (code && code->opcode != IR_FUNDEC) {
        if (code->opcode == IR_CALL || code->opcode == IR_LOAD || std::find(function->params.begin(), function->params.end(), code->result) != function->params.end()) {
            return false;
        }
        code = code->next;
    }
    return true;
}

Code* irCopyCode(Code* code, std::unordered_map<Value*, Value*>& args, Value* ret) {
    Code* nCode = new Code(code->opcode, code->arg1, code->arg2, code->result, code->relop);
    nCode->size = code->size;
    
    std::vector<Value**> vec { &nCode->arg1, &nCode->arg2, &nCode->result };
    for (Value** val : vec) {
        if (*val) {
            auto iter = args.find(*val);
            if (iter != args.end()) {
                *val = iter->second;
            }
        }
    }
    
    if (nCode->opcode == IR_RETURN) {
        nCode->opcode = IR_MOVE;
        nCode->arg1 = code->result;
        nCode->result = ret;
    }
    
    return nCode;
}

void irInsertFunction(Code* code, std::unordered_map<Value*, Value*>& args, Value* ret, IRFunction* function) {
    Code* prev = code;
    Code* insert = function->entry;
    
    while (insert && insert->opcode != IR_FUNDEC) {
        if (insert->opcode == IR_LABEL || insert->opcode == IR_IFGOTO || insert->opcode == IR_GOTO) {
            Value* oldLabel = insert->result;
            if (args.find(oldLabel) == args.end()) {
                args[oldLabel] = makeLabel();
            }
        }
        
        Code* copy = irCopyCode(insert, args, ret);
        Code* prevNext = prev->next;
        prev->next = copy;
        copy->next = prevNext;
        prev = copy;
        
        insert = insert->next;
    }
}

void irInlineFunction(IRFunction* function) {
    std::vector<Code*> args;
    Code* code = function->entry;
    while (code && code->opcode != IR_FUNDEC) {
        if (code->opcode == IR_ARG) {
            args.push_back(code);
        } else if (code->opcode == IR_CALL) {
            IRFunction* callee = functions[code->arg1->to_string()];
            if (callee != function && irCanInline(callee)) {
                std::unordered_map<Value*, Value*> remapping;
                for (int i = 0; i < args.size(); i++) {
                    remapping[callee->params[i]] = args[args.size() - i - 1]->result;
                }
                irInsertFunction(code, remapping, code->result, callee);
                for (int i = 0; i < args.size(); i++) {
                    disableInst(args[i]);
                }
                disableInst(code);
            }
            args.clear();
        }
        code = code->next;
    }
}

void irInline(Code* code) {
    irFindAllFunctions(code);
    for (int i = 0; i < ENABLE_INLINE; i++) {
        for (auto& iter : functions) {
            irFixPrev(code);
            irInlineFunction(iter.second);
        }
    }
}
