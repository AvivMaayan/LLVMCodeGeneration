#include "source.hpp"
#include "hw3_output.hpp"
#include "symbol_table_intf.h"

extern int yylineno;
extern SymbolTable symbolTable;

Exp::Exp(const string type, const string value)
    : Node(type), value(value)
{
    if (type == "byte" && stoi(value) > this->MAX_BYTE)
    {
        output::errorByteTooLarge(yylineno, value);
        exit(1);
    }

    this->value = value;
}

Exp::Exp(const RawNumber *num, const string type)
    : Node(type)
{
    assert(type == "byte" || type == "int");

    if (type == "byte" && stoi(num->value) > this->MAX_BYTE)
    {
        output::errorByteTooLarge(yylineno, num->value);
        exit(1);
    }

    this->value = stoi(num->value);
}

Exp::Exp(const Exp *bool_exp)
    : Node(bool_exp->type)
{
    // assert(bool_exp->type == "bool");

    // Actual value is not needed....
    // this->value = (bool_exp.value == "true") ? "false" : "true";
}

Exp::Exp(const Exp *left_exp, const BinOp *op, const Exp *right_exp)
{
    if (!isNumericExp(left_exp) || !isNumericExp(right_exp))
    {
        output::errorMismatch(yylineno);
        exit(1);
    }

    this->type = (left_exp->type == "int" || right_exp->type == "int") ? "int" : "byte";
    // Actual value is not needed....
}

Exp::Exp(const Exp *left_exp, const BoolOp *op, const Exp *right_exp)
    : Node("bool")
{
    if (!isBooleanExp(left_exp) || !isBooleanExp(right_exp))
    {
        output::errorMismatch(yylineno);
        exit(1);
    }

    // Actual value is not needed....
}

Exp::Exp(const Exp *left_exp, const RelOp *op, const Exp *right_exp)
    : Node("bool")
{
    if (!isNumericExp(left_exp) || !isNumericExp(right_exp))
    {
        output::errorMismatch(yylineno);
        exit(1);
    }

    // Actual value is not needed....
}

Exp::Exp(const Type *new_type, const Exp *exp)
{
    // Check if type conversion is valid
    if (!isNumericExp(exp) || !isNumericType(new_type))
    {
        output::errorMismatch(yylineno);
        exit(1);
    }

    if (new_type->type == "byte" && stoi(exp->value) > this->MAX_BYTE)
    {
        output::errorByteTooLarge(yylineno, exp->value);
        exit(1);
    }

    this->type = new_type->type;
    this->value = exp->value;
}

Exp::Exp(const Id *id)
{
    if (!symbolTable.isSymbolExist(id->name))
    {
        output::errorUndef(yylineno, id->name);
        exit(1);
    }

    this->type = symbolTable.getSymbolType(id->name);
    /* We can't get the symbol's real value,
       so we'll use "0" which is a legal BYTE value */
    this->value = "0";
}

Exp::Exp(const Call *call) : Node(call->return_type) {}

ExpList::ExpList(const Exp *expression)
{
    this->exp_list.push_back(expression->type);
}

ExpList::ExpList(const Exp *additional_exp, const ExpList *current_list)
    : exp_list(current_list->exp_list)
{
    this->exp_list.insert(this->exp_list.begin(), additional_exp->type);
}

Call::Call(const string name, ExpList *exp_list)
{
    if (!symbolTable.isFuncSymbolNameExist(name))
    {
        output::errorUndefFunc(yylineno, name);
        exit(1);
    }

    if (exp_list == nullptr)
    {
        exp_list = new ExpList();
    }

    vector<string> ret_types = symbolTable.getLegalCallReturnTypes(name, exp_list->getTypesVector());

    if (ret_types.empty())
    {
        output::errorPrototypeMismatch(yylineno, name);
        exit(1);
    }

    if (ret_types.size() > 1)
    {
        output::errorAmbiguousCall(yylineno, name);
        exit(1);
    }

    this->name = name;
    this->exp_list = *exp_list;
    this->return_type = ret_types[0];
}

vector<string> ExpList::getTypesVector() const
{
    // vector<string> args;

    // for (auto &exp : this->exp_list)
    // {
    //     args.push_back(exp.type);
    // }

    // return args;
    return this->exp_list;
}

FormalDecl::FormalDecl(const Type *type, const Id *id)
    : Node(type->type)
{
    this->type = type->type;
    this->name = id->name;
}

FormalList::FormalList(const FormalDecl *formal_decl)
{
    this->formal_list.push_back(*formal_decl);
}

FormalList::FormalList(const FormalDecl *formal_decl, const FormalList *current_list)
    : formal_list(current_list->formal_list)
{
    this->formal_list.insert(this->formal_list.begin(), *formal_decl);
}

vector<string> FormalList::getTypesVector() const
{
    vector<string> arg_types;

    for (auto &formal : this->formal_list)
    {
        arg_types.push_back(formal.type);
    }

    return arg_types;
}

vector<string> FormalList::getNamesVector() const
{
    vector<string> arg_names;

    for (auto &formal : this->formal_list)
    {
        arg_names.push_back(formal.name);
    }

    return arg_names;
}

Statement::Statement(Type *type, Id *id) : Node()
{
    /* check if symbol already exists with this name*/
    if (symbolTable.isSymbolExist(id->name))
    {
        output::errorDef(yylineno, id->name);
        exit(1);
    }
    /* insert the symbol to the table*/
    symbolTable.insertSymbol(id->name, type->type);
}

Statement::Statement(Type *type, Id *id, Exp *exp) : Node()
{
    /* check if symbol already exists*/
    if (symbolTable.isSymbolExist(id->name))
    {
        output::errorDef(yylineno, id->name);
        exit(1);
    }
    /* check for type mismatch in the assignment*/
    if (SymbolTable::checkTypes(type->type, exp->type) == false)
    {
        /* different types is illegal*/
        output::errorMismatch(yylineno);
        exit(1);
    }
    /* if we got here this statement is ok. insert the new symbol*/
    symbolTable.insertSymbol(id->name, type->type);
    /* add value of the int/byte if it is one of them:*/
//     if(exp->type == "int" || exp->type == "byte") {
//         id->value = exp->value;
//     }
}

Statement::Statement(Id *id, Exp *exp) : Node()
{
    /* if the symbol doesn't exist it is illegal to assign*/
    if (symbolTable.isSymbolExist(id->name) == false)
    {
        output::errorUndef(yylineno, id->name);
        exit(1);
    }
    /* if this symbol exists but a function, it is illegal to assign*/
    if (symbolTable.isFuncSymbolNameExist(id->name))
    {
        output::errorMismatch(yylineno);
        exit(1);
    }
    /* check for type assignment mismatch*/
    if (SymbolTable::checkTypes(symbolTable.getSymbolType(id->name), exp->type) == false)
    {
        output::errorMismatch(yylineno);
        exit(1);
    }
    /* assignment is legal*/
}

Statement::Statement(Call *call) : Node()
{
    /* getting here means the call is legal! so just assign the type:*/
    this->type = call->type;
}

Statement::Statement(const string operation)
{
    if (operation == "return")
    {
        /* check for the return type (has to be void)*/
        if (symbolTable.getClosestReturnType() != "void")
        {
            output::errorMismatch(yylineno);
            exit(1);
        }
    }
    else
    {
        /* if we're not in a loop, this is unexpected*/
        if (symbolTable.isWithinLoop() == false)
        {
            /* print according to the operation*/
            if (operation == "break")
                output::errorUnexpectedBreak(yylineno);
            else
                output::errorUnexpectedContinue(yylineno);
            /*exit anyways*/
            exit(1);
        }
    }
}

Statement::Statement(bool checkIfExpIsBoolean, Exp *exp)
{
    /* check if this is the [RETURN Exp SC] case*/
    if (checkIfExpIsBoolean == false)
    {
        /* check for the return type (has to be the same as exp)*/
        if (!symbolTable.checkTypes(symbolTable.getClosestReturnType(), exp->type))
        {
            output::errorMismatch(yylineno);
            exit(1);
        }
    }
    else if (exp->type != "bool")
    {
        output::errorMismatch(yylineno);
        exit(1);
    }
}

FuncDecl::FuncDecl(const Override *override_node,
                   const RetType *ret_type_node,
                   const Id *id_node,
                   const FormalList *formals_node)
{
    bool override = override_node->override;
    string ret_type = ret_type_node->type;
    string name = id_node->name;
    vector<string> arg_types = formals_node->getTypesVector();

    if (symbolTable.isFuncSymbolNameExist(name))
    {
        /* Check for double definition of the function*/
        /* if it's not double defined, make sure it's override*/
        if (!symbolTable.isSymbolOverride(name))
        {
            if(!override)
            {
                output::errorDef(yylineno, name);
                exit(1);
            }

            output::errorFuncNoOverride(yylineno, name);
            exit(1);
        }

        // Symbol already declared as override

        if (!override)
        {
            output::errorOverrideWithoutDeclaration(yylineno, name);
            exit(1);
        }

        vector<string> ret_types = symbolTable.getFuncDeclReturnTypes(name, arg_types);
        for (auto &match_type : ret_types)
        {
            if (match_type == ret_type)
            {
                output::errorDef(yylineno, name);
                exit(1);
            }
        }
    }

    if (name == "main")
    {
        if (ret_type != "void" || arg_types.size() > 0)
        {
            // output::???(yylineno, name);
            // exit(1);
        }

        if (override)
        {
            output::errorMainOverride(yylineno);
            exit(1);
        }
    }

    symbolTable.insertFuncSymbol(name, ret_type, override, arg_types);

    this->name = name;
    this->type = ret_type;
    this->args_count = arg_types.size();

    symbolTable.pushScope(false, ret_type);
    /* get the names of the args*/
    vector<string> arg_names = formals_node->getNamesVector();
    /* insert them as args (i.e. with negative offsets)*/
    string errorName = symbolTable.insertArgs(arg_types, arg_names);
    if (errorName != "")
    {
        output::errorDef(yylineno, errorName);
        exit(1);
    }
}
