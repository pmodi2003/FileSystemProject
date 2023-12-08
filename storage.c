/**
 *@file storage.c
 *
 * Implementation of disk storage manipulation
 */

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "storage.h"
#include "directory.h"
#include "inode.h"
#include "helpers/bitmap.h"
#include "helpers/blocks.h"
#include "helpers/slist.h"


//Initialize Files Structure
void storage_init(const char *path){
    // Initialize blocks
    blocks_init(path);
        // Initialize directory
    directory_init();
}

// Access file at given path
int storage_access(const char *path){
    int inum = path_lookup(path);
    if(inum == -ENOENT){
        return -ENOENT;
    }

    return 0;
}

//Update provided stat pointer with file stats of inode at path
int storage_stat(const char *path, struct stat *st){
    int inum = path_lookup(path);
    if(inum == -ENOENT){
        return -ENOENT;
    }
    inode_t *inode = get_inode(inum);
    st->st_mode = inode->mode;
    st->st_size = inode->size;
    st->st_nlink = inode->refs;
    st->st_ino = inum;
    return 0;
}

//Read size bytes from given path and offset and copy them to buf
int storage_read(const char *path, char *buf, size_t size, off_t offset){
    int inum = path_lookup(path);
    if(inum == -ENOENT){
        return 0;
    }
    inode_t *inode = get_inode(inum);

    if(offset >= inode->size){
        return 0;
    }

    size_t read_bytes;
    if(offset + size > inode->size){
        read_bytes = inode->size - offset;
    } else {
        read_bytes = size;
    }

    char *begin_read = (char *) (blocks_get_block(inode->block) + offset);

    memcpy(buf, begin_read, read_bytes);
    return read_bytes;
}

//Write size bytes to given path and offset and copy them from buf
int storage_write(const char *path, const char *buf, size_t size, off_t offset){
    if((storage_truncate(path, offset + size)) == -1){
        return 0;
    } 

    int inum = path_lookup(path);
    if(inum == -ENOENT){
        return 0;
    }
    inode_t *inode = get_inode(inum);

    if(offset >= BLOCK_SIZE){
        return 0;
    }

    size_t write_bytes;
    if(offset + size > BLOCK_SIZE){
        write_bytes = BLOCK_SIZE - offset;
    } else {
        write_bytes = size;
    }

    char *begin_write = (char *) (blocks_get_block(inode->block) + offset);

    memcpy(begin_write, buf, write_bytes);
    return write_bytes;
}

// Truncate file at given path to given size
int storage_truncate(const char *path, off_t size){
    assert(size >= 0);
    int inum = path_lookup(path);
    if(inum == -ENOENT){
        return -1;
    }

    if (size > BLOCK_SIZE){
        return -1;
    }

    inode_t *inode = get_inode(inum);

    if(inode->size < size){
        char *block = (char *) blocks_get_block(inode->block);
        block += inode->size;
        memset(block, 0, size - inode->size);
        inode->size = size;
    } else {
        inode->size = size;
    }
    return 0;
}

// Make file/directory at given path with given mode attributes
int storage_mknod(const char *path, int mode, int dir){
    if(path_lookup(path) != -ENOENT){
        return -EEXIST;
    }

    char *par_dir = malloc(strlen(path) + 48);
    memset(par_dir, 0, strlen(path) + 48);
    par_dir[0] = '/';
    
    slist_t *path_list = slist_explode(path + 1,  '/');
    slist_t *iter_path = path_list;

    while(iter_path != NULL){
        printf("Looking at %s\n", iter_path->data);
        int par_inum = path_lookup(par_dir);
        inode_t *par_inode = get_inode(par_inum);
        int child_inum = directory_lookup(par_inode, iter_path->data);
        inode_t *child_inode = get_inode(child_inum);

        if(iter_path->next == NULL && child_inum == -ENOENT){
            int new_inum = alloc_inode();
            inode_t *new_inode = get_inode(new_inum);
            new_inode->mode = mode;

            if(dir){
                directory_put(new_inode, "..", path_lookup(par_dir));
                directory_put(new_inode, ".", new_inum);
                new_inode->refs = 2;
                new_inode->files = 2;
            }

            int rv = directory_put(par_inode, iter_path->data, new_inum);
           if (rv == -1) {
                free(par_dir);
                slist_free(path_list);
                return -1;
            }
            
            free(par_dir);
            s_free(path_list);
            return 0;
        } else {
            if(strcmp(par_dir, "/") == 0){
                strcpy(par_dir + strlen(par_dir), iter_path->data);
            } else {
                strcpy(par_dir + strlen(par_dir), "/");
                strcpy(par_dir + strlen(par_dir), iter_path->data);
            }
        }
        iter_path = iter_path->next;
    }
}

// Change mode attributes of file at given path
int storage_chmod(const char *path, int mode){
    int inum = path_lookup(path);
    if(inum == -ENOENT){
        return -1;
    }
    inode_t *inode = get_inode(inum);
    
    inode->mode = mode;
    return 0;
}

// Remove directory at given path
int storage_rmdir(const char *path){
    int inum = path_lookup(path);
    if(inum == -ENOENT){
        return -1;
    }
    inode_t *inode = get_inode(inum);

    if(inode->mode != 040755 && inode->mode != 40775 && inode->mode != 16893){
        return -1;
    } else if (strcmp(path, "/") == 0){
        return -1;
    } else if (inode->files > 2){
        return -1;
    }

    int rv = storage_unlink(path);
    return rv;
}

//Unlink file/directory at given path from file system
int storage_unlink(const char *path){
    int inum = path_lookup(path);
    if(inum == -ENOENT){
        return -1;
    }
    
    char *par_dir = (char *) malloc(strlen(path) + 1);
    char *cur_dir = (char *) malloc(strlen(path) + 1);
    if (strcmp(path, "/") == 0) {
        par_dir = "/";
        cur_dir = "/";
    } else {
        strcpy(par_dir, path);
        strcpy(cur_dir, path);

        int child_dir_length = 0;
        for (int i = strlen(path) - 1; path[i] != '/'; i--) {
            child_dir_length += 1;
        }

        if (child_dir_length == strlen(path) - 1) {
            par_dir[1] = '\0';
        } else {
            par_dir[strlen(path) - child_dir_length - 1] = '\0';
        }

        cur_dir += strlen(path) - child_dir_length;
    }

    int par_inum = path_lookup(par_dir);
    inode_t *par_inode = get_inode(par_inum);

    int rv = directory_delete(par_inode, cur_dir);
    free(par_dir);
    free(cur_dir);
    return rv;
}

// Link from file to file system 
int storage_link(const char *from, const char *to){
    int inum = path_lookup(to);
    if(inum == -ENOENT){
        return -1;
    }
    
    char *par_dir = (char *) malloc(strlen(from) + 1);
    char *cur_dir = (char *) malloc(strlen(from) + 1);
    if (strcmp(from, "/") == 0) {
        par_dir = "/";
    }

    strcpy(par_dir, from);
    strcpy(cur_dir, from);

    int child_dir_length = 0;
    for (int i = strlen(from) - 1; from[i] != '/'; i--) {
        child_dir_length += 1;
    }

    if (child_dir_length == strlen(from) - 1) {
        par_dir[1] = '\0';
    } else {
        par_dir[strlen(from) - child_dir_length - 1] = '\0';
    }

    int par_inum = path_lookup(par_dir);
    inode_t *par_inode = get_inode(par_inum);

    cur_dir += strlen(from) - child_dir_length;

    int rv = directory_put(par_inode, cur_dir, inum);
    free(par_dir);
    free(cur_dir);
    return rv;
}

//Rename file from "from" to "to"
int storage_rename(const char *from, const char *to){
    storage_link(to, from);
    storage_unlink(from);
    return 0;
}

// Return list of files and subdirectories at given path
slist_t *storage_list(const char *path){
    return directory_list(path);
}