#include "source.hpp"
#include "hw3_output.hpp"
#include "symbol_table_intf.h"
#include "bp.hpp"

extern int yylineno;
extern SymbolTable symbolTable;
extern CodeBuffer &buffer;

BinOp::BinOp(const string op)
{
    if (op == "+")
        this->opType = BinOp::OpTypes::OP_ADDITION;
    else if (op == "-")
        this->opType = BinOp::OpTypes::OP_SUBTRACTION;
    else if (op == "*")
        this->opType = BinOp::OpTypes::OP_MULTIPLICATION;
    else if (op == "/")
        this->opType = BinOp::OpTypes::OP_DIVISION;
    else
        exit(1);
}

RelOp::RelOp(const string op)
{
    if (op == "==")
        this->opType = RelOp::OpTypes::OP_EQUAL;
    else if (op == "!=")
        this->opType = RelOp::OpTypes::OP_NOT_EQUAL;
    else if (op == ">")
        this->opType = RelOp::OpTypes::OP_GREATER_THAN;
    else if (op == "<")
        this->opType = RelOp::OpTypes::OP_LESS_THAN;
    else if (op == ">=")
        this->opType = RelOp::OpTypes::OP_GREATER_EQUAL;
    else if (op == "<=")
        this->opType = RelOp::OpTypes::OP_LESS_EQUAL;
    else
        exit(1);
}

BoolOp::BoolOp(const string op)
{
    if (op == "and")
        this->opType = BoolOp::OpTypes::OP_AND;
    else if (op == "or")
        this->opType = BoolOp::OpTypes::OP_OR;
    else
        exit(1);
}

Exp::Exp() : Node(){};

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

    // Since this is a constant number, no register is needed and
    // we can abuse the reg member to store the value itself
    this->reg = num->value;
}

Exp::Exp(const Exp *bool_exp)
    : Node(bool_exp->type)
{
    assert(bool_exp->type == "bool");

    this->value = (bool_exp->value == "true") ? "false" : "true";

    this->true_list = bool_exp->false_list;
    this->false_list = bool_exp->true_list;
}

Exp::Exp(const Exp *left_exp, const BinOp *op, const Exp *right_exp)
{
    if (!isNumericExp(left_exp) || !isNumericExp(right_exp))
    {
        output::errorMismatch(yylineno);
        exit(1);
    }

    this->type = (left_exp->type == "int" || right_exp->type == "int") ? "int" : "byte";

    this->reg = buffer.genReg();

    string op_code;
    switch (op->opType)
    {
    case BinOp::OpTypes::OP_ADDITION:
        op_code = "add";
        break;
    case BinOp::OpTypes::OP_SUBTRACTION:
        op_code = "sub";
        break;
    case BinOp::OpTypes::OP_MULTIPLICATION:
        op_code = "mul";
        break;
    case BinOp::OpTypes::OP_DIVISION:
        /** @todo: address udiv or sdiv */
        op_code = "div";
        break;
    }

    string code = this->reg + " = " + op_code + " i32 " + left_exp->reg + ", " + right_exp->reg;
    buffer.emit(code);
}

Exp::Exp(const Exp *left_exp, const BoolOp *op, const MarkerM *mark, const Exp *right_exp)
    : Node("bool")
{
    if (!isBooleanExp(left_exp) || !isBooleanExp(right_exp))
    {
        output::errorMismatch(yylineno);
        exit(1);
    }

    switch (op->opType)
    {
    case BoolOp::OpTypes::OP_OR:
        buffer.bpatch(left_exp->false_list, mark->quad);
        this->true_list = buffer.merge(left_exp->true_list, right_exp->true_list);
        this->false_list = vector<LabelLocation>(right_exp->false_list);
        break;
    case BoolOp::OpTypes::OP_AND:
        buffer.bpatch(left_exp->true_list, mark->quad);
        this->true_list = vector<LabelLocation>(right_exp->true_list);
        this->false_list = buffer.merge(left_exp->false_list, right_exp->false_list);
        break;
    }
}

Exp::Exp(const Exp *left_exp, const RelOp *op, const Exp *right_exp)
    : Node("bool")
{
    if (!isNumericExp(left_exp) || !isNumericExp(right_exp))
    {
        output::errorMismatch(yylineno);
        exit(1);
    }

    this->reg = buffer.genReg();

    string op_code;
    switch (op->opType)
    {
    case RelOp::OpTypes::OP_EQUAL:
        op_code = "eq";
        break;
    case RelOp::OpTypes::OP_NOT_EQUAL:
        op_code = "ne";
        break;
    case RelOp::OpTypes::OP_GREATER_THAN:
        op_code = "sgt";
        break;
    case RelOp::OpTypes::OP_LESS_THAN:
        op_code = "slt";
        break;
    case RelOp::OpTypes::OP_GREATER_EQUAL:
        op_code = "sge";
        break;
    case RelOp::OpTypes::OP_LESS_EQUAL:
        op_code = "sle";
        break;
    }

    string compare_code = this->reg + " = icmp " + op_code + " i32 " + left_exp->reg + ", " + right_exp->reg;
    buffer.emit(compare_code);
    int address = buffer.emit("br i1 " + this->reg + ", label @, label @");
    this->true_list = buffer.makelist(LabelLocation(address, FIRST));
    this->false_list = buffer.makelist(LabelLocation(address, SECOND));
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

    int offset = symbolTable.getSymbolOffset(id->name);
    bool is_arg = (offset < 0);
    this->reg = (is_arg) ? this->getArgReg(offset) : this->loadGetVar(offset);
}

string Exp::loadGetVar(int offset)
{
    string rbp = symbolTable.getCurrentRbp();
    return buffer.loadVaribale(rbp, offset);
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

/* Type ID SC*/
Statement::Statement(Type *type, Id *id) : Node()
{
    /* check if symbol already exists with this name*/
    if (symbolTable.isSymbolExist(id->name))
    {
        output::errorDef(yylineno, id->name);
        exit(1);
    }
    /* insert the symbol to the table*/
    int offset = symbolTable.insertSymbol(id->name, type->type);
    this->type = type->type;
    /******************* code generation: *****************************/
    Exp *exp = new Exp();
    exp->reg = buffer.genReg();
    exp->type = type->type;
    if (this->type == "bool")
    {
        exp->value = "false";
        buffer.boolCode(exp);
    }
    else
    {
        exp->value = "0";
        buffer.numCode(exp->reg, "0");
        buffer.assignCode(exp, offset, type->type);
    }
    delete exp;
}

/* Type ID ASSIGN Exp SC --- int x = 6*/
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
    int offset = symbolTable.insertSymbol(id->name, type->type);
    /******************* code generation: *****************************/
    buffer.assignCode(exp, offset, type->type);
}

/* ID ASSIGN Exp SC*/
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
    /******************* code generation: *****************************/
    int offset = symbolTable.getSymbolOffset(id->name);
    string type = symbolTable.getSymbolType(id->name);
    buffer.assignCode(exp, offset, type);
}

/* Call SC*/
/* DID NOT implement code generation YET*/
Statement::Statement(Call *call) : Node()
{
    /* getting here means the call is legal! so just assign the type:*/
    this->type = call->type;
}

/* RETURN SC --or-- BREAK SC --or-- CONTINUE SC*/
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

/* RETURN Exp SC --or-- IF LPAREN Exp RPAREN Statement
 * --or-- IF LPAREN Exp RPAREN Statement ELSE Statement
 * --or-- WHILE LPAREN Exp RPAREN Statement*/
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
        /******************* code generation: *****************************/
        string returnType = symbolTable.getClosestReturnType();
        string rbp = symbolTable.getCurrentRbp();
        // int offset = symbolTable.getSymbolOffset(exp->name);
    }
    else if (exp->type != "bool")
    {
        output::errorMismatch(yylineno);
        exit(1);
    }
}

/**
 * Merge the lists of the given statement with this one.
 * @param statement - the stmt to merge its list to ours
 */
void Statement::mergeStatements(Statement *statement)
{
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
            if (!override)
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

MarkerM::MarkerM()
{
    this->quad = buffer.nextquad();
    // this->quad = buffer.genLabel();
    // buffer.labelEmit(this->quad);
}

MarkerN::MarkerN()
{
    int address = buffer.emit("br label @");
    this->next_list = buffer.makelist(LabelLocation(address, FIRST));
}
