#pragma once
#include <vector>
#include <unordered_map>

enum IROpCode {
    IR_NOP,
    IR_SYMBOL,
    IR_MOVE,
    IR_ADD,
    IR_MINUS,
    IR_MUL,
    IR_DIV,
    IR_GT,
    IR_LT,
    IR_GE,
    IR_LE,
    IR_EQ,
    IR_NE,
    IR_AND,
    IR_OR,
    IR_NOT,
    IR_FUNDEC,
    IR_LABEL,
    IR_IFGOTO,
    IR_GOTO,
    IR_READ,
    IR_WRITE,
    IR_CALL,
    IR_RETURN,
    IR_ARG,
    IR_PARAM,
    IR_LOADADDR,
    IR_LOAD,
    IR_STORE,
    IR_ALLOC,
    
};

enum ValueType {
    VT_SYMBOL,
    VT_LABEL,
    VT_CONST,
    VT_VAR,
    VT_TEMP,
    VT_POINTER,
    VT_COMPLEX
};

struct Value {
    ValueType type;
    union {
        int val;
        char* name;
        AST* ast;
    };
    
    Value(ValueType type, int val) : type(type), val(val) {};
    Value(ValueType type, char* name) : type(type), name(name) {};
    Value(ValueType type, AST* ast) : type(type), ast(ast) {};
    
    std::string to_string() const {
        switch (type) {
            case VT_SYMBOL:
                return name;
            case VT_LABEL:
                return "label" + std::to_string(val);
            case VT_CONST:
                return "#" + std::to_string(val);
            case VT_VAR:
                return "v" + std::to_string(val);
            case VT_TEMP:
                return "t" + std::to_string(val);
            case VT_POINTER:
                return "a" + std::to_string(val);
            case VT_COMPLEX:
                return "complex";
            default:
                return "?";
        }
    };
    
    char* to_chararray() {
        return strdup(to_string().c_str());
    }
};

struct Array {
    char* name;
    
    std::vector<int> dimensions;
    
    std::vector<int> sizes;
    
    bool param = false;
    
    Array(char* name) : name(name) {};
};

bool isConstant(const Value* v) {
    return v && v->type == VT_CONST;
}

bool isConstant(const Value* v, const int val) {
    return isConstant(v) && v->val == val;
}

Value* makeSV(char* name) {
    return new Value(VT_SYMBOL, name);
}

Value* makeLV(int val) {
    return new Value(VT_LABEL, val);
}

Value* makeCV(int val) {
    return new Value(VT_CONST, val);
}

Value* makeVV(int val) {
    return new Value(VT_VAR, val);
}

Value* makeTV(int val) {
    return new Value(VT_TEMP, val);
}

Value* makePV(int val) {
    return new Value(VT_POINTER, val);
}

Array* makeArray(AST* ast) {
    if (ast->num_children == 1) { // ID
        return new Array(strdup(ast->children[0]->str));
    } else { // VarDec LB INT RB
        Array* arr = makeArray(ast->children[0]);
        arr->dimensions.push_back(ast->children[2]->val);
        return arr;
    }
}

std::unordered_map<std::string, Value*> symbol_table;

std::unordered_map<std::string, Array*> array_table;

std::unordered_map<Value*, Array*> symbol_array_table;

template<typename T>
std::string safe_to_string(T* ptr) {
    if (ptr) return ptr->to_string();
    else return "null";
}

struct Code {
    IROpCode opcode = IR_NOP;
    Value* arg1 = nullptr;
    Value* arg2 = nullptr;
    Value* result = nullptr;
    IROpCode relop = IR_NOP;
    Code* prev = nullptr;
    Code* next = nullptr;
    int size = 0;
    
    Code(IROpCode opcode) : opcode(opcode) {};
    Code(IROpCode opcode, Value* result) : opcode(opcode), result(result) {};
    Code(IROpCode opcode, Value* arg1, Value* result) : opcode(opcode), arg1(arg1), result(result) {};
    Code(IROpCode opcode, Value* arg1, Value* arg2, Value* result) : opcode(opcode), arg1(arg1), arg2(arg2), result(result) {};
    Code(IROpCode opcode, Value* arg1, Value* arg2, Value* result, IROpCode relop) : opcode(opcode), arg1(arg1), arg2(arg2), result(result), relop(relop) {};
    
    std::string to_string() {
        return std::to_string(opcode) + " " + safe_to_string(arg1) + ", " + safe_to_string(arg2) + ", " + safe_to_string(result)
                    + (relop == IR_NOP ? "" : (" (" +std::to_string(relop) + ")"));
    }
    
    char* to_chararray() {
        return strdup(to_string().c_str());
    }
};

Code* combineCode(Code* c1, Code* c2) {
    if (!c1) return c2;
    else if (!c2) return c1;
    
    Code* c = c1;
    while (c->next) {
        c = c->next;
    }
    c->next = c2;
    c2->prev = c1;
    
    return c1;
}

struct IRBlock {
    Value* label;
    Code* code;
    
    std::vector<IRBlock*> incoming;
    int postorderIndex;
    
    IRBlock* outgoingDirect;
    IRBlock* outgoingCond;
};

struct IRMethod {
    char* name;
    std::vector<Value*> args, locals;
    std::vector<IRBlock*> blocks, blocksPre, blocksPost, blocksRPost;
};



