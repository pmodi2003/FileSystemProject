#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "inode.h"
#include "bitmap.h"

const int NUM_INODES = 256; // number of inodes in file system (default = 256)
const int INODE_BLOCK = 1; // block where inodes begin

// Prints inode information
void print_inode(inode_t *node) {
    printf("refs: %d\n", node->refs);
    printf("mode: %d\n", node->mode);
    printf("size: %d\n", node->size);
    printf("block: %d\n", node->block);
    printf("nodes: %d\n", node->nodes);
}

// Return pointer to inode at given inum
inode_t *get_inode(int inum) {
    assert(inum < NUM_INODES);
    inode_t *inodes = (inode_t *) blocks_get_block(INODE_BLOCK);
    return inodes + inum;
}

// Return inum of newly allocated inode.
int alloc_inode() {
    // iterate inode bitmap to find first unused inode
    for (int ind = 0; ind < NUM_INODES; ind++) {
        if (bitmap_get(get_inode_bitmap(), ind) == 0) { // unused inode found
            bitmap_put(get_inode_bitmap(), ind, 1); // set bitmap bit to 1 to mark as used
            inode_t *inode = get_inode(ind); 
            memset(inode, 0, sizeof(inode_t)); // write inode bytes to 0

            // update inode information
            inode->refs = 0; 
            inode->mode = 0100644; // Regular file with read/write permissions for user
            inode->size = 0;
            inode->block = alloc_block(); // allocate block for inode data
            inode->nodes = 0; 

            return ind;
        }
    }

    return -1;
}

// Free inode at given inum.
void free_inode(int inum) {
    bitmap_put(get_inode_bitmap(), inum, 0); // set bitmap bit to 0 to mark as free

    inode_t* inode = get_inode(inum);
    free_block(inode->block); // deallocate data block

    memset(inode, 0, sizeof(inode_t)); // write inode bytes to 0
}
