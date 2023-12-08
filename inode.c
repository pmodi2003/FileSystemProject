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
    //return (inode_t *) (inode_base + (inum * sizeof(inode_t)));
}

// Allocate new inode and return its inum
int alloc_inode(){
    int inum = 0; 
    void *inode_bm = get_inode_bitmap();

    while (bitmap_get(inode_bm, inum) == 1 && inum < BLOCK_COUNT){
        inum++;
    }
    if(inum < BLOCK_COUNT && bitmap_get(inode_bm, inum) == 0){
        bitmap_put(inode_bm, inum, 1);
        printf("+ alloc_inode() -> %d\n", inum);
        inode_t *new_inode = get_inode(inum);
        memset(new_inode, 0, sizeof(inode_t));
        new_inode->refs = 1;
        new_inode->mode = 0100644;
        new_inode->size = 0;
        new_inode->block = alloc_block();
        new_inode->files = 0;

        return inum;
    } else {
        return -1;
    }
}
// Free inode for given inum
void free_inode(int inum){
    void *inode_bm = get_inode_bitmap();
    bitmap_put(inode_bm, inum, 0);

    inode_t *inode = get_inode(inum);
    free_block(inode->block);
    memset(inode, 0, sizeof(inode_t));
}