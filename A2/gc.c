#include <stdio.h>
#include <stdlib.h>
#include "gc.h"
#ifndef LOGFILE
#define LOGFILE "gc.log"
#endif

// global variable to be head of allocated pieces
struct mem_chunk *memory_list_head = NULL;

// global variable for debugging
int debug = 0;

void *gc_malloc(int nbytes)
{
    // we first allocate the correct amount of memory
    void *ptr = malloc(nbytes);
    if (ptr == NULL)
    {
        return NULL; // return null if malloc fails
    }

    // create a new memory chunk struct to track the memory allocation
    struct mem_chunk *chunk = malloc(sizeof(struct mem_chunk));
    if (chunk == NULL)
    {
        free(ptr); // if malloc fails, free the allocated memory
        return NULL;
    }

    // initialize the memory chunk
    chunk->address = ptr;
    chunk->in_use = 1; // chunks start as in use
    chunk->next = NULL;

    // add chunk to the memory list
    if (memory_list_head == NULL)
    {
        memory_list_head = chunk;
    }
    else
    {
        // Add memory chunk to the front of the list
        chunk->next = memory_list_head;
        memory_list_head = chunk;
    }

    return ptr;
}
/* Executes the garbage collector.
 * obj is the root of the data structure to be traversed
 * mark_obj is a function that will traverse the data structure rooted at obj
 *
 * The function will also write to the LOGFILE the results messages with the
 * following format strings:
 * "Mark_and_sweep running\n"
 * "Chunks freed this pass: %d\n"
 * "Chunks still allocated: %d\n"
 */

void mark_and_sweep(void *obj, void (*mark_obj)(void *))
{
    FILE *logfile;
    int chunks_freed = 0;
    int chunks_still_allocated = 0;

    logfile = fopen(LOGFILE, "a");
    if (logfile == NULL)
    {
        perror("Error opening log file");
        return;
    }

    fprintf(logfile, "Mark_and_sweep running\n");

    // first we reset all the in use flags to 0, meaning not in use
    struct mem_chunk *curr = memory_list_head;
    while (curr != NULL)
    {
        curr->in_use = 0;
        curr = curr->next;
    }

    // then we traverse the data structure and mark reachable nodes
    if (obj != NULL)
    {
        mark_obj(obj);
    }

    // finally we free unreachable nodes in memory
    struct mem_chunk *prev = NULL;
    curr = memory_list_head;

    while (curr != NULL)
    {
        if (!curr->in_use)
        {
            // if the chunk is not in use, we can free it from memory
            struct mem_chunk *to_free = curr;

            if (prev == NULL)
            {
                // case one: head of the list
                memory_list_head = curr->next;
                curr = memory_list_head;
            }
            else
            {
                // case two: not the head of the list
                prev->next = curr->next;
                curr = curr->next;
            }

            // free the adress of the chunk and the chunk itself
            free(to_free->address);
            free(to_free);

            chunks_freed++;
        }
        else
        {
            // if the chunk is still in use, keep it in memory
            chunks_still_allocated++;
            prev = curr;
            curr = curr->next;
        }
    }

    // lastly we log the results in the logfile
    fprintf(logfile, "Chunks freed this pass: %d\n", chunks_freed);
    fprintf(logfile, "Chunks still allocated: %d\n", chunks_still_allocated);

    fclose(logfile);
}

/*
 Mark the chunk of memory pointed to by vptr as in use.
 Return codes:
   0 if successful
   1 if chunk already marked as in use
   2 if chunk is not found in memory list

   Here is a print statement to print an error message:
   fprintf(stderr, "ERROR: mark_one address not in memory list\n");
 */
int mark_one(void *vptr)
{
    struct mem_chunk *curr = memory_list_head;

    while (curr != NULL)
    {
        if (curr->address == vptr)
        {
            // found the chunk to be marked
            if (curr->in_use)
            {
                return 1; // if it's already marked as in use, return 1 for cycle detection
            }
            else
            {
                curr->in_use = 1; // mark chunk as in use
                return 0;         // return value 0 means that we successfuly marked an unmarked chunk
            }
        }
        curr = curr->next;
    }

    // if chunk isn't found in memory, return an error to stderr
    fprintf(stderr, "ERROR: mark_one address not in memory list\n");
    return 2;
}

void print_memory_list()
{
    struct mem_chunk *current = memory_list_head;
    while (current != NULL)
    {
        printf("%lx (%d) -> ", (unsigned long)current->address, current->in_use);
        current = current->next;
    }
    printf("\n");
}
