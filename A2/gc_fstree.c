#include <stdio.h>
#include "fstree.h"
#include "gc.h"

/* Traverse the fstree data-structure and call mark_one(void *vptr) on
 * each piece of memory that is still used in the fstree
 *
 * mark_one returns 0 if the chunk was marked for the first time and 1 if
 * it has been seen already. We need this to avoid revisiting pieces of the
 * tree that we have already marked -- where hard links have created cycles
 */

void mark_fstree(void *head)
{
    Fstree *node = (Fstree *)head;

    // Base case, if the node is null return
    if (node == NULL)
    {
        return;
    }

    int result = mark_one(node);

    // if the return value of mark_one is 1, it's already been marked so we return to avoid a cycle
    if (result == 1)
    {
        return;
    }

    // mark the node's name
    if (node->name != NULL)
    {
        mark_one(node->name);
    }

    // traverse through all the links
    Link *curr_link = node->links;
    while (curr_link != NULL)
    {
        mark_one(curr_link);

        // recursively mark the subtree the link points to
        if (curr_link->fptr != NULL)
        {
            mark_fstree(curr_link->fptr);
        }

        curr_link = curr_link->next;
    }
}
