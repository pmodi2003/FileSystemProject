// based on cs3650 starter code

#include <assert.h>
#include <bsd/string.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

#include "storage.h"

// implementation for: man 2 access
// Checks if a file exists.
int nufs_access(const char *path, int mask) {
  int rv = storage_access(path);
  printf("access(%s, %04o) -> %d\n", path, mask, rv);
  return rv;
}

// Gets an object's attributes (type, permissions, size, etc).
// Implementation for: man 2 stat
// This is a crucial function.
int nufs_getattr(const char *path, struct stat *st) {
  int rv = storage_stat(path, st);
  st->st_uid = getuid();
  printf("getattr(%s) -> (%d) {mode: %04o, size: %ld}\n", path, rv, st->st_mode,
          st->st_size);
  return rv;
}

// implementation for: man 2 readdir
// lists the contents of a directory
int nufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                 off_t offset, struct fuse_file_info *fi) {
  struct stat st;
  int rv;
  char cur_dir[strlen(path) + 48];

  slist_t *dir = storage_list(path);
  slist_t *iter_dir = dir;
  while (iter_dir != NULL){
    if(strcmp(path, "/") == 0){
      cur_dir[0] = '/';
      
      int cur_dir_length = strlen(iter_dir->data);
      strncpy(cur_dir + 1, iter_dir->data, 48);

      cur_dir[cur_dir_length + 1] = '\0';

      rv = nufs_getattr(cur_dir, &st);
      assert(rv == 0);
      if(filler(buf, iter_dir->data, &st, 0) != 0){
        break;
      }
    } else {
      strncpy(cur_dir, path, strlen(path));
      cur_dir[strlen(path)] = '/';

      int cur_dir_length = strlen(iter_dir->data);
      strncpy(cur_dir + strlen(path) + 1, iter_dir->data, 48);
      if (cur_dir_length > 48) {
        cur_dir_length = 48;
      }
      cur_dir[cur_dir_length + strlen(path) + 1] = '\0';

      rv = nufs_getattr(cur_dir, &st);
      assert(rv == 0);
      if(filler(buf, iter_dir->data, &st, 0) != 0){
        break;
      }    
    }
    iter_dir = iter_dir->next;
  }

  slist_free(dir);

  printf("readdir(%s) -> %d\n", path, rv);
  return 0;
}

// mknod makes a filesystem object like a file or directory
// called for: man 2 open, man 2 link
// Note, for this assignment, you can alternatively implement the create
// function.
int nufs_mknod(const char *path, mode_t mode, dev_t rdev) {
  int rv = storage_mknod(path, mode, 0);
  printf("mknod(%s, %04o) -> %d\n", path, mode, rv);
  return rv;
}

// most of the following callbacks implement
// another system call; see section 2 of the manual
int nufs_mkdir(const char *path, mode_t mode) {
  int rv = storage_mknod(path, mode | 040000, 1);
  printf("mkdir(%s, %04o) -> %d\n", path, mode, rv);
  return rv;
}

int nufs_unlink(const char *path) {
  int rv = storage_unlink(path);
  printf("unlink(%s) -> %d\n", path, rv);
  return rv;
}

int nufs_link(const char *from, const char *to) {
  int rv = storage_link(from, to);
  printf("link(%s => %s) -> %d\n", from, to, rv);
  return rv;
}

int nufs_rmdir(const char *path) {
  int rv = storage_rmdir(path);
  printf("rmdir(%s) -> %d\n", path, rv);
  return rv;
}

// implements: man 2 rename
// called to move a file within the same filesystem
int nufs_rename(const char *from, const char *to) {
  int rv = storage_rename(from, to);
  printf("rename(%s => %s) -> %d\n", from, to, rv);
  return rv;
}

int nufs_chmod(const char *path, mode_t mode) {
  int rv = storage_chmod(path, mode);
  printf("chmod(%s, %04o) -> %d\n", path, mode, rv);
  return rv;
}

int nufs_truncate(const char *path, off_t size) {
  int rv = storage_truncate(path, size);
  printf("truncate(%s, %ld bytes) -> %d\n", path, size, rv);
  return rv;
}

// This is called on open, but doesn't need to do much
// since FUSE doesn't assume you maintain state for
// open files.
// You can just check whether the file is accessible.
int nufs_open(const char *path, struct fuse_file_info *fi) {
  int rv = nufs_access(path, 0);
  printf("open(%s) -> %d\n", path, rv);
  return rv;
}

// Actually read data from file at given path
int nufs_read(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi) {
  int rv = storage_read(path, buf, size, offset);
  printf("read(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
  return rv;
}

// Actually write data to file at given path
int nufs_write(const char *path, const char *buf, size_t size, off_t offset,
               struct fuse_file_info *fi) {
  int rv = storage_write(path, buf, size, offset);
  printf("write(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
  return rv;
}

// Update the timestamps on a file or directory.
int nufs_utimens(const char *path, const struct timespec ts[2]) {
  int rv = -1;
  printf("utimens(%s, [%ld, %ld; %ld %ld]) -> %d\n", path, ts[0].tv_sec,
         ts[0].tv_nsec, ts[1].tv_sec, ts[1].tv_nsec, rv);
  return rv;
}

// Extended operations
int nufs_ioctl(const char *path, int cmd, void *arg, struct fuse_file_info *fi,
               unsigned int flags, void *data) {
  int rv = -1;
  printf("ioctl(%s, %d, ...) -> %d\n", path, cmd, rv);
  return rv;
}

void nufs_init_ops(struct fuse_operations *ops) {
  memset(ops, 0, sizeof(struct fuse_operations));
  ops->access = nufs_access;
  ops->getattr = nufs_getattr;
  ops->readdir = nufs_readdir;
  ops->mknod = nufs_mknod;
  ops->mkdir = nufs_mkdir;
  ops->link = nufs_link;
  ops->unlink = nufs_unlink;
  ops->rmdir = nufs_rmdir;
  ops->rename = nufs_rename;
  ops->chmod = nufs_chmod;
  ops->truncate = nufs_truncate;
  ops->open = nufs_open;
  ops->read = nufs_read;
  ops->write = nufs_write;
  ops->utimens = nufs_utimens;
  ops->ioctl = nufs_ioctl;
};

struct fuse_operations nufs_ops;

int main(int argc, char *argv[]) {
  assert(argc > 2 && argc < 6);
  storage_init(argv[--argc]);
  nufs_init_ops(&nufs_ops);
  return fuse_main(argc, argv, &nufs_ops, NULL);
}
