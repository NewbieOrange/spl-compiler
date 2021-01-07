#pragma once
#include "ast.h"
#include <unordered_map>
#include <functional>
#include <string>
#include <vector>

enum SymbolType { VARIABLE, STRUCTDEF, FUNCTION };

int stack_depth = 0;

struct Symbol {
    SymbolType symbol_type;
    std::string type;
    std::string name;
    
    std::vector<std::string> params;
    std::vector<Symbol> members;
    
    Symbol() = default;
    
    Symbol(SymbolType symbol_type, std::string name) : symbol_type(symbol_type), name(name) {}
    
    std::string to_string() {
        std::string str = type + " " + name;
        if (symbol_type == FUNCTION) {
            str += "(";
            for (const std::string& param : params) {
                str += param + ",";
            }
            str.pop_back();
            str += ")";
        }
        return str;
    };
    
    bool structual_equivalent(const Symbol& other);
};

Symbol* check_symbol(const std::string& name, int depth = stack_depth);

struct ExprType {
    bool l_value;
    std::string type;
    bool valid;
    
    ExprType(bool l_value, std::string type) : l_value(l_value), type(type), valid(type != "") {};
    
    bool isInt() const {
        return type == "int";
    }
    
    bool isIntOrFloat() const {
        return type == "int" || type == "float";
    }
    
    bool isChar() const {
        return type == "char";
    }
    
    bool operator==(const ExprType& other) const {
        if (!valid || !other.valid) {
            return true;
        } else if (type == other.type) {
            return true;
        } else {
            Symbol* this_symbol = check_symbol(type);
            Symbol* other_symbol = check_symbol(other.type);
            if (this_symbol && this_symbol->symbol_type == STRUCTDEF && other_symbol && other_symbol->symbol_type == STRUCTDEF) {
                return this_symbol->structual_equivalent(*other_symbol);
            }
        }
        return false;
    }
    
    bool operator!=(const ExprType& other) const {
        return !(*this == other);
    }
    
    std::string to_string() const {
        if (valid) {
            return type + "(" + (l_value ? "L" : "R") + ")";
        } else {
            return "invalid";
        }
    }
};

void visitNode(AST*);

void initHandlers();
