#include <stdio.h>
#include "list.h"
#include "gc.h"

/* traverse the list data-structure and call mark_one(void *vptr) on
 *  each piece of memory that is still used in the list
 * head is the head of the list data-structure being traversed NOT
 * the head of the memory-list which is part of gc
 */
void mark_list(void *head)
{
    // TODO complete this function
    // struct node *curr = head;
    List *curr = (List *)head;

    while (curr != NULL)
    {
        // First mark the current node itself
        int result = mark_one(curr);

        // Add debugging to check the result
        if (result != 0 && result != 1)
        {
            fprintf(stderr, "Error marking node at %p: result=%d\n", curr, result);
        }

        // Move to next node
        curr = curr->next;
    }
}
