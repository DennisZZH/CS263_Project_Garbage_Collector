/* Author: Zihao Zhang */
#include "gc.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <string.h>

using std::unordered_set;

GcSemiSpace::GcSemiSpace(intptr_t *frame_ptr, int heap_size_in_words) {
  // Initialize GC data structures and allocate space for the heap here
  base_frame_ptr = frame_ptr;
  heap_size = heap_size_in_words;
  heap_space = (intptr_t*) malloc(heap_size * 4);
  from_space = heap_space;
  to_space = heap_space + heap_size / 2;
  from_size = heap_size / 2;
  to_size = heap_size / 2;
  bump_ptr = from_space;
}

intptr_t* GcSemiSpace::Alloc(int32_t num_words, intptr_t *curr_frame_ptr) {
  intptr_t *obj_ptr;

  if (num_words + 1 <= from_size) {
    obj_ptr = bump_ptr + 1;
    bump_ptr = bump_ptr + num_words + 1;
    from_size = from_size - num_words - 1;
    obj_map.insert(std::pair<intptr_t*, int>(obj_ptr, num_words));

  } else {
    bump_ptr = to_space;
    stack_walk(curr_frame_ptr);
    copy_space_on_rootset();
    ReportGCStats(num_obj_copied, num_word_copied);
    num_obj_copied = 0;
    num_word_copied = 0;

    if (num_words + 1 <= from_size) {
      obj_ptr = bump_ptr + 1;
      bump_ptr = bump_ptr + num_words + 1;
      from_size = from_size - num_words -1;
      obj_map.insert(std::pair<intptr_t*, int>(obj_ptr, num_words));
    } else {
      throw OutOfMemoryError();
    }
  }

  return obj_ptr;
}

void GcSemiSpace::stack_walk(intptr_t *curr_frame_ptr) {
  root_set.clear();
  intptr_t *aiw_ptr, *liw_ptr;

  while (curr_frame_ptr != base_frame_ptr) {
    aiw_ptr = curr_frame_ptr - 1;
    info_word_bit_mask(*aiw_ptr, curr_frame_ptr, 2);

    liw_ptr = curr_frame_ptr - 2;
    info_word_bit_mask(*liw_ptr, curr_frame_ptr, -3);

    curr_frame_ptr = (intptr_t*) *curr_frame_ptr;
  }
}

void GcSemiSpace::info_word_bit_mask(int info_word, intptr_t *curr_frame_ptr, 
                                                    int word_offset) {
  int is_ptr, bit_num = 0;
  while (info_word != 0) {
    // mask out the right most bit of info word
    is_ptr = info_word & 0x0001;
  
    if (is_ptr == 1)  {
      if (word_offset > 0) {
        // if it is a argument info word
        root_set.push_back(curr_frame_ptr + word_offset + bit_num);
      } else {
        // if it is a local info word
        root_set.push_back(curr_frame_ptr + word_offset - bit_num);
      }
    }

    bit_num++;
    info_word >>= 1;
  }
}

void GcSemiSpace::copy_space_on_rootset() {
  intptr_t *from_obj_ptr, *root_ptr, *tmp_space, *to_obj_ptr;
  int num_words;

  for (unsigned int i = 0; i < root_set.size(); i++) {
    root_ptr = root_set[i];
    from_obj_ptr = (intptr_t*) *root_ptr;

    if (from_obj_ptr == NULL) continue;

    auto j = obj_map.find(from_obj_ptr);

    if (j != obj_map.end()) {
      num_words = j->second;

      if (isCopied(from_obj_ptr)) {
        // update stack root pointer to forwarding pointer
        to_obj_ptr = (intptr_t*) *(from_obj_ptr - 1);
        *root_ptr = (intptr_t) to_obj_ptr;

      } else {
        memcpy(bump_ptr, from_obj_ptr - 1, 4 * (num_words + 1));

        num_obj_copied++;
        num_word_copied = num_word_copied + num_words + 1;

        to_obj_ptr = bump_ptr + 1;
        new_map.insert(std::pair<intptr_t*, int>(to_obj_ptr, num_words));
        bump_ptr = bump_ptr + num_words + 1;
        to_size = to_size - num_words - 1;
       
        *root_ptr = (intptr_t) to_obj_ptr;

        add_forwarding_ptr(from_obj_ptr, to_obj_ptr);
      
        // if struct fields are pointers, copy them as well
        copy_space_on_struct(to_obj_ptr);
      }

    } else {
      std::cerr << "Error: rootset: Can not find such pointer on heap!"
                << std::endl;
      exit(0);
    }
  }

  // swap from and to
  obj_map = new_map;
  new_map.clear();

  from_size = to_size;
  to_size = heap_size / 2;

  tmp_space = from_space;
  from_space = to_space;
  to_space = tmp_space;
}

bool GcSemiSpace::isCopied(intptr_t *obj_ptr) {
  // check the last bit of head word, if 1 not copied, if 0 is copied
  intptr_t *head_ptr = obj_ptr - 1;
  int head = *head_ptr;
  int last_bit = head & 0x0001;
  if (last_bit == 0) return true;
  else return false;
}

void GcSemiSpace::add_forwarding_ptr(intptr_t *obj_ptr, 
                                     intptr_t *forwarding_ptr) {
  // change the head word of the object in from space into a pointer to the
  // cpoied object in to space
  intptr_t* head_ptr = obj_ptr - 1;
  *head_ptr = (intptr_t) forwarding_ptr;
}

void GcSemiSpace::copy_space_on_struct(intptr_t *obj_ptr) {
  intptr_t *head_ptr = obj_ptr - 1;
  int head = *head_ptr;
  int num_fields = head >> 24;
  int bitvector = (head << 8) >> 9;
  int last_bit;
  intptr_t *from_field_ptr, *to_field_ptr;
  int num_words;

  for (int i = 0; i < num_fields; i++) {
    last_bit = bitvector & 0x0001;

    if (last_bit == 1) {
      // that field is a pointer, copy that field
      from_field_ptr = (intptr_t*) *(obj_ptr + i);
      if (from_field_ptr == 0) {
        bitvector >>= 1;
        continue;
      }
      
      auto it = obj_map.find(from_field_ptr);
      if (it != obj_map.end()) {
        num_words = it->second;
      } else {
        std::cerr << "Error: struct: Can not find such pointer on heap!"
                  << std::endl;
        exit(0);
      }

      if (isCopied(from_field_ptr)) {
        // update field pointer
        head_ptr = from_field_ptr - 1;
        *(obj_ptr + i) = *head_ptr;
      } else {
        // copy
        memcpy(bump_ptr, from_field_ptr - 1, 4*(num_words + 1));

        num_obj_copied++;
        num_word_copied = num_word_copied + num_words + 1;

        to_field_ptr = bump_ptr + 1;
        *(obj_ptr + i) = (intptr_t) to_field_ptr;
         
        add_forwarding_ptr(from_field_ptr, to_field_ptr);

        new_map.insert(std::pair<intptr_t*, int>(to_field_ptr, num_words));
        bump_ptr = bump_ptr + num_words + 1;
        to_size = to_size - num_words - 1;

        // recursively check its fields
        copy_space_on_struct(to_field_ptr);
      }
    }
    bitvector >>= 1;
  }
}

/*----------------------------------------------------------------------------*/

GcMarkSweep::GcMarkSweep(intptr_t *frame_ptr, int heap_size_in_words) {
  // Initialize GC data structures and allocate space for the heap here
  base_frame_ptr = frame_ptr;
  heap_size = heap_size_in_words;
  heap_space = (intptr_t*) malloc(heap_size * 4);
  free_size = heap_size;
  free_list.push_back(std::make_pair(heap_space, free_size));
  free_map.insert(std::make_pair(heap_space, free_list.begin()));
}

intptr_t* GcMarkSweep::Alloc(int32_t num_words, intptr_t *curr_frame_ptr) {
  intptr_t *obj_ptr;
  // Try to find a memory block large enough for 'num_words'
  auto block_iter = find_free_block(num_words);

  if (block_iter != free_list.end()) {
    // Allocate memory for the object
    obj_ptr = allocate_memory(block_iter, num_words);
  } else {
    // Prepare the root set by walking the stack
    stack_walk(curr_frame_ptr);
    // TODO: Recursively track all sturct fields, if pointer add to root set.

    /*** Mark and Sweep ***/
    // Turn the root set into a std unordered_set
    std::cout << "size of root set = " << root_set.size() << std::endl;
    unordered_set<intptr_t*> root_hashset;
    for (unsigned int i = 0; i < root_set.size(); i++) {
      intptr_t *root_ptr = root_set[i];
      std::cout << "root set # " << root_ptr << std::endl;
      intptr_t *obj_ptr = (intptr_t*) *root_ptr;
      std::cout << "root hash set # " << obj_ptr << std::endl;
      root_hashset.insert(obj_ptr);
    }
   std::cout << "size of root hash set = " << root_hashset.size() << std::endl;
    // For every block in obj_list, check it exists in the root set. If not,
    // delete the block from obj_list and add its space back to free_list.
    for (auto iter = obj_list.begin(); iter != obj_list.end();) {
      std::cout << 4444444 << std::endl;
      std::cout << "obj list # " << iter->first << std::endl;
      if (root_hashset.find(iter->first) == root_hashset.end()) {
        std::cout << 444 << std::endl;
        free_list.push_front(std::make_pair(iter->first - 1, iter->second + 1));
        free_map.insert(std::make_pair(iter->first - 1, free_list.begin()));
        free_size += iter->second + 1;
        auto tmp = iter++;
        obj_list.erase(tmp);
      } else {
        iter++;
      }
    }
 
    // Report Gc status
    for (auto iter = obj_list.begin(); iter != obj_list.end(); iter++) {
      num_obj_left++;
      num_word_left += iter->second + 1;
    }
    ReportGCStats(num_obj_left, num_word_left);
    num_obj_left = 0;
    num_word_left = 0;

    // No enough space after Gc. Throw 'OutOfMemoryError' because memory ran out
    if (free_size < num_words) throw OutOfMemoryError();

    // Try to find a memory block large enough again after garbage collection
    block_iter = find_free_block(num_words);

    if (block_iter != free_list.end()) {;
      // Allocate memory for the object
      obj_ptr = allocate_memory(block_iter, num_words);
    } else {
      // Coalesce free memory
      coalesce_free_list();
      // Try find available space again after coalsecing
      block_iter = find_free_block(num_words);

      if (block_iter != free_list.end()) {
        // Allocate memory for the object
        obj_ptr = allocate_memory(block_iter, num_words);
      } else {
        // No enough space after coalesce.
        // Throw 'OutOfMemoryError' due to external fregamentation
        throw OutOfMemoryError();
      }
    }
  }

  return obj_ptr;
}

std::list<std::pair<intptr_t*, int>>::iterator 
  GcMarkSweep::find_free_block(int num_words) {
  // First fit algorithm
  int target_size = num_words + 1;
  for (auto iter = free_list.begin(); iter != free_list.end(); iter++) {
    if (iter->second >= target_size) return iter;
  }
  return free_list.end();
}

intptr_t* GcMarkSweep::allocate_memory(std::list<std::pair<intptr_t*, int>>
                                         ::iterator block_iter,
                                       int32_t num_words) {
  // Allocate memory
  intptr_t *obj_ptr = block_iter->first + 1;

  int leftover_size = block_iter->second - (num_words + 1);
  intptr_t *leftover_ptr = block_iter->first + num_words + 1;
  // Erase the used block
  free_list.erase(block_iter);
  free_map.erase(block_iter->first);
  // Put back the remaining free block into the free_list.
  if (leftover_size != 0) {
    free_list.push_front(std::make_pair(leftover_ptr, leftover_size));
    free_map.insert(std::make_pair(leftover_ptr, free_list.begin()));
  }
  // Decrease free size
  free_size -= num_words + 1;
  // Put allocated block into obj_list
  obj_list.push_front(std::make_pair(obj_ptr, num_words));

  return obj_ptr;
}

void GcMarkSweep::stack_walk(intptr_t *curr_frame_ptr) {
  root_set.clear();
  intptr_t *aiw_ptr, *liw_ptr;

  while (curr_frame_ptr != base_frame_ptr) {
    aiw_ptr = curr_frame_ptr - 1;
    info_word_bit_mask(*aiw_ptr, curr_frame_ptr, 2);

    liw_ptr = curr_frame_ptr - 2;
    info_word_bit_mask(*liw_ptr, curr_frame_ptr, -3);

    curr_frame_ptr = (intptr_t*) *curr_frame_ptr;
  }
}

void GcMarkSweep::info_word_bit_mask(int info_word, intptr_t *curr_frame_ptr,
                        int word_offset) {
  int is_ptr, bit_num = 0;
  while (info_word != 0) {
    // mask out the right most bit of info word
    is_ptr = info_word & 0x0001;
  
    if (is_ptr == 1)  {
      if (word_offset > 0) {
        // if it is a argument info word
        root_set.push_back(curr_frame_ptr + word_offset + bit_num);
      } else {
        // if it is a local info word
        root_set.push_back(curr_frame_ptr + word_offset - bit_num);
      }
    }

    bit_num++;
    info_word >>= 1;
  }
}

void GcMarkSweep::coalesce_free_list() {
  // For every block in the free_list, check if its next abutting block is free.
  // If so, merge this block with its next abutting block. Else, check next
  // block in the free_list.
  for (auto iter = free_list.begin(); iter != free_list.end();) {
    intptr_t* abutting_block_addr = iter->first + iter->second;

    if (free_map.find(abutting_block_addr) != free_map.end()) {
      auto abutting_block_iter = free_map[abutting_block_addr];
      iter->second += abutting_block_iter->second;
      free_map.erase(abutting_block_addr);
      free_list.erase(abutting_block_iter);
    } else {
      iter++;
    }
  } 
}
