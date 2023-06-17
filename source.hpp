#ifndef COMPI_HW3_SOURCE_H
#define COMPI_HW3_SOURCE_H
#include <string>
#include <vector>
#include <iostream>
#include <assert.h>


using std::string;
using std::vector;

class Node
{
public:
    string type;

    Node(const string type = "") : type(type) {}

    Node(const Node &node): type(node.type) {}

    virtual ~Node() = default;
};
#define YYSTYPE Node*

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
    BinOp(const string type) : Node(type) {}

    virtual ~BinOp() = default;
};

class RelOp : public Node
{
public:
    RelOp(const string type) : Node(type) {}

    virtual ~RelOp() = default;
};

class BoolOp : public Node
{
public:
    BoolOp(const string type) : Node(type) {}

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

public:
    string value;

    Exp(const string type, const string value);

    Exp(const RawNumber *num, const string type);

    // NOT Exp
    Exp(const Exp *bool_exp);

    Exp(const Exp *left_exp, const BinOp *op, const Exp *right_exp);

    Exp(const Exp *left_exp, const BoolOp *op, const Exp *right_exp);

    Exp(const Exp *left_exp, const RelOp *op, const Exp *right_exp);

    // LPAREN Type RPAREN Exp
    Exp(const Type *new_type, const Exp *exp);

    Exp(const Id *id);

    Exp(const Call *call);

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

class Statement : public Node
{
public:
    /* Type ID SC*/
    Statement(Type *type, Id *id);
    /* Type ID ASSIGN Exp SC --- int x = 6*/
    Statement(Type *type, Id *id, Exp *exp);
    /* ID ASSIGN Exp SC*/
    Statement(Id *id, Exp *exp);
    /* Call SC*/
    Statement(Call *call);
    /* RETURN SC --or-- BREAK SC --or-- CONTINUE SC*/
    Statement(const string operation);
    /* RETURN Exp SC --or-- IF LPAREN Exp RPAREN Statement
     * --or-- IF LPAREN Exp RPAREN Statement ELSE Statement
     * --or-- WHILE LPAREN Exp RPAREN Statement*/
    Statement(bool checkIfExpIsBoolean, Exp *exp);

    virtual ~Statement() = default;
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

    virtual ~FuncDecl() = default;
};
#endif