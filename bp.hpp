#ifndef EX5_CODE_GEN
#define EX5_CODE_GEN

#include <vector>
#include <string>

using namespace std;

// this enum is used to distinguish between the two possible missing labels of a conditional branch in LLVM during backpatching.
// for an unconditional branch (which contains only a single label) use FIRST.
enum BranchLabelIndex
{
    FIRST,
    SECOND
};

class CodeBuffer
{
    CodeBuffer();
    CodeBuffer(CodeBuffer const &) = delete;
    void operator=(CodeBuffer const &);
    std::vector<std::string> buffer;
    std::vector<std::string> globalDefs;

    int regCounter;

public:
    // With the Singleton design pattern, the single instance of CodeBuffer could be accessed using:
    // CodeBuffer &buffer = CodeBuffer::instance();
    static CodeBuffer &instance();

    // ******** Methods to handle the code section ******** //

    // generates a jump location label for the next command, writes it to the buffer and returns it
    std::string genLabel();

    // helper function that returns (the name of) a fresh variable
    std::string genReg();

    // writes command to the buffer, returns its location in the buffer
    int emit(const std::string &command);

    // gets a pair<int,BranchLabelIndex> item of the form {buffer_location, branch_label_index} and creates a list for it
    static vector<pair<int, BranchLabelIndex>> makelist(pair<int, BranchLabelIndex> item);

    // merges two lists of {buffer_location, branch_label_index} items
    static vector<pair<int, BranchLabelIndex>> merge(const vector<pair<int, BranchLabelIndex>> &l1, const vector<pair<int, BranchLabelIndex>> &l2);

    /* accepts a list of {buffer_location, branch_label_index} items and a label.
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
    void bpatch(const vector<pair<int, BranchLabelIndex>> &address_list, const std::string &label);

    // prints the content of the code buffer to stdout
    void printCodeBuffer();

    // ******** Methods to handle the data section ******** //
    // write a line to the global section
    void emitGlobal(const string &dataLine);
    // print the content of the global buffer to stdout
    void printGlobalBuffer();

    // To test our code generation
    void testBuffer();

    /** Methods for creating and getting addresses of varibales in the stack*/
    
    /**
     * Alloc a new register to possess the address on the stack of the variable
     * located in the specific offset after the rbp address of the scope. 
     * @param rbp the base address of the scope
     * @param offset the offset (positive) from this base address
     *
     * @return the name of the register with the address inside of it 
    */
    string loadVaribale(string rbp, int offset);

    /**
     * Store a new variable in the offset address on the stack after rbp, identified
     * with "reg" name. 
     * @param rbp the base address of the scope  
     * @param offset the offset (positive) from this base address
     * @param reg the name of the register to use for storing the value
    */
    void storeVariable(string rbp, int offset, string reg);
    
    /**
     * Alloc a new register and emit a line stating this register holds the
     * address of the new base pointer to use (the rbp)
     * @return the name of the register holding the rbp value
    */
    string allocFunctionRbp();
};

#endif
