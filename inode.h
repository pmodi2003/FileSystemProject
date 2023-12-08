// Inode manipulation routines.
//
// Feel free to use as inspiration. Provided as-is.

// based on cs3650 starter code
#ifndef INODE_H
#define INODE_H


#include "helpers/blocks.h"

extern const int NUM_INODES; // number of inodes in FS (1 for each block -> 256 inodes)
extern const int INODES_START_BLOCK; // block where inodes start

// inode_t size: 20 bytes
typedef struct inode {
  int refs;  // reference count
  int mode;  // permission & type
  int size;  // bytes
  int block; // single block pointer (if max file size <= 4K)
  int files; // number of files in directory (if type = file, then 0)
} inode_t;

void print_inode(inode_t *node);
inode_t *get_inode(int inum);
int alloc_inode();
void free_inode(int inum);

#endif
