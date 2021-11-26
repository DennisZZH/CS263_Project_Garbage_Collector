# Project: Garbage Collectors for A Simple Progamming Language L2

In this Project, we are going to implement a semi-space garbage collector, a
mark-sweep garbage collector and a generational garbage collector for programs
written in a simple, self-developed programing language L2.

## L2 Language Concrete Syntax

See https://github.com/DennisZZH/CS263_Project_Garbage_Collector/blob/main/L2-concrete-syntax.pdf

## L2 Compiler Front End

The front end consists of a lexer that turn a L2 program into a sequence of tokens, and a 
recursive descent parser that further parse the sequence of tokens into an abstract syntax tree.

## L2 Compiler Back End

The back end consists of a code generator that takes an abstract syntax tree as input and 
generates a list of x86 assembly instructions.

## x86 Resources

Here are some resources on x86. Also there are some notes about x86
below.

*Important note:* We are using the AT&T/GAS syntax, which is the one
with markers for registers (like EAX is denoted with `%eax`) and
constants (e.g., 0 is `$0`).

- x86 Assembly Wikibook: https://en.wikibooks.org/wiki/X86_Assembly
- An x86 instruction reference, we do not need most of the
  instructions here but it is useful for checking out how the
  instructions work: https://c9x.me/x86/
  
## Semi Space Garbage Collector

This is a semi-space garbage collector for L2 in class `GcSemiSpace`. The
interfaces we implemented are the constructor and the `Alloc` method.

The constructor takes the frame pointer for the base of the stack and size of
the whole heap in terms of words. It should keep track of the given stack base
pointer, allocate the heap memory for the L2 program as an array of bytes, and
initialize the data you need to keep, such as heap size, semispace size, and
pointers to the `from` & `to` spaces.

The `Alloc` method allocates an object with the given number of words. It should
allocate `num_words+1` words and return a pointer to the second word in the
chunk. This is because the first word is going to be used as the tag for the
object, aka the header word. The header word will be set so that, bits 0--7 is
the number of fields, bits 8--30 is a bitvector indicating which fields are
pointers and bit 31 is always set to 1. Such interface is established between the
compiler and the garbage collector.

If there is not enough space for the new object
in from space, `Alloc` should run the garbage collector. If after the garbage
collection, there still is not enough space, `Alloc` should throw an
`OutOfMemoryError`.

The garbage collector will walk the stack to get the root set and it will copy
the reachable objects into `to` space, starting from the root set. Finally, it
will swap the pointers to the `from` and `to` spaces.

After a garbage collection is finished, the garbage collector should call the
`ReportGCStats` function to report the number and the total size in words of
live objects after garbage collection (the garbage collector will need to
compute this information each time it's run). We will use this information to
test and evaluate the garbage collector.

  
## Mark Sweep Garbage Collector
This is a Mark Sweep garbage collector for L2 in class `GcMarkSweep`. The
interfaces we implemented are the constructor and the `Alloc` method.

The constructor will take the base stake pointer and the heap size. 
It will allocate the heap memory for the L2 program as an array of bytes.
A free list will be used to manage the available memory.

The `Alloc` method alloacate an object with the given number of words. It
follows the same interface bewteen compiler and garbage collector. When there
is not enough space on allocated heap space for the new object, it will walk the stack and identify
the rootset. Start from the root set, trace all reachable objects in memory and
mark them as live. Then do a sweep through the entire allocated heap space and any memory that
is not marked gets freed and returned to the freelist.

After a garbage collection is finished, the garbage collector should call the
`ReportGCStats` function to report the number and the total size in words of
live objects after garbage collection. 

Note that using a freelist for memory managenent will cause external fregementation.
A function needs to be called periodically to coalesce free memory. To do so efficiently 
requires that each block of memory is marked as free or allocated, which requires more 
meta-data—otherwise we don’t know whether the abutting block (as opposed to the next block in the freelist)
is free or not. Also we cannot coalesce memory by moving allocated regions of memory around,
this would invalidate the addresses being used by the executing program.


## Generational Garbage Collector
TODO

## How to build the project

We use 32-bit GCC 8.4.0 toolchain (including GNU assembler) and the
Makefile is set up to be used with GCC 8. All of the necessary tools
are installed in CSIL and if you are using Ubuntu (or any other Debian
derivative), you can install the necessary tools by installing the
following packages: g++-8, g++-8-multilib, binutils.

Since the garbage collector is supposed to traverse the stack, the
bootstrap code calculates frame pointers. Our frame pointer
calculation code assumes x86 ABI under Linux so its behavior is not
guaranteed to be correct if you are using a different architecture or
operating system.

 - To compile the bootstrap code and the GC, simply run `make`.
 - To build a test program named `test.asm`, you need to first
   assemble it then link it against your GC.

   1. To assemble `test.asm` to `test.o`, you can run the following
      command:
      ```
      as --32 -g test.asm -o test.o
      ```
   2. To link the resulting object file with the bootstrap and the GC
      code, you can run the following command:
      ```
      g++ -m32
      build/bootstrap.o build/gc.o test2.l2.exe.o -o test2.l2.exe
      ```
      You can replace `g++` with the g++ executable installed on
      your system.

You can also give `make` argument `-jN` to run up to `N` processes
while building your code. This can reduce build times especially if
you just ran `make clean`.

The `Makefile` provided to you uses `g++` as the C++ compiler. We will
use `g++` to compile your code on GradeScope. The template we provided
works on CSIL as is so you do not need to change the compiler on CSIL.

## How to test the garbage collector

After assembling and linking a test program, you can run it as follows:

```
./my-test-program heap-size-in-words 2> my-GC-stats.txt
```

This command runs the program with a heap with given size in words
(for the whole space, not for a semispace) and saves standard error to
my-GC-stats.txt. You can then open this file to see how many
collections have happened and how many times.

For example, `test1.l2` has the following table for heap sizes and
collections:

```
// SIZE | COLLECTIONS
// -----+-------------
//   60 | 0 [], OK
//   36 | 1 [2 obj, 6 words], OK
//   30 | 3 [3 obj, 9 words]x3, OK
//   24 | 3 [2 obj, 6 words]x3, OK
//   18 | 1 [3 obj, 9 words], OOM
//   12 | 1 [2 obj, 6 words], OOM
```

This means that if we were to run it on a heap with 36 words, we would
see only 1 collection, after which 2 objects of total size of 6 words
would be still live and the program with exit without an error (`OK`
here denotes that the program will exit normally). On the other hand,
if we run it with a heap size of 18, we would observe 1 collection
after which 3 objects of total size of 9 words would be still alive,
then the program will exit with an out of memory error (`OOM` here
indicates out of memory).

So, if we run a compiled version of it with a
working garbage collector, we expect the following GC stats:

```
% ./test1.exe 12 2> my-GC-stats.txt
% cat my-GC-stats.txt
[2 objects, 6 words]
terminate called after throwing an instance of 'OutOfMemoryError'
  what():  Out of memory.
% ./test1.exe 18 2> my-GC-stats.txt
% cat my-GC-stats.txt
[3 objects, 9 words]
terminate called after throwing an instance of 'OutOfMemoryError'
  what():  Out of memory.
% ./test1.exe 24 2> my-GC-stats.txt
% cat my-GC-stats.txt
[2 objects, 6 words]
[2 objects, 6 words]
[2 objects, 6 words]
% ./test1.exe 30 2> my-GC-stats.txt
% cat my-GC-stats.txt
[3 objects, 9 words]
[3 objects, 9 words]
[3 objects, 9 words]
% ./test1.exe 36 2> my-GC-stats.txt
% cat my-GC-stats.txt
[2 objects, 6 words]
% ./test1.exe 60 2> my-GC-stats.txt
% cat my-GC-stats.txt
```

Here, the lines starting with `%` are shell command prompts.
