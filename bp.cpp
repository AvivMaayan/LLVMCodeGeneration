#include "bp.hpp"
#include <vector>
#include <iostream>
#include <sstream>
using namespace std;

bool replace(string &str, const string &from, const string &to, const BranchLabelIndex index);

CodeBuffer::CodeBuffer() : buffer(), globalDefs(), regCounter(0) {}

CodeBuffer &CodeBuffer::instance()
{
    static CodeBuffer inst; // Only instance
    return inst;
}

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

string CodeBuffer::genReg(bool isGlobal)
{
    string regPrefix = (isGlobal) ? "@" : "%";
    return regPrefix + "var_" + std::to_string(regCounter++);
    ;
}

int CodeBuffer::emit(const string &s)
{
    buffer.push_back(s);
    return buffer.size() - 1;
}

void CodeBuffer::bpatch(const vector<LabelLocation> &address_list, const std::string &label)
{
    for (vector<LabelLocation>::const_iterator i = address_list.begin(); i != address_list.end(); i++)
    {
        int address = (*i).first;
        BranchLabelIndex labelIndex = (*i).second;
        replace(buffer[address], "@", "%" + label, labelIndex);
    }
}

void CodeBuffer::printCodeBuffer()
{
    for (std::vector<string>::const_iterator it = buffer.begin(); it != buffer.end(); ++it)
    {
        cout << *it << endl;
    }
}

vector<LabelLocation> CodeBuffer::makelist(LabelLocation item)
{
    vector<LabelLocation> newList;
    newList.push_back(item);
    return newList;
}

vector<LabelLocation> CodeBuffer::merge(const vector<LabelLocation> &l1, const vector<LabelLocation> &l2)
{
    vector<LabelLocation> newList(l1.begin(), l1.end());
    newList.insert(newList.end(), l2.begin(), l2.end());
    return newList;
}

// ******** Methods to handle the global section ********** //
void CodeBuffer::emitGlobal(const std::string &dataLine)
{
    globalDefs.push_back(dataLine);
}

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

void CodeBuffer::testBuffer()
{
    cout << "\n\n**** Buffer Testing ****\n\n";
    // for(int i=0; i<5; ++i)
    // {
    //     string fresh_reg = genReg();
    //     emit(fresh_reg);
    // }
    printCodeBuffer();
}
