#include "symbol_table_intf.h"
#include <assert.h>

bool compareTypeVectors(const vector<string> &v1, const vector<string> &v2)
{
    /* sizes have to be the same*/
    if (v1.size() != v2.size())
        return false;
    /* Go over all of the elements and compare*/
    for (int i = 0; i < v1.size(); i++)
    {
        if (!SymbolTable::checkTypes(v1[i], v2[i]))
        {
            return false;
        }
    }
    return true;
}

/* CLASS Symbol*/

string Symbol::getPrintingType()
{
    /* if this symbol isn't a function, no special printing is needed*/
    if (m_type != "func")
    {
        return upperCase(m_type);
    }
    /* if it's a func, upper case the parameters*/
    vector<string> upperParameters;
    for (auto it = m_parameters.begin(); it != m_parameters.end(); it++)
    {
        upperParameters.push_back(upperCase(*it));
    }
    /* Use the given function*/
    return output::makeFunctionType(upperCase(m_returnType), upperParameters);
}

string Symbol::upperCase(const string str)
{
    if (str == "bool")
        return "BOOL";
    else if (str == "byte")
        return "BYTE";
    else if (str == "int")
        return "INT";
    else if (str == "void")
        return "VOID";
    else
        return "STRING";
}

/* CLASS Scope */

Scope::~Scope()
{
    for (auto &sym : m_symbols)
    {
        delete sym;
    }
    m_symbols.clear();
}

bool Scope::isSymbolExist(const string name)
{
    for (auto &sym : m_symbols)
    {
        if (sym->m_name == name)
            return true;
    }
    return false;
}

bool Scope::getSymbol(const string name, PSymbol *pSymbolOut)
{
    if (!isSymbolExist(name))
    {
        *pSymbolOut = nullptr;
        return false;
    }
    /* otherwise assign the correct value (using a copy c'tor) and return true*/
    for (auto &sym : m_symbols)
    {
        if (sym->m_name == name)
        {
            *pSymbolOut = sym;
            return true;
        }
    }
    /* not supposed to be here*/
    assert(false);
}

void Scope::printScope()
{
    for (auto &sym : m_symbols)
    {
        /* assign the symbol within the map to pSymbol*/
        sym->printSymbol();
    }
}

bool Scope::getFuncSymbol(const string name, const vector<string> &parametersTypes,
                          PSymbol *pSymbolOut)
{
    /* get the iterator of the symbol*/
    getSymbol(name, pSymbolOut);
    /* if it's not a function, return false*/
    if ((*pSymbolOut)->m_type != "func")
        return false;
    /* if it's not the same params, return false*/
    if ((*pSymbolOut)->m_parameters != parametersTypes)
        return false;
    return true;
}

/* CLASS SymbolTable */

SymbolTable::SymbolTable() : m_scopes(), m_offsets(), m_distributer(0)
{
    /* Add a new non-loop (hence the false) Scope*/
    pushScope(false);
    /* Add the basic two functions as symbols*/
    insertFuncSymbol("print", "void", false, {"string"});
    insertFuncSymbol("printi", "void", false, {"int"});
}

SymbolTable::~SymbolTable()
{
    /* Go over the Scope vector and release + pop each Scope*/
    for (auto it = m_scopes.begin(); it != m_scopes.end(); it++)
    {
        delete *it;
    }
}

void SymbolTable::pushScope(bool isLoop, string returnType)
{
    /* Allocate a new empty Scope*/
    PScope scope = new Scope(isLoop, returnType);
    string rbp;
    /* Handling for the first Scope inserted*/
    if (m_offsets.empty())
    {
        /* Start the first scope with 0 offset*/
        m_offsets.push(0);
        /* Address is empty for the first rbp*/
        rbp = "";
    }
    else
    {
        /* Duplicate the offset at the top of the offsets stack*/
        int offset = m_offsets.top();
        m_offsets.push(offset);
        /* Address for the next rbp is the last one in the m_scopes vec*/
        rbp = m_scopes.back()->m_rbp;
    }
    /* Update to correct rbp*/
    scope->m_rbp = rbp;
    /* push the scope to the stack*/
    m_scopes.push_back(scope);
}

void SymbolTable::popScope()
{
    /* assert that the sizes of the two DS fit. Bug otherwise*/
    assert(m_scopes.size() == m_offsets.size());
    /* assert that there is indeed something to pop*/
    assert(m_scopes.size() > 0);
    /* Get the current scope*/
    PScope pScope = m_scopes.back();
    /* Pop the scope from the vector*/
    m_scopes.pop_back();
    /* Print end scope*/
    output::endScope();
    /* Print all of the symbols in the scope*/
    pScope->printScope();
    /* Pop offsets stack*/
    m_offsets.pop();
    /* Release memory*/
    delete pScope;
}

int SymbolTable::insertSymbol(const string name, string type)
{
    /* assert that there is a Scope with an offset already*/
    assert(m_offsets.size() > 0 && m_scopes.size() > 0);
    /* Get current offset*/
    int offset = m_offsets.top();
    /* Create the new symbol*/
    PSymbol pSymbol = new Symbol(name, type, offset);
    /* Add it to the current scope*/
    m_scopes.back()->insertSymbol(pSymbol);
    /* Update offset head stack to +1*/
    m_offsets.pop();
    m_offsets.push(offset + 1);
    /** return the offset of the symbol inserted*/
    return offset;
}

int SymbolTable::insertFuncSymbol(const string name, string returnType, bool isOverride,
                                   const vector<string> &parametersTypes)
{
    assert(m_offsets.size() > 0 && m_scopes.size() > 0);
    /* Get current offset*/
    int offset = m_offsets.top();
    /* Create the new function symbol*/
    int version = m_distributer;
    PSymbol pFuncSymbol = new Symbol(name, "func", 0, isOverride, version, returnType, parametersTypes);
    /* Update the distributer*/
    m_distributer++;
    /* Add it to the current scope*/
    m_scopes.back()->insertSymbol(pFuncSymbol);
    /* return the version of the function inserted*/
    return version;
}

bool SymbolTable::isSymbolExist(const string name)
{
    /* Search all scopes one at a time from the begining*/
    for (auto it = m_scopes.begin(); it != m_scopes.end(); it++)
    {
        /* (*it) is the current scope*/
        if ((*it)->isSymbolExist(name))
        {
            return true;
        }
    }
    return false;
}

int SymbolTable::getSymbolOffset(const string name)
{
    PSymbol pSymbol;
    /* Search all scopes one at a time from the begining for the symbol*/
    for (auto it = m_scopes.begin(); it != m_scopes.end(); it++)
    {
        /* (*it) is the current scope*/
        if ((*it)->getSymbol(name, &pSymbol) == true)
        {
            /* we found the symbol*/
            return pSymbol->m_offset;
        }
    }
    /* not supposed to get here*/
    assert(false);
}

int SymbolTable::getFuncSymbolVersion(const string name)
{
    PSymbol pSymbol;
    /* Search all scopes one at a time from the begining for the symbol*/
    for (auto it = m_scopes.begin(); it != m_scopes.end(); it++)
    {
        /* (*it) is the current scope*/
        if ((*it)->getSymbol(name, &pSymbol) == true)
        {
            /* we found the symbol*/
            return pSymbol->m_version;
        }
    }
    /* the symbol doesn't exist*/
    return -1;
}

bool SymbolTable::isFuncSymbolNameExist(const string name)
{
    return getSymbolType(name) == "func";
}

vector<string> SymbolTable::getFuncDeclReturnTypes(const string name, const vector<string> &parametersTypes)
{
    PScope pScope;
    PSymbol pSymbol;
    vector<string> returnTypes;
    /* Search all scopes one at a time from the begining*/
    for (auto it = m_scopes.begin(); it != m_scopes.end(); it++)
    {
        pScope = *it;
        /* Go over all of the symbols in the scope*/
        for (auto symIt = pScope->m_symbols.begin(); symIt != pScope->m_symbols.end(); symIt++)
        {
            pSymbol = *symIt;
            /* if the symbol has the same name && if the symbol has the same parameters types EXACTLY*/
            if (pSymbol->m_name == name && pSymbol->m_parameters == parametersTypes)
            {
                returnTypes.push_back(pSymbol->m_returnType);
            }
        }
    }
    return returnTypes;
}

vector<pair<string, int>> SymbolTable::getLegalCallReturnTypes(const string name, const vector<string> &parametersTypes)
{
    PScope pScope;
    PSymbol pSymbol;
    vector<pair<string, int>> returnTypes;
    /* Search all scopes one at a time from the begining*/
    for (auto it = m_scopes.begin(); it != m_scopes.end(); it++)
    {
        pScope = *it;
        /* Go over all of the symbols in the scope*/
        for (auto symIt = pScope->m_symbols.begin(); symIt != pScope->m_symbols.end(); symIt++)
        {
            pSymbol = *symIt;
            /* if the symbol has the same name && if the symbol has the "same" parameters types*/
            if (pSymbol->m_name == name && compareTypeVectors(pSymbol->m_parameters, parametersTypes))
            {
                auto pair = std::pair<string, int>(pSymbol->m_returnType, pSymbol->m_version);
                returnTypes.push_back(pair);
            }
        }
    }
    return returnTypes;
}

bool SymbolTable::isFuncSymbolExist(const string name, const vector<string> &parametersTypes)
{
    PSymbol pSymbol;
    /* Search all scopes one at a time from the begining*/
    for (auto it = m_scopes.begin(); it != m_scopes.end(); it++)
    {
        /* (*it) is the current scope. Try to get the function symbol from the scope.*/
        if ((*it)->getFuncSymbol(name, parametersTypes, &pSymbol))
        {
            return true;
        }
    }
    return false;
}

string SymbolTable::getSymbolType(const string name)
{
    PSymbol pSymbol;
    /* Search all scopes one at a time from the begining*/
    for (auto it = m_scopes.begin(); it != m_scopes.end(); it++)
    {
        /* (*it) is the current scope. Try to get the symbol from the scope.*/
        if ((*it)->getSymbol(name, &pSymbol))
        {
            /* otherwise, we found it*/
            return pSymbol->m_type;
        }
    }
    return "";
}

string SymbolTable::getClosestReturnType()
{
    /* go over the vectors from last to first using reverse iterator*/
    for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); it++)
    {
        /* (*it) is the current scope */
        string scopeReturn = (*it)->getReturnType();
        /* if the scopeReturn is defined, return it*/
        if (scopeReturn != "")
        {
            return scopeReturn;
        }
    }
    return "";
}

string SymbolTable::getCurrentRbp()
{
    PScope currentScope = m_scopes.back();
    /* the m_scopes is not suppose to be empty. if so, it's a bug*/
    assert(currentScope != nullptr);
    return currentScope->m_rbp;
}

void SymbolTable::setCurrentRbp(string newRbp)
{
    PScope currentScope = m_scopes.back();
    /* the m_scopes is not suppose to be empty. if so, it's a bug*/
    assert(currentScope != nullptr);
    /* update the rbp*/
    currentScope->m_rbp = newRbp; 
}

bool SymbolTable::isSymbolOverride(const string name)
{
    PSymbol pSymbolToCheck;
    /* Search all scopes one at a time from the begining*/
    for (auto it = m_scopes.begin(); it != m_scopes.end(); it++)
    {
        /* (*it) is the current scope. Try to get the symbol from the scope.*/
        if ((*it)->getSymbol(name, &pSymbolToCheck))
        {
            /* we found it. return if override AND a function symbol*/
            return pSymbolToCheck->m_isOverride && pSymbolToCheck->m_type == "func";
        }
    }
    return false;
}

bool SymbolTable::isWithinLoop()
{
    /* Search all scopes one at a time from the begining*/
    for (auto it = m_scopes.begin(); it != m_scopes.end(); it++)
    {
        /* (*it) is the current scope*/
        if ((*it)->isLoop())
        {
            return true;
        }
    }
    return false;
}

string SymbolTable::insertArgs(vector<string> types, vector<string> names)
{
    int offset = -1;
    assert(types.size() == names.size());
    for (int i = 0; i < types.size(); i++)
    {
        /* if already exists*/
        if (isSymbolExist(names[i]))
        {
            return names[i];
        }
        /* Create the new function symbol*/
        PSymbol pSymbol = new Symbol(names[i], types[i], offset);
        /* Add it to the current scope*/
        m_scopes.back()->insertSymbol(pSymbol);
        offset--;
    }
    return "";
}

bool SymbolTable::checkTypes(string leftType, string rightType)
{
    /* if the types are the same, everything is fine*/
    if (leftType == rightType)
    {
        return true;
    }
    /* check for the (int <-- byte) scenario*/
    if (leftType == "int" && rightType == "byte")
    {
        return true;
    }
    /* assignment is wrong*/
    return false;
}

void SymbolTable::checkMain()
{
    vector<string> returnTypesFromMain = this->getFuncDeclReturnTypes("main", {});

    if (returnTypesFromMain.size() == 1 && returnTypesFromMain[0] == "void")
    {
        this->popScope();
    }
    else
    {
        output::errorMainMissing();
        exit(1);
    }
}
