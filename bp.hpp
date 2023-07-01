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

typedef std::pair<int, BranchLabelIndex> LabelLocation;

class CodeBuffer
{
    CodeBuffer();
    CodeBuffer(CodeBuffer const &) = delete;
    void operator=(CodeBuffer const &);
    std::vector<std::string> buffer;
    std::vector<std::string> globalDefs;
    
    int regCounter;

    /**************** Emit specific code methods *******************/
    
    void emitPtintingFunctions();

    void emitDivisionFunction();

    void emitDeclareFunctions();

public:
    static CodeBuffer &instance();

    void emitGlobals();

    // ******** Methods to handle the code section ******** //

    std::string genLabel();

    std::string genReg(bool isGlobal = false);

    int emit(const std::string &command);

    void emitFile(const string &path);
    void emitLeftBrace() { emit("{"); };
    void emitRightBrace() { emit("}"); };
    void returnFunc(string ret_type);
    string typeCode(string ret_type);
    string getDefaultValue(string ret_type);
    string paddReg(string reg, string typeToPadd);
    /* convert the reg from 'fromType' to 'toType' and put it a new reg*/
    string convertTypes(string fromType, string toType, string reg);

    static vector<LabelLocation> makelist(LabelLocation item);

    static vector<LabelLocation> merge(const vector<LabelLocation> &l1, const vector<LabelLocation> &l2);

    void bpatch(const vector<LabelLocation> &address_list, const std::string &label);

    void printCodeBuffer();

    void labelEmit(string &labelName);

    LabelLocation emitJump();

    // ******** Methods to handle the data section ******** //
    void emitGlobal(const string &dataLine);
    
    void printGlobalBuffer();

    /** Methods for creating and getting addresses of varibales in the stack*/

    string loadVaribale(string rbp, int offset);

    void storeVariable(string rbp, int offset, string reg);
    
    string allocFunctionRbp();
};

#endif
