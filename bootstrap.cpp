#include "gc.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>

// The runtime memory manager.
GcSemiSpace *gc;

// 'Entry' is the entry point of an L2 program.
extern "C" {
int32_t Entry(void);
}

// Define the 'allocate' function without name mangling so that it can be called
// from L2 code.
extern "C" intptr_t *allocate(int32_t num_words) {
  // The current frame pointer is for allocate(), which is called from
  // the L2 program so we dereference the frame pointer once to get
  // the L2 program's frame pointer.
  intptr_t* curr_frame_ptr = *(intptr_t**)__builtin_frame_address(0);
  return gc->Alloc(num_words, curr_frame_ptr);
}

// Called by the garbage collector after each collection to report the
// statistics about the heap after garbage collection.
void ReportGCStats(size_t liveObjects, size_t liveWords) {
  std::cerr << "[" << liveObjects << " objects, " << liveWords << " words]\n";
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr <<
        "Must pass the total size of the heap in terms of the number of 32-bit "
        "words (must be a positive even number).";
    exit(1);
  }

  // Initialize the garbage collector.
  gc = new GcSemiSpace(/*frame_ptr=*/(intptr_t *)__builtin_frame_address(0),
                       /*heap_sizein_words=*/atoi(argv[1]));

  // Run the L2 program.
  std::cout << Entry() << "\n";
  // printf("%d\n", Entry());

  delete gc;
  return 0;
}
