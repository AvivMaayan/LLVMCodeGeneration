/* Includes for the std DS of the symbol table*/
#ifndef COMPI_HW3_SYMBOL_TABLE_H
#define COMPI_HW3_SYMBOL_TABLE_H
#include <string>
#include <vector>
#include <map>
#include <stack>
#include "hw3_output.hpp"
/* Using sttmnts for easy reding this document*/
using std::map;
using std::stack;
using std::string;
using std::vector;

/* Symbol class - this class represents a symbol in the symbol table*/
class Symbol
{
public:
    /**
     * c'tor that simply assigns the parameters in the equivelant members
     */
    Symbol(const string name, const string type, int offset = 0, bool isOverride = false,
           const string returnType = "", const vector<string> &parameters = {""}) : m_name(name),
                                                                                    m_type(type),
                                                                                    m_offset(offset),
                                                                                    m_isOverride(isOverride),
                                                                                    m_returnType(returnType),
                                                                                    m_parameters(parameters){};

    /**
     * d'tor that clears the vector. The rest of the fields aren't ptrs
     * and don't need "delete"
     */
    ~Symbol() { m_parameters.clear(); };

    /**
     * print the Symbol according to the demands. Using getPrintingType for the special function
     * printing, if needed.
     * We use output::printID for this one.
     */
    void printSymbol() { output::printID(m_name, m_offset, getPrintingType()); };

    /**
     * get the "type" to print for this symbol.
     * @return A regular symbol would return m_type.
     *         Function symbols return the format given by output::makeFunctionType
     */
    string getPrintingType();

    /**
     * get the str upper cased. Used for printing.
     * @param str the string to upper case
     * @return the string upper cased
     */
    string upperCase(const string str);

    /*******************MEMBERS**************************/
    /* Public members since this class is used only by the SymbolTable class*/

    /* The name of the symbol as defined in the code */
    string m_name;
    /* The type of the symbol: [num, char,]*/
    string m_type;
    /* Return value type*/
    string m_returnType;
    /* Boolean stating if the function is declared with 'override'*/
    bool m_isOverride;
    /* The offset of this symbol in the current scope stack*/
    int m_offset;
    /* Parameters of the function */
    vector<string> m_parameters;
};
using PSymbol = Symbol *;

/** Scope class - this class is a scope, i.e. it has all of the symbols defined in this scope within a map*/
class Scope
{
public:
    /*default c'tor and d'tor since the values aren't known yet*/
    Scope(bool isLoop, string returnType = "") : m_symbols(),
                                                 m_isLoop(isLoop),
                                                 m_returnType(returnType){};

    ~Scope();

    string getReturnType() { return m_returnType; };

    /**
     * Insert a symbol to the Scope. No checks are preformed so assumes it is supposed to be inserted.
     * @param psymbol the symbol to add to the scope
     */
    void insertSymbol(PSymbol pSymbol) { m_symbols.push_back(pSymbol); };

    /**
     * Return a boolean stating if exists a symbol with the given name in the Scope
     * @param name the symbol to look for in the scope
     * @return True - if the symbol exsits. False otherwise.
     */
    bool isSymbolExist(const string name);

    /**
     * Get the symbol from the scope if it exists
     * @param name the name of the symbol to get from the scope
     * @param pSymbolOut out parameter for the symbol
     * @return True - the symbol was found and it is inside pSymbolOut
     *         False - the symbol is not in the table
     *         The out parameter is nullptr in case of false
     */
    bool getSymbol(const string name, PSymbol *pSymbolOut);

    /**
     * Get the function symbol from the scope if it exists
     * @param name the name of the function symbol to get from the scope
     * @param parametersTypes the params of the requested function
     * @param pSymbolOut out parameter for the function symbol
     * @return True - the symbol was found and it is inside pSymbolOut
     *         False - the symbol is not in the table
     *         The out parameter is nullptr in case of false
     */
    bool getFuncSymbol(const string name, const vector<string> &parametersTypes,
                       PSymbol *pSymbolOut);

    /* returns if the isLoop member is True / False*/
    bool isLoop() { return m_isLoop; }

    /**
     * Print all of the symbols in the offset according to the demands.
     * Called ONLY at popScope() function.
     */
    void printScope();

    /*******************MEMBERS**************************/
    /* Public members since this class is used only by the SymbolTable class*/

    /* The symbols of this scope is a HT of the symbols in the scope.
     * key == name of the symbol
     * PSymbol == ptr to the symbol located in the cell*/
    vector<PSymbol> m_symbols;
    /* boolean stating if this scope is a loop*/
    bool m_isLoop;
    /* the return type of the scope - gets a value only if this is a function scope*/
    string m_returnType;
    /* the name of the register holding the rbp of this scope*/
    string m_rbp;
};
using PScope = Scope *;

class SymbolTable
{
public:
    /*c'tor and d'tor aren't default*/
    SymbolTable();
    ~SymbolTable();

    /* push a new empty scope to the scope vector*/
    void pushScope(bool isLoop, string returnType = "");

    /* pop the latest scope from the scope vector*/
    void popScope();

    /**
     * Create a symbol from the given parameters and insert to the latest scope. ONLY
     * for non-function symbols. No checks are performed so assumes that the symbol is supposed to be inserted.
     * @param name the name of the symbol to add
     * @param type the type of the symbol to add
     */
    void insertSymbol(const string name, string type);

    /**
     * Create a function symbol from the given parameters and insert to the latest scope.
     * ONLY for function symbols. No checks are performed so assumes that the symbol is supposed to be inserted.
     * @param name the name of the symbol to add
     * @param returnType the return type of the function
     * @param isOverride bool indicating if this function is declared with "override"
     * @param parametersTypes vector of the parameters' TYPES that this function receives
     */
    void insertFuncSymbol(const string name, string returnType, bool isOverride, const vector<string> &parametersTypes);

    /**
     * Return a boolean stating if exists a symbol within any of the Scopes
     * @param name the symbol to look for in the scope
     * @return True - the symbol exist
     *         False - the symbol doesn't exist
     */
    bool isSymbolExist(const string name);

    /**
     * Return a boolean stating if exists a *function* symbol within any of the Scopes
     * @param name the function symbol to look for in the scope
     * @return True - the symbol exist
     *         False - the symbol doesn't exist
     */
    bool isFuncSymbolNameExist(const string name);

    /**
     * Checks if a function named "name" exists. Then it makes sure that the given parameters types are the same (in order also)
     * as the parameters' types in the symbol we found.
     * @param name the name of the function from the call
     * @param parametersTypes vector<string> of the parameters that were stated in the call
     */
    bool isFuncSymbolExist(const string name, const vector<string> &parametersTypes);

    /**
     * Go over all of the scopes:
     * Look for any function named "name" that there is an EXACT type match of the
     * given parametersTypes to its arguments.
     * @param name name of the function
     * @param parametersTypes the types of the declaration
     * @return vector<string> all of the return types of the suitable functions
     *         empty vector if no func allowed
     */
    vector<string> getFuncDeclReturnTypes(const string name, const vector<string> &parametersTypes);

    /**
     * Go over all of the scopes:
     * Look for any function named "name" that there is a possible assignment of the
     * given parametersTypes to its arguments.
     * @param name name of the function in the call
     * @param parametersTypes types in the call
     * @return vector<string> all of the return types of the suitable functions
     *         empty vector if no func allowed
     */
    vector<string> getLegalCallReturnTypes(const string name, const vector<string> &parametersTypes);

    /**
     * Get the symbol's type from any of the Scopes (if it exists).
     * If it does not exist "" is returned.
     * @param name the name of the symbol to get from the scope
     * @return string the type of the symbol
     *         string "" if doesn't exist in any scope
     */
    string getSymbolType(const string name);

    /**
     * Go over the scopes from last to first and find the closest
     * return type.
     * @return string indicating the return type
     */
    string getClosestReturnType();

    /**
     * Checks if there exists a symbol named "name" in the table, and if it also
     * a with value of "True" in the isOverride member
     * @param name the name of the symbol to look for
     * @return True - the symbol exists in one of the scopes && it is a funcSymbol && isOverride == True
     *         False - one of the above doesn't hold
     */
    bool isSymbolOverride(const string name);

    /**
     * Checks if one of the current scopes is within a loop.
     * Iterating over all of the m_scopes member.
     * @return True - one of the scopes is a loop scope
     *         False - there is not any loop scope "open"
     */
    bool isWithinLoop();

    string insertArgs(vector<string> types, vector<string> names);

    static bool checkTypes(string leftType, string rightType);

    void checkMain();

private:
    /* Vector of all of the scopes so far*/
    vector<PScope> m_scopes;
    /* Stack for the offsets as was shown in the tutorial*/
    stack<int> m_offsets;
};
using PSymbolTable = SymbolTable *;

#endif