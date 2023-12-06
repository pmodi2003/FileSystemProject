/**
 *@file storage.c
 *
 * Implementation of disk storage manipulation
 */

#include "storage.h"
#include "directory.h"
#include "inode.h"
#include "helpers/bitmap.h"
#include "helpers/blocks.h"
#include "helpers/slist.h"


//Initialize Files Structure
void storage_init(const char *path){
    blocks_init
}

int storage_stat(const char *path, struct stat *st);
int storage_read(const char *path, char *buf, size_t size, off_t offset);
int storage_write(const char *path, const char *buf, size_t size, off_t offset);
int storage_truncate(const char *path, off_t size);
int storage_mknod(const char *path, int mode);
int storage_unlink(const char *path);
int storage_link(const char *from, const char *to);
int storage_rename(const char *from, const char *to);
int storage_set_time(const char *path, const struct timespec ts[2]);
slist_t *storage_list(const char *path);