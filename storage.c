/**
 *@file storage.c
 *
 * Implementation of disk storage manipulation
 */

#include <errno.h>
#include <string.h>
#include <stdlib.h>

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

// Access given path
int storage_access(const char *path){
    int inum;
    if((inum = path_lookup(path)) != -ENOENT){
        inode_t *inode = get_inode(inum);
        inode->accessed_time = time(NULL);
        return 0;
    } else {
        return -ENOENT;
    }
}

//Update provided stat pointer with file stats of inode at path
int storage_stat(const char *path, struct stat *st){
    int inum;
    // Check if inode was found
    if((inum = path_lookup(path)) != -ENOENT){
        inode_t *inode = get_inode(inum);
        st->st_mode = inode->mode;
        st->st_size = inode->size;
        st->st_atime = inode->accessed_time;
        st->st_mtime = inode->modified_time;
        st->st_ctime = inode->created_time;
        st->st_nlink = inode->refs;
        return 0;
    } else {
        return -ENOENT;
    }
}

//Read size bytes from given path and offset and copy them to buf
int storage_read(const char *path, char *buf, size_t size, off_t offset){
    int inum = path_lookup(path);
    inode_t *inode = get_inode(inum);

    int sizeArg = size;
    int offsetArg = offset;

    int i = 0;
    while(sizeArg > 0){
        int bnum = inode_get_bnum(inode, offsetArg);
        char *block = (char *)blocks_get_block(bnum);
        block += offsetArg % BLOCK_SIZE;

        int min;
        if(sizeArg <= BLOCK_SIZE - (offsetArg % BLOCK_SIZE)){
            min = sizeArg;
        } else {
            min = BLOCK_SIZE - (offsetArg % BLOCK_SIZE);
        }

        memcpy(buf + i, block, min);
        i += min;
        offsetArg += min;
        sizeArg -= min;
    }

    return size;

}

//Write size bytes to given path and offset and copy them from buf
int storage_write(const char *path, const char *buf, size_t size, off_t offset){
    int inum = path_lookup(path);
    inode_t *inode = get_inode(inum);

    int offsetSize = size + offset;
    if(inode->size < offsetSize){
        storage_truncate(path, offsetSize);
    }

    int sizeArg = size;
    int offsetArg = offset;

    int i = 0;
    while (sizeArg > 0){
        int bnum = inode_get_bnum(inode, offsetArg);
        char *block = (char *)blocks_get_block(bnum);
        block += offsetArg % BLOCK_SIZE;

        int min;
        if(sizeArg <= BLOCK_SIZE - (offsetArg % BLOCK_SIZE)){
            min = sizeArg;
        } else {
            min = BLOCK_SIZE - (offsetArg % BLOCK_SIZE);
        }

        memcpy(block, buf + i, min);
        i+= min;
        offsetArg += min;
        sizeArg -= min;
    }

    return size;

}

// Truncate file at given path to given size
int storage_truncate(const char *path, off_t size){
    int inum = path_lookup(path);
    inode_t *inode = get_inode(inum);

    if(inode->size < size){
        grow_inode(inode, size);
    } else {
        shrink_inode(inode, size);
    }
    return 0;
}

// Make file/directory at given path with given mode attributes
int storage_mknod(const char *path, int mode){
    if(path_lookup(path) != -ENOENT){
        return -EEXIST;
    }

    char *cur_dir = malloc(DIR_NAME_LENGTH);
    char *par_dir = malloc(strlen(path));
    

    slist_t *head = slist_explode(path, "/");
    slist_t *cur = head;

    par_dir[0] = 0;
    while(cur->next){
        strcat(par_dir, "/");
        strcat(par_dir, cur->data);
        cur = cur->next;
    }
    memcpy(cur_dir, cur->data, strlen(cur->data));
    cur_dir[strlen(cur->data)] = 0;
    slist_free(head);
    

    int par_inum;
    if((par_inum = path_lookup(par_dir)) == -ENOENT){
        free(cur_dir);
        free(par_dir);
        return -ENOENT;
    }

    int new_inode_inum = alloc_inode();
    inode_t *inode = get_inode(new_inode_inum);
    inode->mode = mode;
    inode->size = 0;
    inode->refs = 1;

    inode_t *par_inode = get_inode(par_inum);
    directory_put(par_inode, cur_dir, new_inode_inum);

    free(cur_dir);
    free(par_dir);

    return 0;

}

// Change mode attributes of file at given path
int storage_chmod(const char *path, int mode){
    int inum;
    if((inum = path_lookup(path)) == -ENOENT){
        return -ENOENT;
    }

    inode_t *inode = get_inode(inum);
    inode->mode = mode;
    return 0;
}

//Unlink file at given path from file system
int storage_unlink(const char *path){
    char *cur_dir = malloc(DIR_NAME_LENGTH);
    char *par_dir = malloc(strlen(path));
    
    
    slist_t *head = slist_explode(path, "/");
    slist_t *cur = head;

    par_dir[0] = 0;
    while(cur->next){
        strcat(par_dir, "/");
        strcat(par_dir, cur->data);
        cur = cur->next;
    }
    memcpy(cur_dir, cur->data, strlen(cur->data));
    cur_dir[strlen(cur->data)] = 0;
    slist_free(head);


    int par_inum = path_lookup(par_dir);
    inode_t *par_inode = get_inode(par_inum);
    int rv = directory_delete(par_inode, cur_dir);
    
    free(cur_dir);
    free(par_dir);
    return rv;
}

// Link from file to file system 
int storage_link(const char *from, const char *to){
    int inum;
    if((inum = path_lookup(to)) == -ENOENT){
        return -ENOENT;
    }

    char *cur_dir = malloc(DIR_NAME_LENGTH);
    char *par_dir = malloc(strlen(from));
    
    
    slist_t *head = slist_explode(from, "/");
    slist_t *cur = head;

    par_dir[0] = 0;
    while(cur->next){
        strcat(par_dir, "/");
        strcat(par_dir, cur->data);
        cur = cur->next;
    }
    memcpy(cur_dir, cur->data, strlen(cur->data));
    cur_dir[strlen(cur->data)] = 0;
    slist_free(head);
    

    int par_inum = path_lookup(par_dir);
    inode_t *par_inode = get_inode(par_inum);
    directory_put(par_inode, cur_dir, inum);

    inode_t *inode = get_inode(inum);
    inode->refs++;
    
    free(cur_dir);
    free(par_dir);
    return 0;
}

//Rename file from "from" to "to"
int storage_rename(const char *from, const char *to){
    storage_link(from, to);
    storage_unlink(from);
    return 0;
}

// Set time stats for file at given path
int storage_set_time(const char *path, const struct timespec ts[2]){
    int inum;
    if ((inum = path_lookup(path)) == -ENOENT){
        return -ENOENT;
    }

    inode_t *inode = get_inode(inum);
    inode->accessed_time = ts[0].tv_sec;
    inode->modified_time = ts[1].tv_sec;

    return 0;
}

// Return list of files and subdirectories at given path
slist_t *storage_list(const char *path){
    return directory_list(path);
}