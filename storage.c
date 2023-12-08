#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <alloca.h>
#include <stdlib.h>
#include <assert.h>

#include "storage.h"
#include "directory.h"
#include "blocks.h"
#include "slist.h"

// Initializes storage for file system in user space
void storage_init(const char *path) {
    blocks_init(path); // initialize blocks at given path
    directory_init(); // initialize root directory
}

// Update given stat struct with stats of inode for given path
int storage_stat(const char *path, struct stat *st) {
    int inum = path_lookup(path);
    
    // return -ENOENT if no inode was found for given path
    if (inum == -1) {
        return -ENOENT;
    }

    // Update stat struct with inode information
    inode_t *inode = get_inode(inum);
    st->st_nlink = inode->refs;
    st->st_mode = inode->mode;
    st->st_size = inode->size;
    st->st_uid = getuid();
    return 0;
}

// Read size bytes from given path + offset to given buffer and return
// number of bytes read
int storage_read(const char *path, char *buf, size_t size, off_t offset) {
    int inum = path_lookup(path);
    // return - bytes if no file found at path
    if (inum == -1) {
        return 0;
    }

    inode_t *inode = get_inode(inum);

    // return 0 bytes of offset is larger than size of inode
    if (offset >= inode->size) {
        return 0;
    }

    // Limit read size to minimum bytes available
    size_t size_to_read;
    if (offset + size > inode->size) {
        size_to_read = inode->size - offset;
    } else {
        size_to_read = size;
    }

    // Get the block and read data starting from the offset
    char *begin_read = (char *) blocks_get_block(inode->block) + offset;

    memcpy(buf, begin_read, size_to_read);
    return size_to_read;
}

// Write size bytes to given path + offset from given buffer and return
// number of bytes written
int storage_write(const char *path, const char *buf, size_t size, off_t offset) {
    int rv = storage_truncate(path, offset + size); // Resize inode at path to fit size + offset
    // return 0 bytes written if truncating file size was unsuccessful
    if (rv == -1) {
        return 0;
    }

    int inum = path_lookup(path);
    // return 0 bytes written if file not found
    if (inum == -1) {
        return 0;
    }

    inode_t *inode = get_inode(inum);
    
    //return 0 bytes written if offset is larger than BLOCK_SIZE (4kb);
    if (offset >= BLOCK_SIZE) {
        return 0;
    }

    // Limit write size to minimum bytes available
    size_t size_to_write;
    if (offset + size > BLOCK_SIZE) {
        size_to_write = BLOCK_SIZE - offset;
    } else {
        size_to_write = size;
    }

    // Get the block and write data starting from the offset
    char *begin_write = (char *) blocks_get_block(inode->block) + offset;

    memcpy(begin_write, buf, size_to_write);
    return size_to_write;
}

// Truncates inode at path to given size
int storage_truncate(const char *path, off_t size) {
    assert(size >= 0);
    int inum = path_lookup(path);
    // return -1 if file can't be found
    if (inum == -1) {
        return -1;
    }
    
    //return -1 if size is larger than BLOCK_SIZE
    if (size > BLOCK_SIZE) {
        return -1;
    }

    inode_t *inode = get_inode(inum);
    if (size <= inode->size) { // update inode size if it is shrinking or staying the same
        inode->size = size;
    }
    else { // set new bytes to 0 if inode size is increasing
        char *block = (char *) blocks_get_block(inode->block);
        block += inode->size;
        memset(block, 0, size - inode->size);
        inode->size = size;
    }
    return 0;
}

// Make file/directory at given poth iwth given mode permission and type
int storage_mknod(const char *path, int mode) {
    // return -1 if file already exists
    if (path_lookup(path) != -1) {
        return -1;
    }

    // get the parent directory of the new node
    char *par_dir = malloc(strlen(path) + 48);
    memset(par_dir, 0, strlen(path) + 48);
    par_dir[0] = '/';

    // Split path by '/' and iterate through each item
    slist_t *path_list = slist_explode(path + 1, '/');
    slist_t *iter_path = path_list;
    while (iter_path != NULL) {
        int par_dir_inum = path_lookup(par_dir);
        inode_t *par_dir_inode = get_inode(par_dir_inum);
        int child_inum = directory_lookup(par_dir_inode, iter_path->data);

        // the parent directory has been reached and the new node needs to be created
        if (iter_path->next == NULL && child_inum == -1) {
            int new_inum = alloc_inode();
            inode_t *new_inode = get_inode(new_inum); 
            new_inode->mode = mode;

            if (mode == 040755) { // if new node is a directory
                directory_put(new_inode, "..", path_lookup(par_dir)); // link parent reference in directory
                directory_put(new_inode, ".", new_inum); // link self reference in directory
                new_inode->refs = 2;
                new_inode->nodes = 2;
            }

            // link new node to parent directory
            int rv = directory_put(par_dir_inode, iter_path->data, new_inum);
            if (rv == -1) {
                free(par_dir);
                slist_free(path_list);
                return -1;
            }

            free(par_dir);
            slist_free(path_list);
            return 0;
        }
        // Continue to next path level
        else {
            if (strcmp(par_dir, "/") == 0) {
                strcpy(par_dir + strlen(par_dir), iter_path->data);
            } else {
                strcpy(par_dir + strlen(par_dir), "/");
                strcpy(par_dir + strlen(par_dir), iter_path->data);
            }
        }
        iter_path = iter_path->next;
    }
}

// Unlink node at given path
int storage_unlink(const char *path) {
    int node_inum = path_lookup(path);
    // return -1 if node is not found at path
    if (node_inum == -1) {
        return -1;
    }

    // Get the parent path and inode
    char *par_dir = alloca(strlen(path) + 1);
    par_dir = path_to_parent(path, par_dir);
    int par_dir_inum = path_lookup(par_dir);
    inode_t *par_dir_inode = get_inode(par_dir_inum);

    // get node name
    char *node = alloca(strlen(path) + 1);
    node = get_name(path, node);

    int rv = directory_delete(par_dir_inode, node);
    return rv;
}

// Links node at from path to node at to path
int storage_link(const char *from, const char *to) {
    int to_inum = path_lookup(to);
    if (to_inum == -1) {
        return -1;
    }

    // Get the parent path and inode
    char *par_dir = alloca(strlen(from) + 1);
    par_dir = path_to_parent(from, par_dir);
    int par_inum = path_lookup(par_dir);
    inode_t *par_inode = get_inode(par_inum);

    // get node name
    char *from_node = alloca(strlen(from) + 1);
    from_node = get_name(from, from_node);

    int rv = directory_put(par_inode, from_node, to_inum);
    return rv;
}

// Renames node at from path to node at to path
int storage_rename(const char *from, const char *to) {
    storage_link(to, from);
    storage_unlink(from);
    return 0;
}

// Return list of nodes in directory at given path
slist_t *storage_list(const char *path) {
    return directory_list(path);
}

// Remove directory at given path
int storage_rmdir(const char *path) {
    int inum = path_lookup(path);
    inode_t *inode = get_inode(inum);

    if (inode->mode != 040755) { // check to see if it is a directory
        return -1;
    }
    else if (strcmp(path, "/") == 0) { // make sure it isn't root directory
        return -1;
    }
    else if (inode->nodes > 2) { // can't delete directory with nodes in it
        return -1;
    }

    int rv = storage_unlink(path);
    return rv;
}


// Change permission for node at given path
int storage_chmod(const char *path, mode_t mode) {
    int inum = path_lookup(path);
    inode_t *inode = get_inode(inum);
    
    inode->mode = mode;
    return 0;
}

// Get the path of the parent directory of this path.
char *path_to_parent(const char *path, char *parent_path) {
    if (strcmp(path, "/") == 0) {
        return "/";
    }

    strcpy(parent_path, path);

    int child_name_length = 0;
    for (int i = strlen(path) - 1; path[i] != '/'; i--) {
        child_name_length += 1;
    }

    parent_path[strlen(path) - child_name_length] = '\0';   
    return parent_path;
}

// Get the name of the child directory of this path.
char *get_name(const char *path, char *child_name) {
    if (strcmp(path, "/") == 0) {
        return "/";
    }

    strcpy(child_name, path);

    int child_name_length = 0;
    for (int i = strlen(path) - 1; path[i] != '/'; i--) {
        child_name_length += 1;
    }

    child_name += strlen(path) - child_name_length;
    return child_name;
}
