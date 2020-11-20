### Simple Meoory Allocator
#### How To Test
1. `make sma`
2. `./sma.exe`
#### Testing Routine
1. Most of `a3_test.c` are from the original test file provided. 
2. I added a function `debug()` to print the freelist details and check if the output of `mallinfo()` is the same as the total size of the freelist. 
   As we can see from `output.png`, the program passed the test.