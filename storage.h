// Disk storage abstracttion.
//
// Feel free to use as inspiration. Provided as-is.

// based on cs3650 starter code

#ifndef NUFS_STORAGE_H
#define NUFS_STORAGE_H

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "helpers/slist.h"

void storage_init(const char *path);
int storage_access(const char *path);
int storage_stat(const char *path, struct stat *st);
int storage_read(const char *path, char *buf, size_t size, off_t offset);
int storage_write(const char *path, const char *buf, size_t size, off_t offset);
int storage_truncate(const char *path, off_t size);
int storage_mknod(const char *path, int mode);
int storage_chmod(const char *path, int mode);
int storage_rmdir(const char *path);
int storage_unlink(const char *path);
int storage_link(const char *from, const char *to);
int storage_rename(const char *from, const char *to);
slist_t *storage_list(const char *path);

#endif
