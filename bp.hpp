#ifndef EX5_CODE_GEN
#define EX5_CODE_GEN

#include <vector>
#include <string>
#include "common.hpp"
#include "source.hpp"

using namespace std;



class CodeBuffer
{
    CodeBuffer();
    CodeBuffer(CodeBuffer const &) = delete;
    void operator=(CodeBuffer const &);
    std::vector<std::string> buffer;
    std::vector<std::string> globalDefs;
    
    int regCounter;

    /**************** Emit specific code methods *******************/
    

public:
    static CodeBuffer &instance();

    // ******** Methods to handle the code section ******** //

    std::string genLabel();

    std::string genReg(bool isGlobal = false);

    int emit(const std::string &command);

    void emitLeftBrace() { emit("{"); };
    void emitRightBrace() { emit("}"); };
    void returnFunc(string ret_type);
    string typeCode(string ret_type);
    string getDefaultValue(string ret_type);

    static vector<LabelLocation> makelist(LabelLocation item);

    static vector<LabelLocation> merge(const vector<LabelLocation> &l1, const vector<LabelLocation> &l2);

    void bpatch(const vector<LabelLocation> &address_list, const std::string &label);

    void printCodeBuffer();

    void labelEmit(string &labelName);

    LabelLocation emitJump();

    // ******** Methods to handle the data section ******** //
    void emitGlobal(const string &dataLine);
    
    void printGlobalBuffer();

    void testBuffer();

    /** Methods for creating and getting addresses of varibales in the stack*/

    string loadVaribale(string rbp, int offset);

    void storeVariable(string rbp, int offset, string reg);
    
    string allocFunctionRbp();
};

#endif
