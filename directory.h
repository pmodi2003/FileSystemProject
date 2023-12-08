// Directory manipulation functions.
//
// Feel free to use as inspiration. Provided as-is.

// Based on cs3650 starter code
#ifndef DIRECTORY_H
#define DIRECTORY_H

#define DIR_NAME_LENGTH 48

#include "helpers/blocks.h"
#include "inode.h"
#include "helpers/slist.h"

// dirent_t size: 54 bytes
typedef struct dirent {
  char name[DIR_NAME_LENGTH]; // name of entry
  int inum; //inode number of entry
} dirent_t;

void directory_init();
int directory_lookup(inode_t *di, const char *name);
int path_lookup(const char *path);
int directory_put(inode_t *di, const char *name, int inum);
int directory_delete(inode_t *di, const char *name);
slist_t *directory_list(const char *path);
void print_directory(inode_t *di);

#endif
