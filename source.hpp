#ifndef COMPI_HW3_SOURCE_H
#define COMPI_HW3_SOURCE_H
#include <string>
#include <vector>
#include <iostream>
#include <assert.h>
// #include "bp.hpp"
#include "common.hpp"

using std::string;
using std::vector;

class Node
{
public:
    string type;

    Node(const string type = "") : type(type) {}

    Node(const Node &node) : type(node.type) {}

    virtual ~Node() = default;
};
#define YYSTYPE Node *

class MarkerM : public Node
{
public:
    MarkerM();
    virtual ~MarkerM() = default;
    string quad;
};

class MarkerN : public Node
{
public:
    MarkerN();
    virtual ~MarkerN() = default;
    vector<LabelLocation> next_list;
};

class Id : public Node
{
public:
    string name;

    Id(const string name) : name(name) {}

    virtual ~Id() = default;
};

class Type : public Node
{
public:
    Type(const string type) : Node(type) {}

    virtual ~Type() = default;
};

class RetType : public Node
{
public:
    RetType(const Type *type_node) : Node(type_node->type) {}

    RetType(const string type) : Node(type) { assert(type == "void"); }

    virtual ~RetType() = default;
};

class BinOp : public Node
{
public:
    enum
    {
        OP_ADDITION,
        OP_SUBTRACTION,
        OP_MULTIPLICATION,
        OP_DIVISION,
    } typedef OpTypes;

    BinOp::OpTypes opType;

    BinOp(const string op);

    virtual ~BinOp() = default;
};

class RelOp : public Node
{
public:
    enum
    {
        OP_EQUAL,
        OP_NOT_EQUAL,
        OP_GREATER_THAN,
        OP_LESS_THAN,
        OP_GREATER_EQUAL,
        OP_LESS_EQUAL,
    } typedef OpTypes;

    RelOp::OpTypes opType;

    RelOp(const string op);

    virtual ~RelOp() = default;
};

class BoolOp : public Node
{
public:
    enum
    {
        OP_AND,
        OP_OR
    } typedef OpTypes;

    BoolOp::OpTypes opType;

    BoolOp(const string op);

    virtual ~BoolOp() = default;
};

class RawNumber : public Node
{
public:
    string value;

    RawNumber(const string value) : value(value) {}

    virtual ~RawNumber() = default;
};
/*FWD decl*/
class Call;

class Exp : public Node
{
private:
    const int MAX_BYTE = 255;

    bool isNumericExp(const Exp *exp) { return (exp->type == "byte" || exp->type == "int"); }

    bool isNumericType(const Type *exp) { return (exp->type == "byte" || exp->type == "int"); }

    bool isBooleanExp(const Exp *exp) { return (exp->type == "bool"); }

    string getArgReg(int offset) { return "%" + std::to_string((-1 - offset)); }

    string loadGetVar(int offset);

public:
    string value;
    string reg;
    bool in_reg() { return reg != ""; };
    string code;
    vector<LabelLocation> true_list;
    vector<LabelLocation> false_list;
    vector<LabelLocation> next_list;

    Exp(); // for the newly created expressions in this assignment

    Exp(const string type, const string value);

    Exp(const RawNumber *num, const string type);

    Exp(bool is_not, const Exp *bool_exp);

    Exp(const Exp *left_exp, const BinOp *op, const Exp *right_exp);

    Exp(const Exp *left_exp, const BoolOp *op, const MarkerM *mark, const Exp *right_exp);

    Exp(const Exp *left_exp, const RelOp *op, const Exp *right_exp);

    // LPAREN Type RPAREN Exp
    Exp(const Type *new_type, const Exp *exp);

    Exp(const Id *id);

    Exp(const Call *call);

    void evaluateBoolToReg();

    virtual ~Exp() = default;
};

class ExpList : public Node
{
public:
    // vector<Exp> exp_list;
    vector<string> exp_list;

    ExpList() {}

    ExpList(const Exp *expression);

    ExpList(const Exp *additional_exp, const ExpList *current_list);

    virtual ~ExpList() = default;

    vector<string> getTypesVector() const;
};

class Call : public Node
{
public:
    string name;
    ExpList exp_list;
    string return_type;

    Call(const string name, ExpList *exp_list = nullptr);

    virtual ~Call() = default;
};

class Override : public Node
{
public:
    bool override;

    Override(const bool override) : override(override) {}

    virtual ~Override() = default;
};

class FormalDecl : public Node
{
public:
    string name;

    FormalDecl(const Type *type, const Id *id);

    virtual ~FormalDecl() = default;
};

class FormalList : public Node
{
public:
    vector<FormalDecl> formal_list;

    FormalList() {}

    FormalList(const FormalDecl *formal_decl);

    FormalList(const FormalDecl *formal_decl, const FormalList *current_list);

    virtual ~FormalList() = default;

    vector<string> getTypesVector() const;
    vector<string> getNamesVector() const;
};
/* FWD declaration*/
class Statement;

class Statements : public Node
{
public:
    vector<LabelLocation> cont_list = {};
    vector<LabelLocation> break_list = {};
    /* Statements: Statement*/
    Statements(Statement *statement);
    /* Statements: Statements Statement*/
    Statements(Statements *statements, Statement *statement);

    ~Statements() = default;
};

class Statement : public Node
{
public:
    vector<LabelLocation> cont_list = {};
    vector<LabelLocation> break_list = {};
    /* Type ID SC*/
    Statement(Type *type, Id *id);
    /* Type ID ASSIGN Exp SC*/
    Statement(Type *type, Id *id, Exp *exp);
    /* ID ASSIGN Exp SC*/
    Statement(Id *id, Exp *exp);
    /* Call SC*/
    Statement(Call *call);
    /* RETURN SC --or-- BREAK SC --or-- CONTINUE SC*/
    Statement(const string operation);
    /* RETURN Exp SC*/
    Statement(Exp *exp);
    /* merge statements lists*/
    Statement();
    void mergeLists(Statements *statements);
    /* IF LPAREN Exp RPAREN M Statement*/
    Statement(Exp *exp, MarkerM *m, Statement *statement);
    /* IF LPAREN Exp RPAREN M Statement ELSE N M Statement*/
    Statement(Exp *exp, MarkerM *trueCondition, Statement *ifStatement, MarkerM *falseCondition, Statement *elseStatement);
    /* WHILE LPAREN M Exp RPAREN M Statement*/
    Statement(MarkerM *loopCondition, Exp *exp, MarkerM *loopStmts, Statement *statement);

    virtual ~Statement() = default;

    /* methods for creating the code */

    void assignCode(Exp *exp, int offset);

    void returnCode(Exp *exp);
};

class FuncDecl : public Node
{
public:
    string name;
    int args_count;

    FuncDecl(const Override *override_node,
             const RetType *ret_type_node,
             const Id *id_node,
             const FormalList *formals_node);

    string returnTypeCode(string ret_type);
    string funcNameCode(string name, int version);
    string formalsCode(vector<string> formals_types);

    virtual ~FuncDecl() = default;
};

/* global functions*/
void isBool(Exp *exp);
void mergeNextList(Exp *exp, MarkerN *n);

#endif
