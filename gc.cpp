#include "gc.h"

#include <cassert>
#include <cstdio>

#include <iomanip>
#include <sstream>

using std::unordered_set;

GcSemiSpace::GcSemiSpace(intptr_t* frame_ptr, int heap_size_in_words) {
  // Initialize GC data structures and allocate space for the heap
  // here
}

intptr_t* GcSemiSpace::Alloc(int32_t num_words, intptr_t * curr_frame_ptr) {
  // This is a dummy return value to make the compiler happy. Return
  // the allocated address in your implementation.
  return nullptr;
}
