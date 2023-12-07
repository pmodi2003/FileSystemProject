/**
 *@file directory.c
 *
 * Implementation of directory manipulation
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "directory.h"
#include "inode.h"
#include "helpers/slist.h"

// Initialize Root Directory with read and write permissions (root->mode)
void directory_init(){
    int inum = alloc_inode();
    inode_t *root = get_inode(inum);
    root->mode = 040755; 
}

// Return inum for file with given name in given directory
int directory_lookup(inode_t *di, const char *name){
    // Check to see if a file name is given
    if (strcmp(name, "") == 0){
        return 0;
    }

    dirent_t *folder = (dirent_t *)blocks_get_block(di->ptrs[0]);
    for(int i = 0; i < di->size / sizeof(dirent_t); i++){
        dirent_t dir = folder[i];

        if((strcmp(name, dir.name) == 0)){
            return dir.inum;
        }
    }
    return -ENOENT;
}

// Return inum for file at given path
int path_lookup(const char *path){
    slist_t *head = slist_explode(path, '/');
    slist_t *cur = head;
    int inum = 0;

    while (cur != NULL){
        inode_t *inode = get_inode(inum);
        inum = directory_lookup(inode, cur->data);
        if(inum == -ENOENT){
            slist_free(cur);
            return -ENOENT;
        } else {
            cur = cur->next;
        }
    }
    slist_free(head);

    return inum;
}

// Add file to given directory
int directory_put(inode_t *di, const char *name, int inum){
    dirent_t *dir = (dirent_t *)blocks_get_block(di->ptrs[0]);

    dirent_t newFile;
    strcpy(newFile.name, name);
    newFile.inum = inum;

    int size = di->size / sizeof(dirent_t);
    for(int i = 1; i < size; i++){
        if(dir[i].inum == -1){
            dir[i] = newFile;
            di->size = di->size + sizeof(dirent_t);
            return 0;
        }
    }

    dir[size] = newFile;
    di->size += sizeof(dirent_t);

    di->size = di->size + sizeof(dirent_t);
    return 0;
}

// Delete given file from given directory
int directory_delete(inode_t *di, const char *name){
    dirent_t *dir = (dirent_t *)blocks_get_block(di->ptrs[0]);

    for(int i = 0; i < di->size / sizeof(dirent_t); i++){
        if(strcmp(name, dir[i].name) == 0){
            int inum = dir[i].inum;
            dir[i].inum = -1;
            inode_t *inode = get_inode(inum);
            inode->refs -= 1;
            if(inode->refs == 0){
                free_inode(inum);
            }
            return 0;
        }
    }

    return -1;
}

// Return list of files at given path
slist_t *directory_list(const char *path){
    int inum = path_lookup(path);
    inode_t *inode = get_inode(inum);

    dirent_t *dir = (dirent_t *)blocks_get_block(inode->ptrs[0]);
    slist_t *list = NULL;

    for(int i = 0; i < inode->size / sizeof(dirent_t); i++){
        if(dir[i].inum != -1){
            list = slist_cons(dir[i].name, list);
        }
    }

    return list;
}

// Print contents of given directory
void print_directory(inode_t *di){
    dirent_t *dir = blocks_get_block(di->ptrs[0]);
    for(int i = 0; i < di->size / sizeof(dirent_t); i++){
        printf("%s\n", dir[i].name);
    }
}