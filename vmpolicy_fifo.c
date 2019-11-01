/* ============================================================================
 * Implementation of the FIFO page replacement policy.
 *
 * We don't mind if paging policies use malloc() and free(), just because it
 * keeps things simpler.  In real life, the pager would use the kernel memory
 * allocator to manage data structures, and it would endeavor to allocate and
 * release as little memory as possible to avoid making the kernel slow and
 * prone to memory fragmentation issues.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "vmpolicy.h"

/* ============================================================================
 * Data Structures
 */

/* "Page Node" Data Structure
 *
 * This data structure reprsents a page as a node in the FIFO queue (see below)
 */
typedef struct __page_node {
    /* Data: the page */ 
    page_t page;
    /* Next node */ 
    struct __page_node *next;
} page_node;


/* "Loaded Pages" Data Structure
 *
 * This data structure records all pages that are currently loaded in the
 * virtual memory as a FIFO queue. We choose to evict the front of the queue
 */
typedef struct loaded_pages_t {
    /* The maximum number of pages that can be resident in memory at once. */
    int max_resident;
    
    /* Head and Tail of the queue */ 
    page_node *head;
    page_node *tail;

} loaded_pages_t;


/* ============================================================================
 * Policy Implementation
 */


/* The list of pages that are currently resident. */
static loaded_pages_t *loaded;


/* Initialize the policy.  Return nonzero for success, 0 for failure. */
int policy_init(int max_resident) {
    fprintf(stderr, "Using FIFO eviction policy.\n\n");
    
    loaded = malloc(sizeof(loaded_pages_t));
    if (loaded) {
        loaded->max_resident = max_resident;
        loaded->head = NULL;
        loaded->tail = NULL;
    }
    
    /* Return nonzero if initialization succeeded. */
    return (loaded != NULL);
}


/* Clean up the data used by the page replacement policy. */
void policy_cleanup(void) {
    free(loaded); 
}


/* This function is called when the virtual memory system maps a page into the
 * virtual address space.  Record that the page is now resident.
 */
void policy_page_mapped(page_t page) {
    /* Allocate memory for a new node for the new page */ 
    page_node *node = (page_node *)malloc(sizeof(page_node));
    if(node == NULL) {
        fprintf(stderr, "Could not allocate a page node!\n");
        exit(0);
    }

    /* Initialize page node */ 
    node->page = page;
    node->next = NULL;

    /* If queue is empty, set head and tail to the new page node */ 
    if(loaded->head == NULL) {
        loaded->head = node;
        loaded->tail = node;
    }

    /* Else, add the node at the back of the queue (we evict the front) */ 
    else {
        loaded->tail->next = node;
        loaded->tail = node;
    }
}


/* This function is called when the virtual memory system has a timer tick. */
void policy_timer_tick(void) {
    /* Do nothing! */
}


/* Choose a page to evict from the collection of mapped pages.  Then, record
 * that it is evicted.  We evict the head of the queue, since it is a FIFO
 * implementation. 
 */
page_t choose_and_evict_victim_page(void) {
    /* Evict first page of the queue */ 
    page_t victim = loaded->head->page;
    page_node *temp = loaded->head;
    loaded->head = loaded->head->next;

    /* Free the page node of the evicted page */ 
    free(temp);

#if VERBOSE
    fprintf(stderr, "Choosing victim page %u to evict.\n", victim);
#endif

    return victim;
}

