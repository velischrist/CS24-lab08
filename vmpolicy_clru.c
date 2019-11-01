/* ============================================================================
 * Implementation of the CLOCK/LRU page replacement policy.
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
    /* Previous node */ 
    struct __page_node *prev;
    /* Next node */ 
    struct __page_node *next;
} page_node;

/* "Loaded Pages" Data Structure
 *
 * This data structure records all pages that are currently loaded in the
 * virtual memory, so that we can choose a random page to evict very easily.
 */
typedef struct loaded_pages_t {
    /* The maximum number of pages that can be resident in memory at once. */
    int max_resident;

    /* The number of pages that are currently loaded.  This can initially be
     * less than max_resident.
     */
    int num_loaded;
    
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
    fprintf(stderr, "Using CLOCK/LRU eviction policy.\n\n");
    
    loaded = malloc(sizeof(loaded_pages_t));
    if (loaded) {
        loaded->max_resident = max_resident;
        loaded->head = NULL;
        loaded->tail = NULL;
        loaded->num_loaded = 0;
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
    node->prev = NULL;
    node->next = NULL;

    /* If queue is empty, set head and tail to the new page node */ 
    if(loaded->head == NULL) {
        loaded->head = node;
        loaded->tail = node;
    }

    /* Else, add the node at the back of the queue (we evict the front) */ 
    else {
        node->prev = loaded->tail;
        loaded->tail->next = node;
        loaded->tail = node;
    }

    /* Increment the number of loaded pages */ 
    loaded->num_loaded++;
}


/* This function is called when the virtual memory system has a timer tick.
 * Traverse throught the FIFO queue and move the accessed pages to the back.
 * In this way, the pages that have not been accessed for a while will be
 * evicted (since they will be at the front) */
void policy_timer_tick(void) {
    /* If the queue is empty or has 1 element, do nothing (return) */ 
    if(loaded->num_loaded == 0 || loaded->num_loaded == 1) {
        return;
    }

    /* Start from the head of the list */ 
    page_node *node = loaded->head;

    /* Traverse through the queue, operating only once on each element. 
     * This is why we need a for loop (since we move elements at the back. */ 
    for(int i = 0; i < loaded->num_loaded; i++) {
        page_t page = node->page;
        page_node *next_node = node->next;

        /* If the page has been accessed, move it to the back of the queue */ 
        if(is_page_accessed(page)) { 
            /* Clear its accessed bit */ 
            clear_page_accessed(page);
            /* Update permission to NONE, so that we know if it gets 
             * accessed again */ 
            set_page_permission(page, PAGEPERM_NONE);

            /* If node is the head update the head to be the second node, 
             * so that the node gets removed */
            if(node == loaded->head) {
                node->next->prev = NULL;
                loaded->head = node->next;
            }

            /* If the node is between the head and tail, remove it */ 
            else if(node != loaded->tail) {
                node->prev->next = node->next;
                node->next->prev = node->prev;
            }

            /* Add the node at the back of the queue */ 
            node->prev = loaded->tail;
            node->next = NULL;
            loaded->tail->next = node;
            loaded->tail = node;

        }
        
        /* Go to the next node */ 
        node = next_node;
    }
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

    /* Decrement number of loaded pages */ 
    loaded->num_loaded--;

#if VERBOSE
    fprintf(stderr, "Choosing victim page %u to evict.\n", victim);
#endif

    return victim;
}

