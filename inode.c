/**
 *@file inode.c
 *
 * Implementation of inode manipulation
 */

#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include "inode.h"
#include "helpers/bitmap.h"

// Print inode data
void print_inode(inode_t *node){
    if(node){
        printf("node position: %p\n", node);
        printf("refs: %d\n", node->refs);
        printf("mode: 0x%X\n", node->mode);
        printf("size: %d\n", node->size);
        printf("indirect pointer: %d\n", node->block);
        printf("pointers: %d, %d\n", node->ptrs[0], node->ptrs[1]);
    } else {
        printf("node: null");
    }
}

// Return pointer to inode for given inum
inode_t *get_inode(int inum){
    inode_t *inode_base = (inode_t *) get_inode_bitmap();
    return (inode_t *) (inode_base + (inum * sizeof(inode_t)));
}

// Allocate new inode and return its inum
int alloc_inode(){
    int inum = 0; 
    void *inode_bm = get_inode_bitmap();

    while (bitmap_get(inode_bm, inum) == 1 && inum < BLOCK_COUNT){
        inum++;
    }
    if(bitmap_get(inode_bm, inum) == 0 && inum < BLOCK_COUNT){
        bitmap_put(inode_bm, inum, 1);
        inode_t *new_inode = get_inode(inum);
        new_inode->refs = 1;
        new_inode->mode = 0;
        new_inode->size = 0;
        new_inode->ptrs[0] = alloc_block();
        return inum;
    } else {
        return -1;
    }
}
// Free inode for given inum
void free_inode(int inum){
    void *inode_bm = get_inode_bitmap();
    inode_t *inode = get_inode(inum);
    shrink_inode(inode, 0);
    free_block(inode->ptrs[0]);
    bitmap_put(inode_bm, inum, 0);
}

// Grow given inode to provided size
int grow_inode(inode_t *node, int size){
    for(int i = (node->size / BLOCK_SIZE) + 1; i <= size / BLOCK_SIZE; i++){
        if(i < 2){
            node->ptrs[i] = alloc_block();
        } else {
            if (node->block == 0){
                node->block = alloc_block();
            }
            int* block = (int *)blocks_get_block(node->block);
            block[i-2] = alloc_block();
        }
    }
    node->size = size;
    return 0;
}

// Shrink given inode to provided size
int shrink_inode(inode_t *node, int size){
    for(int i = (node->size / BLOCK_SIZE); i > size / BLOCK_SIZE; i--){
        if(i < 2){
            free_block(node->ptrs[i]);
            node->ptrs[i] = 0;
        } else {
            int* block = (int *)blocks_get_block(node->block);
            free_block(block[i-2]);
            block[i-2] = 0;
            if(i == 2){
                free_block(node->block);
                node->block = 0;
            }
        }
    }
    node->size = size;
    return 0;
}

// Return block number for given inode
int inode_get_bnum(inode_t *node, int file_bnum){
    int bnum = file_bnum / BLOCK_SIZE;

    if(bnum < 2){
        return node->ptrs[bnum];
    } else {
        int *block = (int *)blocks_get_block(node->block);
        return block[bnum-2];
    }

}