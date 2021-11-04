#include "gc.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <string.h>

using std::unordered_set;

GcSemiSpace::GcSemiSpace(intptr_t* frame_ptr, int heap_size_in_words) {
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

intptr_t* GcSemiSpace::Alloc(int32_t num_words, intptr_t * curr_frame_ptr) {
  intptr_t* obj_ptr;

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

void GcSemiSpace::stack_walk(intptr_t* curr_frame_ptr) {
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

void GcSemiSpace::info_word_bit_mask(int info_word, intptr_t* curr_frame_ptr, 
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
        memcpy(bump_ptr, from_obj_ptr - 1, 4*(num_words + 1));

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

bool GcSemiSpace::isCopied(intptr_t* obj_ptr) {
  // check the last bit of head word, if 1 not copied, if 0 is copied
  intptr_t* head_ptr = obj_ptr - 1;
  int head = *head_ptr;
  int last_bit = head & 0x0001;
  if (last_bit == 0) return true;
  else return false;
}

void GcSemiSpace::add_forwarding_ptr(intptr_t* obj_ptr, 
                                     intptr_t* forwarding_ptr) {
  // change the head word of the object in from space into a pointer to the
  // cpoied object in to space
  intptr_t* head_ptr = obj_ptr - 1;
  *head_ptr = (intptr_t) forwarding_ptr;
}

void GcSemiSpace::copy_space_on_struct(intptr_t* obj_ptr) {
  intptr_t* head_ptr = obj_ptr - 1;
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