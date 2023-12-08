/**
 *@file inode.c
 *
 * Implementation of inode manipulation
 */

#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "inode.h"
#include "helpers/bitmap.h"

const int NUM_INODES = 256; // number of inodes in FS (1 for each block -> 256 inodes)
const int INODES_START_BLOCK = 1; // block where inodes start

// Print inode data
void print_inode(inode_t *node){
    printf("refs: %d\n", node->refs);
    printf("mode: 0x%X\n", node->mode);
    printf("size: %d\n", node->size);
    printf("block: %d\n", node->block);
    printf("files: %d\n", node->files);
}

// Return pointer to inode for given inum
inode_t *get_inode(int inum){
    assert(inum < NUM_INODES);
    inode_t *inode_base = (inode_t *) blocks_get_block(INODES_START_BLOCK);
    return inode_base + inum;
}

// Allocate new inode and return its inum
int alloc_inode(){
    for (int bit_ind = 0; bit_ind < NUM_INODES; bit_ind++) {
        if (bitmap_get(get_inode_bitmap(), bit_ind) == 0) {
            bitmap_put(get_inode_bitmap(), bit_ind, 1);
            printf("+ alloc_inode() -> %d\n", bit_ind);
            inode_t *new_inode = get_inode(bit_ind);
            memset(new_inode, 0, sizeof(inode_t));

            new_inode->refs = 0;
            new_inode->mode = 0100644;
            new_inode->size = 0;
            new_inode->block = alloc_block();
            new_inode->files = 0;

            return bit_ind;
        }
    }

    return -1;
}
// Free inode for given inum
void free_inode(int inum){
    bitmap_put(get_inode_bitmap(), inum, 0);
    printf("+ free_inode(%d)\n", inum);

    inode_t *inode = get_inode(inum);
    free_block(inode->block);
    
    memset(inode, 0, sizeof(inode_t));
}