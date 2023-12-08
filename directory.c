#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "directory.h"
#include "inode.h"
#include "slist.h"

// Initializes the root directory
void directory_init() {
    int inum = alloc_inode();
    inode_t *inode = get_inode(inum);
    inode->mode = 040755;

    directory_put(inode, ".", inum);
    inode->refs = 1;
    inode->nodes = 1;
}

// Finds the file with the given name in the given directory
int directory_lookup(inode_t *dd, const char *name) {
    dirent_t *block = (dirent_t *) blocks_get_block(dd->block);

    // Iterate through the nodes in this directory
    dirent_t *curr_dirent = block;
    int curr_i = 0;
    while (curr_i < dd->nodes) {
        curr_dirent = block + curr_i;
        if (strcmp(curr_dirent->name, name) == 0) {
            return curr_dirent->inum;
        }
        curr_i += 1;
    }

    return -1;
}

// Gets the inum at the given path.
int path_lookup(const char *path) {
    if (strcmp("/", path) == 0) {
        return 0;
    }

    // Iterate through the path, looking up each directory
    int curr_dir_inum = 0;
    slist_t *head = slist_explode(path + 1, '/');
    slist_t *cur = head;
    while (cur != NULL) {
        inode_t *curr_dir_inode = get_inode(curr_dir_inum);
        curr_dir_inum = directory_lookup(curr_dir_inode, cur->data);
        if (curr_dir_inum == -1) {
            slist_free(head);
            return -1;
        }

        cur = cur->next;
    }

    slist_free(head);
    return curr_dir_inum;
}

// Creates a new directory entry in the given directory with the given name and inum.
int directory_put(inode_t *dd, const char *name, int inum) {
    if (dd->nodes == BLOCK_SIZE / sizeof(dirent_t)) {
        return -1;
    }

    inode_t *entry_node = get_inode(inum);

    // Update the directory data block
    dirent_t *block = (dirent_t *) blocks_get_block(dd->block);
    dirent_t *new_entry = block + dd->nodes;
    strcpy(new_entry->name, name);
    new_entry->inum = inum;

    // Update the directory inode
    if (entry_node->nodes > 0) {  
        dd->refs += 1;
    }
    dd->size += sizeof(dirent_t);
    dd->nodes += 1;
    entry_node->refs += 1;

    return 0;
}

// Deletes the inode with the given name from the given directory.
int directory_delete(inode_t *dd, const char *name) {
    dirent_t *dir = (dirent_t *) blocks_get_block(dd->block);
    dirent_t *curr_dirent = dir;

    // Iterate through the nodes in this directory
    int curr_i = 0;
    while (curr_i < dd->nodes) {
        curr_dirent = dir + curr_i;
        if (strcmp(curr_dirent->name, name) == 0) {
            int entry_inum = curr_dirent->inum;

            // Update directory data block
            if (curr_i < dd->nodes - 1) {
                dirent_t *next_entry = curr_dirent + 1;
                int num_nodes = dd->nodes - curr_i - 1;
                memmove(curr_dirent, next_entry, num_nodes * sizeof(dirent_t));
            }

            // Update directory inode
            inode_t *entry_inode = get_inode(entry_inum);
            if (entry_inode->nodes > 0) {  
                dd->refs -= 1;
            }
            dd->size -= sizeof(dirent_t);
            dd->nodes -= 1;

            // Update entry inode
            entry_inode->refs -= 1;
            if (entry_inode->nodes > 0 && entry_inode->refs <= 1) { 
                free_block(entry_inode->block);
            }
            else if (entry_inode->nodes == 0 && entry_inode->refs == 0) { 
                free_inode(entry_inum);
                free_block(entry_inode->block);
            }

            return 0;
        }
        curr_i += 1;
    }

    return -1;
}

// Lists the contents of the directory specified by the given path.
slist_t *directory_list(const char *path) {
    int inum = path_lookup(path);
    assert(inum >= 0);
    inode_t *inode = get_inode(inum);
    dirent_t *block = (dirent_t *) blocks_get_block(inode->block);

    dirent_t *curr_dirent = block;
    slist_t *list = 0;

    int curr_i = 0;
    while (curr_i < inode->nodes) {
        curr_dirent = block + curr_i;
        list = slist_cons(curr_dirent->name, list);
        curr_i += 1;
    }

    return list;
}