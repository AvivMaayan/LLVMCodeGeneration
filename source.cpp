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

    /** When we have const 'true' or 'false' we can use reg to store them.
     * This way 'evaluateBoolToReg()' won't be called in assignment or returning.
    */
    if (type == "bool")
    {
        // this->reg = value;
        int address = buffer.emit("br label @");
        if (value == "true")
            this->true_list = buffer.makelist(LabelLocation(address, FIRST));
        else
            this->false_list = buffer.makelist(LabelLocation(address, FIRST));
    }
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

Exp::Exp(bool is_not, const Exp *exp)
    : Node(exp->type)
{
    if (exp->type != "bool")
    {
        this->reg = exp->reg;
        return;
    }

    if (is_not)
    {
        this->value = (exp->value == "true") ? "false" : "true";
        this->true_list = exp->false_list;
        this->false_list = exp->true_list;
    }
    else
    {
        this->value = exp->value;
        this->true_list = exp->true_list;
        this->false_list = exp->false_list;
    }
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
        if (this->type == "int") 
        {
            op_code = "sdiv";
        } else {
            op_code = "udiv";
        }

        buffer.emit("call void @check_division(i32 " + right_exp->reg + ")");
        break;
    }

    buffer.emit(this->reg + " = " + op_code + " i32 " + left_exp->reg + ", " + right_exp->reg);

    if (this->type == "byte")
    {
        /** All BinOps aare calculated as Ints.
         * Threfore, we need to mask out additional bits when the type is Byte */
        string new_reg = buffer.genReg();
        buffer.emit(new_reg + " = and i32 255, " + this->reg);
        this->reg = new_reg;
    }
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
    this->reg = exp->reg;
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

    if (this->type == "bool")
    {
        /** If the stored varibale is boolean, we beed to create a conditioned branch
         * command and lists for backpatching it.
         * To do so we'll compare current value with 'false' and store the result
         * in a new register.
        */
        string new_reg = buffer.genReg();
        string compare_code = new_reg + " = icmp ne i32 0, " + this->reg;
        this->reg = new_reg;
        buffer.emit(compare_code);
        int address = buffer.emit("br i1 " + this->reg + ", label @, label @");
        this->true_list = buffer.makelist(LabelLocation(address, FIRST));
        this->false_list = buffer.makelist(LabelLocation(address, SECOND));
    }
}

string Exp::loadGetVar(int offset)
{
    string rbp = symbolTable.getCurrentRbp();
    return buffer.loadVaribale(rbp, offset);
}

Exp::Exp(const Call *call) : Node(call->return_type) {}

void Exp::evaluateBoolToReg()
{
    assert(this->type == "bool");
    assert(!this->in_reg());

    string true_label = buffer.genLabel();
    LabelLocation true_jump_to_phi_loc = buffer.emitJump();
    buffer.bpatch(this->true_list, true_label);

    string false_label = buffer.genLabel();
    LabelLocation false_jump_to_phi_loc = buffer.emitJump();
    buffer.bpatch(this->false_list, false_label);

    string phi_label = buffer.genLabel();
    vector<LabelLocation> phi_jump_locations = buffer.merge(
        buffer.makelist(true_jump_to_phi_loc),
        buffer.makelist(false_jump_to_phi_loc));
    buffer.bpatch(phi_jump_locations, phi_label);

    this->reg = buffer.genReg();
    buffer.emit(this->reg + " = phi i32 [1, %" + true_label + "], [0, %" + false_label + "]");
}

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

void Statements::enforceReturn()
{
    if (this->return_in_last)
        return;

    string return_type_c = symbolTable.getClosestReturnType();
    if (return_type_c != "void")
    {
        string return_type_llvm = buffer.typeCode(return_type_c);
        string default_val_llvm = buffer.getDefaultValue(return_type_c);

        buffer.emit("ret " + return_type_llvm + " " + default_val_llvm);
        return;
    }

    buffer.emit("ret void");
    return;
}

Statements::Statements(Statement *statement)
{
    /* merge the lists of the Statements and Statement*/
    this->break_list = buffer.merge(this->break_list, statement->break_list);
    this->cont_list = buffer.merge(this->cont_list, statement->cont_list);
    this->return_in_last = statement->return_statement;
    delete statement;
}

Statements::Statements(Statements *statements, Statement *statement) : Node(), cont_list(), break_list()
{
    /* merge the lists of the Statements and Statement that were given into this one*/
    this->break_list = buffer.merge(statements->break_list, statement->break_list);
    this->cont_list = buffer.merge(statements->cont_list, statement->cont_list);
    this->return_in_last = statement->return_statement;

    delete statement;
    delete statements;
}

/* Type ID SC --- int x; */
Statement::Statement(Type *type, Id *id) : Node(), break_list(), cont_list()
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
    /* store default value within this variable on the stack*/
    buffer.storeVariable(symbolTable.getCurrentRbp(), offset, buffer.getDefaultValue(this->type));
}

/* Type ID ASSIGN Exp SC --- int x = 6*/
Statement::Statement(Type *type, Id *id, Exp *exp) : Node(), break_list(), cont_list()
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
    assignCode(exp, offset);
}

/* ID ASSIGN Exp SC*/
Statement::Statement(Id *id, Exp *exp) : Node(), break_list(), cont_list()
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
    /* the case of an assignemnt to a parameter isn't supposed to be checked*/
    assert(offset >= 0);
    assignCode(exp, offset);
}

/* Call SC*/
/* DID NOT implement code generation YET*/
Statement::Statement(Call *call) : Node()
{
    /* getting here means the call is legal! so just assign the type:*/
    this->type = call->type;
}

/* RETURN SC --or-- BREAK SC --or-- CONTINUE SC*/
Statement::Statement(const string operation) : Node(), break_list(), cont_list()
{
    if (operation == "return")
    {
        /* check for the return type (has to be void)*/
        if (symbolTable.getClosestReturnType() != "void")
        {
            output::errorMismatch(yylineno);
            exit(1);
        }
        /******************* code generation: *****************************/
        this->return_statement = true;
        buffer.emit("ret void");
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
        /* 'break' or 'continue' both require a jump that will later be backpatched*/
        int address = buffer.emit("br label @");
        if (operation == "break")
        {
            /* create break list for this break command*/
            this->break_list = buffer.makelist(LabelLocation(address, FIRST));
        }
        else
        { /* the operation is continue*/
            /* create continue list for this continue command*/
            this->cont_list = buffer.makelist(LabelLocation(address, FIRST));
        }
    }
}

/* RETURN Exp SC*/
Statement::Statement(Exp *exp) : Node(), break_list(), cont_list()
{
    /* check for the return type (has to be the same as exp)*/
    if (!symbolTable.checkTypes(symbolTable.getClosestReturnType(), exp->type))
    {
        output::errorMismatch(yylineno);
        exit(1);
    }
    /******************* code generation: *****************************/
    this->return_statement = true;
    returnCode(exp);
}

/* merge statements lists*/
Statement::Statement() : Node(), break_list(), cont_list()
{
}

void Statement::mergeLists(Statements *statements)
{
    break_list = buffer.merge(break_list, statements->break_list);
    cont_list = buffer.merge(cont_list, statements->cont_list);
    delete statements;
}

/* IF LPAREN Exp RPAREN M Statement*/
Statement::Statement(Exp *exp, MarkerM *m, Statement *statement) : Node(), break_list(), cont_list()
{
    /* exp is a boolean, no need to check*/
    /* merge the break and continue lists with the ones of the statement*/
    this->break_list = buffer.merge(break_list, statement->break_list);
    this->cont_list = buffer.merge(cont_list, statement->cont_list);

    /**
     * backpatch the true_list of the expression (the condition) with the stmts
     * within the "if" scope
     * */
    buffer.bpatch(exp->true_list, m->quad);

    /* generate the label that states the "if" condition is false, and emit it*/
    string falseLabel = buffer.genLabel();
    /* backpatch the false and next list of the expression (the condition)*/
    buffer.bpatch(exp->false_list, falseLabel);
    buffer.bpatch(exp->next_list, falseLabel);
}

/* IF LPAREN Exp RPAREN M Statement ELSE N M Statement*/
Statement::Statement(Exp *exp, MarkerM *trueCondition, Statement *ifStatement, MarkerM *falseCondition, Statement *elseStatement) : Node(), break_list(), cont_list()
{
    /* exp is a boolean, no need to check*/
    /**
     * merge the continue and break lists of both statements, since when this if-else is within a loop,
     * both continue and break need to jump to the same location.
     */
    this->cont_list = buffer.merge(ifStatement->cont_list, elseStatement->cont_list);
    this->break_list = buffer.merge(ifStatement->break_list, elseStatement->break_list);
    /* backpatch the true list of the condition to jump to the inner part of the if block*/
    buffer.bpatch(exp->true_list, trueCondition->quad);
    /* backpatch the false list of the condition to jump to the inner part of the else block*/
    buffer.bpatch(exp->false_list, falseCondition->quad);
    /* the next operation to perform is a new label, outside of both if and else blocks*/
    string outLabel = buffer.genLabel();
    buffer.bpatch(exp->next_list, outLabel);
}

/* WHILE LPAREN M Exp RPAREN M Statement*/
Statement::Statement(MarkerM *loopCondition, Exp *exp, MarkerM *loopStmts, Statement *statement) : Node(), break_list(), cont_list()
{
    /* exp is a boolean, no need to check*/
    /* emit the correct label for the condition of the loop*/
    string loopLabel = loopCondition->quad;
    if (loopLabel[0] != '%' && loopLabel[0] != '@')
    {
        buffer.emit("br label %" + loopLabel);
    }
    else
    {
        buffer.emit("br label " + loopLabel);
    }
    /* emit another label for getting out of the loop*/
    string outLabel = buffer.genLabel();
    /* if the condition is true, bp to jump to the statements*/
    buffer.bpatch(exp->true_list, loopStmts->quad);
    /* otherwise, jump out of the loop*/
    buffer.bpatch(exp->false_list, outLabel);
    buffer.bpatch(exp->next_list, outLabel);
    /* the breaks within the loop should go out of the loop*/
    buffer.bpatch(statement->break_list, outLabel);
    /* the continues within the loop should go back to the condition*/
    buffer.bpatch(statement->cont_list, loopCondition->quad);
}

/* methods for creating the code */

/**
 * creates and emits the code for storing a variable within a reg into an address on the stack.
 * @note: if the exp given is NOT an id variable, Aviv wrote evaluation function to insert its value in a reg
 * @param exp the expression to insert to the stack
 * @param offset the offset after the rbp that this variable needs to be inserted to
 */
void Statement::assignCode(Exp *exp, int offset)
{
    if (!exp->in_reg())
    {
        assert(exp->type == "bool");
        exp->evaluateBoolToReg();
    }
    buffer.storeVariable(symbolTable.getCurrentRbp(), offset, exp->reg);
}

/**
 * creates and emits a code for a return command in LLVM
 *
 * @param exp the expression to return in the LLVM code
 */
void Statement::returnCode(Exp *exp)
{
    /* convert return type to LLVM syntax*/
    string returnType = (symbolTable.getClosestReturnType() == "string")
                        ? "i8*"
                        : "i32";

    /* make sure exp->reg has the correct result*/
    if (!exp->in_reg())
    {
        assert(exp->type == "bool");
        exp->evaluateBoolToReg();
    }
    /* emit the return command*/
    buffer.emit("ret " + returnType + " " + exp->reg);
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

    int version = symbolTable.insertFuncSymbol(name, ret_type, override, arg_types);

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

    buffer.emit("define " + buffer.typeCode(ret_type) + " " + funcNameCode(name, version) + formalsCode(arg_types));
    buffer.emitLeftBrace();
    symbolTable.setCurrentRbp(buffer.allocFunctionRbp());
}

string FuncDecl::funcNameCode(string name, int version)
{
    return "@" + name + "_" + std::to_string(version);
}

string FuncDecl::formalsCode(vector<string> formals_types)
{
    string formals_str = "(";

    for (string formal_type : formals_types)
    {
        formals_str += (formal_type == "string") ? "i8*" : "i32";
        formals_str += ", ";
    }

    return formals_str.substr(0, formals_str.size() - 2) + ")";
}

MarkerM::MarkerM()
{
    /** Although in the lacture's IR line numbers were used for backpatching-
     *  in LLVM we must use labels.
     *  Therefore, instead of saving #line to this->quad-
     *  we generate a fresh label, emit it and save it as quad.
     */

    this->quad = buffer.genLabel();
}

MarkerN::MarkerN()
{
    int address = buffer.emit("br label @");
    this->next_list = buffer.makelist(LabelLocation(address, FIRST));
}

void isBool(Exp *exp)
{
    if (exp->type != "bool")
    {
        output::errorMismatch(yylineno);
        exit(1);
    }
}

void mergeNextList(Exp *exp, MarkerN *n)
{
    exp->next_list = buffer.merge(n->next_list, exp->next_list);
}
