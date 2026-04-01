/* 
 * Operating Systems  (2INC0)  Practical Assignment.
 * Condition Variables Application.
 *
 * Defne Uyguc (2078007)
 * Dora Er (2074478)
 * Tamer Yurtsever (2081350)
 *
 * Grading:
 * Students who hand in clean code that fully satisfies the minimum requirements will get an 8. 
 * Extra steps can lead to higher marks because we want students to take the initiative.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>  
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>

#include "prodcons.h"

static ITEM buffer[BUFFER_SIZE];

static void rsleep (int t);        
static ITEM get_next_item (void);  

// one mutex for all shared stuff
static pthread_mutex_t mutex       = PTHREAD_MUTEX_INITIALIZER;

// consumer waits here if buffer is empty
static pthread_cond_t  cv_consumer = PTHREAD_COND_INITIALIZER;

// producers wait here if buffer full or not their turn
static pthread_cond_t  cv_producer = PTHREAD_COND_INITIALIZER;

// circular buffer bookkeeping
static int  buf_head  = 0;   // write index
static int  buf_tail  = 0;   // read index
static int  buf_count = 0;   // number of items in buffer

// enforce ascending order
static ITEM next_produce = 0;

// used to stop consumer
static int items_consumed = 0;

/* producer thread */
static void * 
producer (void * arg)
{
    (void) arg;

    while (true /* TODO: not all items produced */)
    {
        // TODO: 
        // * get the new item
        // just grabbing next job
        ITEM item = get_next_item();

        // stop when no more items
        if (item == NROF_ITEMS)
        {
            break;
        }
		
        rsleep (100);   // simulating all kind of activities...
		
        // TODO:
        // * put the item into buffer[]
        //
        // follow this pseudocode:
        //      mutex-lock;
        //      while not condition
        //          wait-cv;
        //      critical-section;
        //      signal;
        //      mutex-unlock;

        pthread_mutex_lock (&mutex);

        // wait if buffer full OR it's not this item's turn yet
        while (buf_count == BUFFER_SIZE || item != next_produce)
        {
            pthread_cond_wait (&cv_producer, &mutex);
        }

        // critical section: insert item into circular buffer
        buffer[buf_head] = item;
        buf_head = (buf_head + 1) % BUFFER_SIZE;
        buf_count++;

        // update what item should come next
        next_produce++;

        // now buffer is not empty → wake consumer
        pthread_cond_signal (&cv_consumer);

        // wake all producers because next_produce changed
        pthread_cond_broadcast (&cv_producer);

        pthread_mutex_unlock (&mutex);
    }
    return (NULL);
}


/* consumer thread */
static void * 
consumer (void * arg)
{
    (void) arg;

    while (true /* TODO: not all items retrieved from buffer[] */)
    {
        // TODO: 
        // * get the next item from buffer[]
        // * print the number to stdout

        pthread_mutex_lock (&mutex);

        // wait if buffer empty (but not finished yet)
        while (buf_count == 0 && items_consumed < NROF_ITEMS)
        {
            pthread_cond_wait (&cv_consumer, &mutex);
        }

        // exit condition
        if (items_consumed == NROF_ITEMS)
        {
            pthread_mutex_unlock (&mutex);
            break;
        }

        // critical section: remove item
        ITEM item = buffer[buf_tail];
        buf_tail = (buf_tail + 1) % BUFFER_SIZE;
        buf_count--;
        items_consumed++;

        // free space available → producers can try again
        pthread_cond_broadcast (&cv_producer);

        pthread_mutex_unlock (&mutex);

        // required output
        printf ("%d\n", item);

        rsleep (100);   // simulating all kind of activities...
    }
    return (NULL);
}


int main (void)
{
    // TODO: 
    // * startup the producer threads and the consumer thread
    // * wait until all threads are finished

    pthread_t producers[NROF_PRODUCERS];
    pthread_t cons;
    int producer_ids[NROF_PRODUCERS];

    // start consumer first
    pthread_create (&cons, NULL, consumer, NULL);

    // start all producers
    for (int i = 0; i < NROF_PRODUCERS; i++)
    {
        producer_ids[i] = i;
        pthread_create (&producers[i], NULL, producer, &producer_ids[i]);
    }

    // wait for producers
    for (int i = 0; i < NROF_PRODUCERS; i++)
    {
        pthread_join (producers[i], NULL);
    }

    // wake consumer in case it is stuck waiting at the end
    pthread_mutex_lock (&mutex);
    pthread_cond_signal (&cv_consumer);
    pthread_mutex_unlock (&mutex);

    // wait for consumer
    pthread_join (cons, NULL);

    return (0);
}