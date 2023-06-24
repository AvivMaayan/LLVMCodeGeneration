#include "bp.hpp"
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

bool replace(string &str, const string &from, const string &to, const BranchLabelIndex index);

CodeBuffer::CodeBuffer() : buffer(), globalDefs(), regCounter(0) {}

void CodeBuffer::emitGlobals()
{
    this->emitFile("print_functions.llvm");
}

/**
 * With the Singleton design pattern, the single instance of CodeBuffer could be accessed using:
 * CodeBuffer &buffer = CodeBuffer::instance();
 */
CodeBuffer &CodeBuffer::instance()
{
    static CodeBuffer inst; // Only instance
    return inst;
}
/**
 * generates a jump location label for the next command, writes it to the buffer and returns it
 */
string CodeBuffer::genLabel()
{
    std::stringstream label;
    label << "label_";
    label << buffer.size();
    std::string ret(label.str());
    label << ":";
    emit(label.str());
    return ret;
}
/**
 * helper function that returns (the name of) a fresh variable
 */
string CodeBuffer::genReg(bool isGlobal)
{
    string regPrefix = (isGlobal) ? "@" : "%";
    return regPrefix + "var_" + std::to_string(regCounter++);
}
/**
 * writes command to the buffer, returns its location in the buffer
 */
int CodeBuffer::emit(const string &s)
{
    buffer.push_back(s);
    return buffer.size() - 1;
}

void CodeBuffer::emitFile(const string &path)
{
    std::ifstream file_stream(path);
    string line;

    while(std::getline(file_stream, line))
    {
        emitGlobal(line);
    }
}

/**
accepts a list of {buffer_location, branch_label_index} items and a label.
For each {buffer_location, branch_label_index} item in address_list, backpatches the branch command
at buffer_location, at index branch_label_index (FIRST or SECOND), with the label.
note - the function expects to find a '@' char in place of the missing label.
note - for unconditional branches (which contain only a single label) use FIRST as the branch_label_index.
example #1:
int loc1 = emit("br label @");  - unconditional branch missing a label. ~ Note the '@' ~
bpatch(makelist({loc1,FIRST}),"my_label"); - location loc1 in the buffer will now contain the command "br label %my_label"
note that index FIRST referes to the one and only label in the line.
example #2:
int loc2 = emit("br i1 %cond, label @, label @"); - conditional branch missing two labels.
bpatch(makelist({loc2,SECOND}),"my_false_label"); - location loc2 in the buffer will now contain the command "br i1 %cond, label @, label %my_false_label"
bpatch(makelist({loc2,FIRST}),"my_true_label"); - location loc2 in the buffer will now contain the command "br i1 %cond, label @my_true_label, label %my_false_label"
*/
void CodeBuffer::bpatch(const vector<LabelLocation> &address_list, const std::string &label)
{
    for (vector<LabelLocation>::const_iterator i = address_list.begin(); i != address_list.end(); i++)
    {
        int address = (*i).first;
        BranchLabelIndex labelIndex = (*i).second;
        replace(buffer[address], "@", "%" + label, labelIndex);
    }
}
/**
 * prints the content of the code buffer to stdout
 */
void CodeBuffer::printCodeBuffer()
{
    for (std::vector<string>::const_iterator it = buffer.begin(); it != buffer.end(); ++it)
    {
        cout << *it << endl;
    }
}
void CodeBuffer::returnFunc(string ret_type)
{
    if (buffer.back().find("return") == std::string::npos)
        emit("return"); /** @todo: default value for type*/
    emitRightBrace();
}

string CodeBuffer::typeCode(string c_type)
{
    if (c_type == "void")
        return "void";
    if (c_type == "string")
        return "i8*";
    if (c_type == "bool")
        return "i1";
    if (c_type == "byte")
        return "i8";
    return "i32";
}

string CodeBuffer::getDefaultValue(string c_type)
{
    if (c_type == "void")
        return "";
    if (c_type == "bool")
        return "false";
    return "0";
}

/**
 * gets a pair<int,BranchLabelIndex> item of the form
 * {buffer_location, branch_label_index} and creates a list for it
 */
vector<LabelLocation> CodeBuffer::makelist(LabelLocation item)
{
    vector<LabelLocation> newList;
    newList.push_back(item);
    return newList;
}
/**
 * merges two lists of {buffer_location, branch_label_index} items
 */
vector<LabelLocation> CodeBuffer::merge(const vector<LabelLocation> &l1, const vector<LabelLocation> &l2)
{
    vector<LabelLocation> newList(l1.begin(), l1.end());
    newList.insert(newList.end(), l2.begin(), l2.end());
    return newList;
}

LabelLocation CodeBuffer::emitJump()
{
    return LabelLocation(emit("br label @"), FIRST);
}

/**
 * Emit two lines needed for generating a label jump.
 * @param labelName the name of the label to emit
 */
void CodeBuffer::labelEmit(string &labelName)
{
    /** We might need to close the prev block here using the first emit.
     * @todo: Figure out
     */
    // emit("br label %" + labelName);
    emit(labelName + ":");
}

// ******** Methods to handle the global section ********** //
/**
 * write a line to the global section
 */
void CodeBuffer::emitGlobal(const std::string &dataLine)
{
    globalDefs.push_back(dataLine);
}
/**
 * print the content of the global buffer to stdout
 */
void CodeBuffer::printGlobalBuffer()
{
    for (vector<string>::const_iterator it = globalDefs.begin(); it != globalDefs.end(); ++it)
    {
        cout << *it << endl;
    }
}

// ******** Helper Methods ********** //
bool replace(string &str, const string &from, const string &to, const BranchLabelIndex index)
{
    size_t pos;
    if (index == SECOND)
    {
        pos = str.find_last_of(from);
    }
    else
    { // FIRST
        pos = str.find_first_of(from);
    }
    if (pos == string::npos)
        return false;
    str.replace(pos, from.length(), to);
    return true;
}

/** Methods for creating and getting addresses of varibales in the stack*/

/**
 * Alloc a new register to possess the vakue in the stack of the variable
 * located in the specific offset after the rbp address of the scope.
 * @param rbp the base address of the scope
 * @param offset the offset (positive) from this base address
 *
 * @return the name of the register with the varibale value inside of it
 */
string CodeBuffer::loadVaribale(string rbp, int offset)
{
    string reg = genReg();
    string varPtr = genReg();
    /* get the pointer to the correct address within the register varPtr*/
    emit(varPtr + " = getelementptr i32, i32* " + rbp + ", i32 " + std::to_string(offset));
    /* insert the value within the address to the new register*/
    emit(reg + " = load i32, i32* " + varPtr);
    return reg;
}
/**
 * Store a new variable in the offset address on the stack after rbp. The value
 * is identified with "reg" register name.
 * @param rbp the base address of the scope
 * @param offset the offset (positive) from this base address
 * @param reg the name of the register holding the value to store
 */
void CodeBuffer::storeVariable(string rbp, int offset, string reg)
{
    string varPtr = genReg();
    /* get the pointer to the correct address within the register varPtr*/
    emit(varPtr + " = getelementptr i32, i32* " + rbp + ", i32 " + std::to_string(offset));
    /* store in the memory*/
    emit("store i32 " + reg + ", i32* " + varPtr);
}
/**
 * Alloc a new register and emit a line stating this register holds the
 * address of the new base pointer to use (the rbp)
 * @return the name of the register holding the rbp value
 */
string CodeBuffer::allocFunctionRbp()
{
    string rbp = genReg();
    /* allocate memory for 50 variables on the stack, this is the new
    base ptr for the function that is declared (outside)*/
    emit(rbp + " = alloca i32, i32 50");
    return rbp;
}

/**************** Emit specific code methods *******************/
