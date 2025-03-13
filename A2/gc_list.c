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
    List *curr = (List *)head;

    while (curr != NULL)
    {
        // Mark the current node as in use
        mark_one(curr);

        // Continue to the next node
        curr = curr->next;
    }
}