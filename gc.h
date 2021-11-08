#include <stdint.h>

#include <string>
#include <unordered_set>
#include <vector>
#include <map>
#include <list>

// Called by the garbage collector after each collection to report the
// statistics about the heap after garbage collection.
void ReportGCStats(size_t liveObjects, size_t liveWords);

// Thrown by Alloc if the L2 program has run out of memory.
struct OutOfMemoryError : public std::runtime_error {
  OutOfMemoryError() : runtime_error("Out of memory.") {}
};

// Implements a semispace garbage collector for L2 programs.
class GcSemiSpace {
 public:
  // The 'frame_ptr' argument should be the frame pointer for the stack frame of
  // 'main', i.e., the stack frame immediately before the stack frame of 'Entry'
  // for the L2 program. The 'heap_size' argument is the number of desired words
  // in the heap; it should be a positive even number.
  GcSemiSpace(intptr_t* frame_ptr, int heap_size_in_words);

  // Allocates num_words+1 words on the heap and returns the address of the
  // second word. The first word (at a negative offset from the returned
  // address) is intended to be the 'header word', which should be filled in by
  // the L2 program with the correct type information.
  //
  // `curr_frame_ptr` is the frame pointer for the last frame in the
  // L2 program. It is needed for when the garbage collector is
  // walking the stack.
  //
  // Throws 'OutOfMemoryError' if the heap runs out of memory.
  intptr_t* Alloc(int32_t num_words, intptr_t * curr_frame_ptr);

 private:
  // Your private methods for functionality such as garbage
  // collection, stack walking, and copying live data should go here

  intptr_t* base_frame_ptr;

  int heap_size;
  intptr_t *heap_space, *from_space, *to_space;

  int from_size;
  int to_size;
  intptr_t* bump_ptr;

  std::map<intptr_t*, int> obj_map;
  std::map<intptr_t*, int> new_map;

  // memory locations (on stack) of a pointer (to heap)
  std::vector<intptr_t*> root_set;

  // Variables needed for Gc Stat Report
  size_t num_obj_copied, num_word_copied;

  // Walk the stack and fill the root set
  void stack_walk(intptr_t* curr_frame_ptr);
  // Helper function that read the info words
  void info_word_bit_mask(int info_word, intptr_t* curr_frame_ptr,
                          int word_offset);

  void copy_space_on_rootset();
  void copy_space_on_struct(intptr_t* obj_ptr);
  bool isCopied(intptr_t* obj_ptr);
  void add_forwarding_ptr(intptr_t* obj_ptr, intptr_t* forwarding_ptr);
};


// Implements a mark-sweep garbage collector for L2 programs.
class GcMarkSweep {
 public:
  // The 'frame_ptr' argument should be the frame pointer for the stack frame of
  // 'main', i.e., the stack frame immediately before the stack frame of 'Entry'
  // for the L2 program. The 'heap_size' argument is the number of desired words
  // in the heap; it should be a positive even number.
  GcMarkSweep(intptr_t* frame_ptr, int heap_size_in_words);

  // Allocates num_words+1 words on the heap and returns the address of the
  // second word. The first word (at a negative offset from the returned
  // address) is intended to be the 'header word', which should be filled in by
  // the L2 program with the correct type information.
  //
  // `curr_frame_ptr` is the frame pointer for the last frame in the
  // L2 program. It is needed for when the garbage collector is
  // walking the stack.
  //
  // Throws 'OutOfMemoryError' if the heap runs out of memory.
  intptr_t* Alloc(int32_t num_words, intptr_t * curr_frame_ptr);

 private:
  intptr_t* base_frame_ptr;
  int heap_size;
  intptr_t* heap_space;
  int free_size;
  // A free list for managing free memory
  std::list<std::pair<intptr_t*, int>> free_list;

  std::vector<intptr_t*> root_set;

  // Variables needed for Gc Stat Report
  size_t num_obj_copied, num_word_copied;

  // Helper function that find a free memory block larger or equal to
  // 'num_words' + 1. If 'num_words' + 1 in bytes is greater than the available
  // memory 'free_size', return an iterator pointing to the end of 'free_list'.
  // If no available memory block is greater or equal to 'num_words' + 1,
  // return an iterator pointing to the end of the 'free_list'.
  std::list<std::pair<intptr_t*, int>>::iterator find_free_block(int num_words);

  intptr_t* allocate_memory(std::list<std::pair<intptr_t*, int>>::
                              iterator block_iter,
                            int32_t num_words);

  // Helper function that walks the stack and fills the root set
  void stack_walk(intptr_t* curr_frame_ptr);

  // Helper function that reads the info words
  void info_word_bit_mask(int info_word, intptr_t* curr_frame_ptr,
                          int word_offset);

  // Helper function that coalesce free memory. Sweep the freelist to find
  // abutting free blocks, then merge the those blocks together into a single
  // block.
  void coalesce_free_list();
};