1. In order to check code generation methods:
    ```shell
    make
    ./hw5 < our_tests/t01.in
    ```
    * I've added ```void CodeBuffer::testBuffer()``` to bp.cpp and ```buffer.testBuffer();``` to parse.ypp's main.
    * As we add to CodeBuffer we may use this to see the generated LLVM IR.

2. To do:
    1. (Aviv) Update C'tors of Expressions to use markers and update lists.
    2. (Aviv) Finish RelOp and BoolOp.
    3. (Nitai) Finish SymbolTable::getSymbolOffset and SymbolTable::getCurrentRbp ASAP.
