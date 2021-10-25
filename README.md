# CS 160 Assignment 4: Garbage Collector for L2

**Assignment due: Friday, May 22 11:59 PM**

Read [The course description post on
Slack](https://cs160spring20.slack.com/archives/C010B82M9KL/p1585378023017500)
for late submission and slip day policy.

In this assignment, you are going to implement a semi-space garbage
collector for L2 programs.

## What is provided

We provide you with some pre-compiled assembly programs along with L2
source code so that you can test your garbage collector.

The comments in the beginning of each L2 program indicates how many
collections should happen and how many words and objects should be in
the live space.

After the slip day period has passed, we will provide our compiler
implementation for L2 so that you can test your garbage collector with
arbitrary L2 programs.

We also provide you with the bootstrap code that runs the given L2
program and provides a hook for your memory allocator and a hook for you
to call to collect GC statistics.

## What you need to implement

You need to implement the garbage collector class `GcSemiSpace`. The
interfaces you need to implement are the constructor and the `Alloc`
method.

The constructor takes the frame pointer for the base of the stack and size of
the whole heap in terms of words. It should keep track of the given stack base
pointer, allocate the heap memory for the L2 program as an array of bytes, and
initialize the data you need to keep, such as heap size, semispace size, and
pointers to the `from` & `to` spaces.

The `Alloc` method allocates an object with the given number of words. It should
allocate `num_words+1` words and return a pointer to the second word in the
chunk. This is because the first word is going to be used as the tag for the
object. See the lecture notes for the interface between the compiler and the
garbage collector. If there is not enough space for the new object in from
space, `Alloc` should run the garbage collector. If after the garbage
collection, there still is not enough space, `Alloc` should throw an
`OutOfMemoryError`.

The garbage collector will walk the stack to get the root set and it will copy
the reachable objects into `to` space, starting from the root set. Finally, it
will swap the pointers to the `from` and `to` spaces. You will need to create
helper functions for these functions. Please see the lectures for the object
representation, walking the stack, and other implementation details.

After a garbage collection is finished, the garbage collector should call the
`ReportGCStats` function to report the number and the total size in words of
live objects after garbage collection (the garbage collector will need to
compute this information each time it's run). We will use this information to
test and grade your garbage collector.

## Invariants you may want to check

Garbage collectors are finicky and debugging one without much information can
get complicated. We suggest using a debugger to investigate error states and
using defensive programming techniques to check invariants about your garbage
collector and memory model through assertions, and printing statistics whenever
you feel like they would help. You can create assertions using the `assert`
command, such as:

```
assert(some_predicate_that_should_be_true &&
    "some error message to print if the assert fails.");
```

This assert will evaluate the given predicate; if it is true then execution
proceeds as normal, otherwise execution halts and the error message is printed.
Here are some invariants that may be useful to check:

### Constructor

Preconditions:

- Base frame pointer should not be a `nullptr`.
- Heap size should be positive and even.
- `sizeof(intptr_t) = 4` (i.e. words are 4-byte long). Otherwise, it
  means you are not compiling your code for a 32-bit machine.

### Alloc

Preconditions:

- `num_words > 0`
- Your bump pointer should point to somewhere in the from space
  (i.e. the interval `[start of from space, start of from space +
  semispace size)`).

### Root set traversal (stack walking)

- At each step of walking the stack (when reading a new frame
  pointer), current frame pointer should point to somewhere between
  the base frame pointer and the stack pointer (i.e. it should be in
  the interval `[stack_ptr, base_frame_ptr]`, this interval is
  backwards because the stack grows downward.

## Copying phase

- Each heap pointer passed to the copy function should point to
  somewhere in the from space (i.e. the interval `[start of from
  space, start of from space + semispace size)`).

## How to build the assignment

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

For an explanation of the commands above, you can look at the
README.md file in assignment 3 template.

## How to test your garbage collector

After implementing your garbage collector, make sure that you are
calling `ReportGCStats` function. We will use this function for
testing and grading purposes. The version of `ReportGCStats` function
we provide you uses the standard error but the GradeScope version will
use some other mechanism for output so you can print extra debug
information to standard output or standard error.

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

### Testing via GradeScope

Later this week, we will enable submission via GradeScope and your
assignments will be tested against a more comprehensive test
suite. Auto-grader on GradeScope will give you feedback on your
assignment as well. You will be able to submit your assignment
multiple times and get feedback before submitting a final solution.

We will explain the details on the auto-grader once it is online.

## x86 Resources

Here are some resources on x86. Also there are some notes about x86
below.

*Important note:* We are using the AT&T/GAS syntax, which is the one
with markers for registers (like EAX is denoted with `%eax`) and
constants (e.g., 0 is `$0`).

- x86 Assembly Wikibook: https://en.wikibooks.org/wiki/X86_Assembly
- An x86 instruction reference, you will not need most of the
  instructions here but it is useful for checking out how the
  instructions you want to use work: https://c9x.me/x86/

## What to submit

You are going to submit your whole assignment through GradeScope. We
are going to consider only your changes to `gc.cpp` and `gc.h` while
grading and ignore changes in other files and additional files. We are
going to clarify submission instructions once GradeScope is online.

Your submitted `gc.h` and `gc.cpp` need to compile with the bootstrap
code given to you, otherwise you automatically fail the assignment.
