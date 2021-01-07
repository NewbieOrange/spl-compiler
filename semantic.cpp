#include "semantic.hpp"
#include <algorithm>
#include <unordered_map>
#include <functional>
#include <string>
#include <vector>


bool Symbol::structual_equivalent(const Symbol& other) {
        std::vector<ExprType> this_members, other_members;
        for (const auto& member : members) {
            this_members.push_back(ExprType(false, member.type));
        }
        for (const auto& member : other.members) {
            other_members.push_back(ExprType(false, member.type));
        }
        return this_members == other_members;
};

std::unordered_map<int, std::function<void(AST*)>> handler_map;

std::vector<std::unordered_map<std::string, Symbol>> symbol_table { { } };

std::string current_scope;

void symbol_push_stack() {
    symbol_table.emplace_back();
    stack_depth++;
}

void symbol_pop_stack() {
    symbol_table.pop_back();
    stack_depth--;
}

std::string pop_array_bracket(std::string specifier) {
    int begin = specifier.find('['), end = specifier.find(']', begin);
    return begin == std::string::npos ? specifier : specifier.erase(begin, end - begin + 1);
}

void semantic_error(int err_type, int lineno, const char* msg) {
    printf("Error type %d at Line %d: %s\n", err_type, lineno, msg);
};

Symbol* check_symbol(const std::string& name, int depth) {
    const auto& iter = symbol_table[depth].find(name);
    if (iter != symbol_table[depth].end()) {
        return &iter->second;
    } else if (depth > 0) {
        return check_symbol(name, depth - 1);
    } else {
        return nullptr;
    }
}

Symbol* check_variable(const std::string& name, int lineno) {
    Symbol* ptr = check_symbol(name);
    if (!ptr) {
        semantic_error(1, lineno, "variable is used without definition");
    }
    return ptr;
}

Symbol* check_struct(const std::string& name, int lineno) {
    Symbol* ptr = check_symbol(name);
    if (!ptr) {
        semantic_error(16, lineno, "struct is used without definition");
    }
    return ptr;
}

Symbol* check_function(const std::string& name, int lineno) {
    Symbol* ptr = check_symbol(name);
    if (!ptr) {
        semantic_error(2, lineno, "function is invoked without definition");
    }
    return ptr;
}

bool insert_symbol(Symbol& symbol, int lineno, int depth = stack_depth) {
    bool isVariable = symbol.symbol_type == VARIABLE;
    bool isFunction = symbol.symbol_type == FUNCTION;
    auto& table = symbol_table[isVariable ? depth : 0];
    if (table.find(symbol.name) != table.end()) {
        semantic_error(isVariable ? 3 : (isFunction ? 4 : 15), lineno, isVariable ? "variable is redefined in the same scope" : (isFunction ? "function is redefined in the global scope" : "struct is redefined in the global scope"));
        return false;
    } else {
        table[symbol.name] = symbol;
        return true;
    }
}

void visitNode(AST* ast);

void visitChildren(AST* ast) {
    for (int i = 0; i < ast->num_children; i++) {
        visitNode(ast->children[i]);
    }
}

void visitNode(AST* ast) {
    if (ast) {
        auto handler = handler_map.find(ast->op);
        if (handler != handler_map.end()) {
            handler->second(ast);
        } else {
            visitChildren(ast);
        }
    }
}

std::vector<Symbol> visitDecList(std::string specifier, AST* ast);

std::string symbol_from_specifier(AST* ast);

std::vector<Symbol> visitStructSpecifierDef(AST* ast) {
    std::string specifier = symbol_from_specifier(ast->children[0]);
    return visitDecList(specifier, ast->children[1]);
}

std::vector<Symbol> visitStructSpecifierDefList(AST* ast) {
    if (!ast) {
        return {};
    }
    auto vec = visitStructSpecifierDef(ast->children[0]);
    auto vec2 = visitStructSpecifierDefList(ast->children[1]);
    vec.insert(vec.end(), vec2.begin(), vec2.end());
    return vec;
}

void visitStructSpecifier(AST* ast) { // dirty hacks :(
    if (ast->num_children >= 4) { // STRUCT ID LC (DefList) RC
        Symbol symbol(STRUCTDEF, ast->children[1]->str);
        if (insert_symbol(symbol, ast->lineno) && ast->num_children == 5) { // STRUCT ID LC DefList RC
            symbol_table[0][symbol.name].members = visitStructSpecifierDefList(ast->children[3]);
        }
    } else { // STRUCT ID
        check_struct(ast->children[1]->str, ast->lineno);
    }
}

std::string symbol_from_specifier(AST* ast) {
    if (ast->num_children == 0) { // ID
        return typeName(ast->val);
    } else { // StructSpecifier
        visitStructSpecifier(ast->children[0]);
        return ast->children[0]->children[1]->str;
    }
}

Symbol visitVarDec(std::string specifier, AST* ast) {
    if (ast->num_children == 1) { // ID
        Symbol symbol(VARIABLE, ast->children[0]->str);
        symbol.type = specifier;
        return symbol;
    } else { // VarDec LB INT RB
        auto symbol = visitVarDec(specifier, ast->children[0]);
        symbol.type += "[" + std::to_string(ast->children[2]->val) + "]";
        return symbol;
    }
}

ExprType visitExp(AST* ast);

Symbol visitDec(std::string specifier, AST* ast) {
    // VarDec or VarDec ASSIGN Exp, don't care, check assign in visitExp
    return visitVarDec(specifier, ast->children[0]);
}

std::vector<Symbol> visitDecList(std::string specifier, AST* ast) {
    if (ast->num_children == 1) {
        return { visitDec(specifier, ast->children[0]) };
    } else {
        auto vec = visitDecList(specifier, ast->children[2]);
        vec.push_back(visitDec(specifier, ast->children[0]));
        return vec;
    }
}

void visitDef(AST* ast) {
    std::string specifier = symbol_from_specifier(ast->children[0]);
    AST* decList = ast->children[1];
    auto symbols = visitDecList(specifier, decList);
    for (auto& symbol : symbols) {
        insert_symbol(symbol, ast->lineno);
    }
}

void visitDefList(AST* ast) {
    if (ast && ast->num_children > 0) {
        visitDef(ast->children[0]);
        visitDefList(ast->children[1]);
    }
}

std::vector<ExprType> visitArgs(AST* ast) {
    if (ast->num_children == 1) { // Exp
        return { visitExp(ast->children[0]) };
    } else { // Exp COMMA Args
        auto vec = visitArgs(ast->children[2]);
        vec.push_back(visitExp(ast->children[0]));
        return vec;
    }
}

ExprType visitExp(AST* ast) {
    // TODO: Add other exp
    if (ast->num_children == 4) {
        if (ast->children[0]->op == SYMBOL) { // ID LP Args RP
            Symbol* ptr = check_function(ast->children[0]->str, ast->lineno);
            if (ptr) {
                if (ptr->symbol_type != FUNCTION) {
                    semantic_error(11, ast->lineno, "invoke function operator on non-function names");
                } else {
                    auto args = visitArgs(ast->children[2]);
                    bool argsAllMatch = args.size() == ptr->params.size();
                    if (argsAllMatch) {
                        for (int i = 0; i < args.size(); i++) {
                            if (args[i] != ExprType(false, ptr->params[i])) {
                                argsAllMatch = false;
                                break;
                            }
                        }
                    }
                    if (!argsAllMatch) {
                        semantic_error(9, ast->lineno, "function's arguments mismatch the declared arguments");
                    } else {
                        return ExprType(false, ptr->type);
                    }
                }
            }
        } else { // Exp LB Exp RB
            ExprType array_type = visitExp(ast->children[0]);
            ExprType index_type = visitExp(ast->children[2]);
            bool success = true;
            if (array_type.type.find('[') == std::string::npos) {
                semantic_error(10, ast->lineno, "indexing on non-array");
                success = false;
            }
            if (index_type.type != "int") {
                semantic_error(12, ast->lineno, "indexing by non-integer");
                success = false;
            }
            if (success) {
                return ExprType(true, pop_array_bracket(array_type.type));
            }
        }
    } else if (ast->num_children == 3) {
        if (ast->to_string() == "Exp_ExpASSIGNExp") {
            ExprType ltype = visitExp(ast->children[0]);
            ExprType rtype = visitExp(ast->children[2]);
            if (ltype != rtype) {
                semantic_error(5, ast->lineno, "unmatching types on both sides of assignment");
            } else if (ltype.valid && rtype.valid && !ltype.l_value) {
                semantic_error(6, ast->lineno, "rvalue on the left side of assignment");
            } else {
                return ExprType(false, ltype.type);
            }
        } else if (ast->children[0]->op == EXP && ast->children[2]->op == EXP) { // Exp(OPR)Exp
            ExprType ltype = visitExp(ast->children[0]);
            ExprType rtype = visitExp(ast->children[2]);
            if (ltype != rtype) {
                semantic_error(7, ast->lineno, "unmatching operands on both sides of operator");
            } else {
                if (ltype.valid && rtype.valid) {
                    switch (ast->children[1]->op) {
                        case AND_OP:
                        case OR_OP: // only int can do bool operation
                            if (!ltype.isInt() || !rtype.isInt()) {
                                semantic_error(17, ast->lineno, "non-integral boolean operation");
                            }
                            break;
                        case PLUS_OP:
                        case MINUS_OP:
                        case MUL_OP:
                        case DIV_OP:
                            if (!ltype.isIntOrFloat() || !rtype.isIntOrFloat()) {
                                semantic_error(18, ast->lineno, "non-numeral arithmetic operation");
                            }
                            break;
                        default:
                            if (ltype.isChar() || rtype.isChar()) {
                                semantic_error(19, ast->lineno, "char in binary operation");
                            }
                    }
                }
                return ExprType(false, ltype.type);
            }
        } else if (ast->to_string() == "Exp_ExpDOTID") {
            ExprType ltype = visitExp(ast->children[0]);
            std::string field = ast->children[2]->str;
            Symbol* ptr = check_variable(ltype.type, ast->lineno);
            if (ptr) {
                if (ptr->symbol_type != STRUCTDEF) {
                    semantic_error(13, ast->lineno, "accessing member of non-struct variables");
                } else {
                    auto members = ptr->members;
                    std::string found_type;
                    for (auto& member : members) {
                        if (member.name == field) {
                            found_type = member.type;
                            break;
                        }
                    }
                    if (!found_type.empty()) {
                        return ExprType(true, found_type);
                    } else {
                        semantic_error(14, ast->lineno, "accessing an undefined struct member");
                    }
                }
            } else if (ltype.valid) {
                semantic_error(13, ast->lineno, "accessing member of non-struct variables");
            }
        } else if (ast->to_string() == "Exp_IDLPRP") {
            Symbol* ptr = check_function(ast->children[0]->str, ast->lineno);
            if (ptr) {
                if (ptr->symbol_type != FUNCTION) {
                    semantic_error(11, ast->lineno, "invoking function operator on non-function names");
                } else if (!ptr->params.empty()) {
                    semantic_error(9, ast->lineno, "function's arguments mismatch the declared arguments");
                } else {
                    return ExprType(false, ptr->type);
                }
            }
        } else if (ast->to_string() == "Exp_LPExpRP") {
            return visitExp(ast->children[1]);
        }
    } else if (ast->num_children == 2) { // MINUS Exp || NOT Exp
        ExprType type = visitExp(ast->children[1]);
        if (type.valid) {
            switch (ast->children[0]->op) {
                case NOT_OP: // only int can do bool operation
                    if (!type.isInt()) {
                        semantic_error(17, ast->lineno, "non-integral boolean operation");
                    }
                    break;
                case MINUS_OP:
                    if (!type.isIntOrFloat()) {
                        semantic_error(18, ast->lineno, "non-numeral arithmetic operation");
                    }
                    break;
            }
        }
        type.l_value = false;
        return type;
    } else if (ast->num_children == 1) {
        if (ast->children[0]->op == SYMBOL) { // ID
            Symbol* ptr = check_variable(ast->children[0]->str, ast->lineno);
            if (ptr) {
                return ExprType(true, ptr->type);
            }
        } else if (ast->children[0]->op == INT_CONST) { // INT
            return ExprType(false, "int");
        } else if (ast->children[0]->op == FLOAT_CONST) { // FLOAT
            return ExprType(false, "float");
        } else if (ast->children[0]->op == CHAR_CONST) { // CHAR
            return ExprType(false, "char");
        }
    } else {
        visitChildren(ast);
    }
    return ExprType(false, "");
}

Symbol visitParamDec(AST* ast) {
    std::string specifier = symbol_from_specifier(ast->children[0]);
    Symbol symbol = visitVarDec(specifier, ast->children[1]);
    symbol.type = specifier;
    return symbol;
}

std::vector<Symbol> visitVarList(AST* ast) {
    if (ast->num_children == 1) { // ParamDec
        return { visitParamDec(ast->children[0]) };
    } else { // ParamDec COMMA VarList
        auto vec = visitVarList(ast->children[2]);
        vec.push_back(visitParamDec(ast->children[0]));
        return vec;
    }
}

Symbol visitFunDec(AST* ast) {
    Symbol symbol(FUNCTION, ast->children[0]->str);
    if (ast->num_children == 4) {
        auto varList = visitVarList(ast->children[2]);
        for (auto& var : varList) {
            insert_symbol(var, ast->lineno);
            symbol.params.push_back(var.type);
        }
    }
    return symbol;
}

std::vector<Symbol> visitExtDecList(std::string specifier, AST* ast) {
    if (ast->num_children == 1) { // VarDec
        return { visitVarDec(specifier, ast->children[0]) };
    } else { // VarDec COMMA ExtDecList
        auto vec = visitExtDecList(specifier, ast->children[2]);
        vec.push_back(visitVarDec(specifier, ast->children[0]));
        return vec;
    }
}

void visitExtDef(AST* ast) {
    std::string specifier = symbol_from_specifier(ast->children[0]);
    if (ast->num_children == 3 && ast->to_string() == "ExtDef_SpecifierExtDecListSEMI") {
        auto symbols = visitExtDecList(specifier, ast->children[1]);
        for (auto& symbol : symbols) {
            insert_symbol(symbol, ast->lineno);
        }
    } else if (ast->num_children == 3 && ast->to_string() == "ExtDef_SpecifierFunDecCompSt") {
        symbol_push_stack();
        auto symbol = visitFunDec(ast->children[1]);
        symbol.type = specifier;
        insert_symbol(symbol, ast->lineno);
        std::string alt_name = symbol.to_string();
        current_scope = alt_name;
        symbol_table[0][alt_name] = symbol;
        visitNode(ast->children[2]);
        symbol_pop_stack();
    } else {
        visitChildren(ast);
    }
}

void visitCompSt(AST* ast) {
    symbol_push_stack();
    if (ast->children[1] && ast->children[1]->op == DEFLIST) {
        visitDefList(ast->children[1]);
    }
    visitChildren(ast);
    symbol_pop_stack();
}

void visitStmt(AST* ast) {
    if (ast->num_children == 3 && ast->children[0]->op == RETURN_SIGN) { // RETURN Exp SEMI
        Symbol current_fun = symbol_table[0][current_scope]; // Functions are always at global scope
        ExprType rtype = visitExp(ast->children[1]);
        if (ExprType(true, current_fun.type) != rtype) {
            semantic_error(8, ast->lineno, "function return type mismatch the declared type");
        }
    } else {
        visitChildren(ast);
    }
}

void initHandlers() {
    Symbol symbol_int(VARIABLE, "int"), symbol_float(VARIABLE, "float"), symbol_char(VARIABLE, "char");
    symbol_int.type = "int", symbol_float.type = "float", symbol_char.type = "char";
    insert_symbol(symbol_int, -1);
    insert_symbol(symbol_float, -1);
    insert_symbol(symbol_char, -1);
    
    handler_map[EXTDEF] = visitExtDef;
    handler_map[COMPST] = visitCompSt;
    handler_map[STMT] = visitStmt;
    handler_map[EXP] = visitExp;
}
