// Directory manipulation functions.
//
// Feel free to use as inspiration.

// based on cs3650 starter code

#ifndef DIRECTORY_H
#define DIRECTORY_H

#define DIR_NAME_LENGTH 48

#include <stdbool.h>

#include "blocks.h"
#include "inode.h"
#include "slist.h"

// dirent_t size: 52 bytes
typedef struct dirent {
  char name[DIR_NAME_LENGTH]; // name of entry
  int inum;                   // inum for entry
} dirent_t;

void directory_init();
int directory_lookup(inode_t *dd, const char *name);
int path_lookup(const char *path);
int directory_put(inode_t *dd, const char *name, int inum);
int directory_delete(inode_t *dd, const char *name);
slist_t *directory_list(const char *path);

#endif
