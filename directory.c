/**
 *@file directory.c
 *
 * Implementation of directory manipulation
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "directory.h"
#include "inode.h"
#include "helpers/slist.h"

// Initialize Root Directory with read and write permissions (root->mode)
// It initially has the self reference "."
// All other directories have a self-reference and a parent reference ".."
// Self reference does not count as a reference
void directory_init(){
    int inum = alloc_inode();
    inode_t *root = get_inode(inum);
    root->mode = 040755;

    directory_put(root, ".", inum);
    root->refs = 1;
    root->files = 1;
}

// Return inum for file with given name in given directory
int directory_lookup(inode_t *di, const char *name){
    dirent_t *block = (dirent_t *)blocks_get_block(di->block);

    dirent_t *iter_dir = block;
    int ind = 0;
    while(ind < di->files){
        iter_dir = block + ind;
        if((strcmp(name, iter_dir->name) == 0)){
            return iter_dir->inum;
        }
        ind++;
    }
    return -ENOENT;
}

// Return inum for file at given path
int path_lookup(const char *path){
    if (strcmp("/", path) == 0) {
        return 0;
    }

    slist_t *path_list = slist_explode(path + 1, '/');
    slist_t *iter_path = path_list;
    
    int inum = 0;
    while (iter_path != NULL){
        inode_t *inode = get_inode(inum);
        inum = directory_lookup(inode, iter_path->data);
        if(inum == -ENOENT){
            slist_free(path_list);
            return -ENOENT;
        }
        iter_path = iter_path->next;
    }

    slist_free(path_list);
    return inum;
}

// Add file to given directory
int directory_put(inode_t *di, const char *name, int inum){
    if (di->files == BLOCK_SIZE / sizeof(dirent_t)) {
        printf("Can't add new file due to full directory.\n");
        return -1;
    }

    dirent_t *dir = (dirent_t *) blocks_get_block(di->block);
    dirent_t *new = dir + di->files;
    strcpy(new->name, name);
    new->inum = inum;

    inode_t *entry = get_inode(inum);
    if(entry->files > 0){
        di->refs++;
    }
    di->size += sizeof(dirent_t);
    di->files++;

    entry->refs += 1;

    return 0;
}

// Delete given file from given directory
int directory_delete(inode_t *di, const char *name){
    dirent_t *dir = (dirent_t *)blocks_get_block(di->block);
    dirent_t *iter_dir = dir;
    int dir_ind = 0;
    while(dir_ind < di->files){
        iter_dir = dir + dir_ind;
        if(strcmp(name, iter_dir->name) == 0){
            int inum = iter_dir->inum;

            if(dir_ind < di->files - 1){
                dirent_t *next_file = iter_dir + 1;
                int num_files_below = di->files - dir_ind - 1;
                memmove(iter_dir, next_file, num_files_below * sizeof(dirent_t));
            }
            
            inode_t *file_inode = get_inode(inum);
            if(file_inode->files > 0){
                di->refs--;
            }
            di->size -= sizeof(dirent_t);
            di->files--;

            file_inode->refs--;
            if(file_inode->files > 0 && file_inode->refs <= 1){
                free_inode(inum);
                free_block(file_inode->block);
            } else if (file_inode->files == 0 && file_inode->refs == 0){
                free_inode(inum);
                free_block(file_inode->block);
            }

            return 0;
        }
        dir_ind++;
    }
    return -1;
}

// Return list of files at given path
slist_t *directory_list(const char *path){
    int inum = path_lookup(path);
    assert(inum >= 0);
    
    inode_t *inode = get_inode(inum);
    dirent_t *dir = (dirent_t *)blocks_get_block(inode->block);
    
    dirent_t *iter_dir = dir;
    slist_t *list = 0;
    
    int dir_ind = 0;
    while(dir_ind < inode->files){
        iter_dir = dir + dir_ind;
        list = slist_cons(iter_dir->name, list);
        dir_ind++;
    }

    return list;
}

// Print contents of given directory
void print_directory(inode_t *di){
    dirent_t *dir = (dirent_t *) blocks_get_block(di->block);
    dirent_t *iter_dir = dir;
    int dir_ind = 0;
    while(dir_ind < di->files){
        iter_dir = dir + dir_ind;
        printf("%s\n", iter_dir->name);
        dir_ind++;
    }
    return;
}