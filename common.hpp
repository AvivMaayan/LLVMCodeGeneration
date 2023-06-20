#ifndef EX5_COMMON
#define EX5_COMMON

#include <vector>

// this enum is used to distinguish between the two possible missing labels of a conditional branch in LLVM during backpatching.
// for an unconditional branch (which contains only a single label) use FIRST.
enum BranchLabelIndex
{
    FIRST,
    SECOND
};

typedef std::pair<int, BranchLabelIndex> LabelLocation;

#endif
