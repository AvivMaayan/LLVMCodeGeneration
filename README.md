1. In order to check code generation methods:
    ```shell
    make
    ./hw5 < our_tests/t01.in
    ```
    * I've added ```void CodeBuffer::testBuffer()``` to bp.cpp and ```buffer.testBuffer();``` to parse.ypp's main.
    * As we add to CodeBuffer we may use this to see the generated LLVM IR.

2. To do:
