// Inode manipulation routines.
//
// Feel free to use as inspiration.

// based on cs3650 starter code

#ifndef INODE_H
#define INODE_H

#include "blocks.h"

extern const int NUM_INODES; // number of inodes in file system (default = 256)
extern const int INODE_BLOCK; // block where inodes begin

// inode_t size: 20 bytes
typedef struct inode {
  int refs;             // reference count
  int mode;             // permission & type
  int size;             // sie of inode
  int block;            // single block pointer/number (max file size <= 4K)
  int nodes;          // number of nodes in this inode (files have 0 nodes)
} inode_t;

void print_inode(inode_t *node);
inode_t *get_inode(int inum);
int alloc_inode();
void free_inode(int inum);

#endif
