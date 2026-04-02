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

#include "prodcons.h"

static ITEM buffer[BUFFER_SIZE];

static void rsleep (int t);         // already implemented (see below)
static ITEM get_next_item (void);   // already implemented (see below)

// one mutex for all shared stuff
static pthread_mutex_t mutex       = PTHREAD_MUTEX_INITIALIZER;

// producers wait here if buffer full or not their turn yet
static pthread_cond_t  cv_producer = PTHREAD_COND_INITIALIZER;

// consumer waits here if buffer is empty
static pthread_cond_t  cv_consumer = PTHREAD_COND_INITIALIZER;

// circular buffer stuff
static int  buf_head       = 0;   // where we write next
static int  buf_tail       = 0;   // where we read next
static int  buf_count      = 0;   // how many items in buffer

// keeps track of what item should be inserted next
static ITEM next_produce   = 0;

// used to know when consumer is done
static int  items_consumed = 0;


/* producer thread */
static void * 
producer (void * arg)
{
    (void) arg;

    while (true /* TODO: not all items produced */)
    {
        // TODO: 
        // * get the new item
        ITEM item = get_next_item();

        // stop when no more items are left
        if (item == NROF_ITEMS)
        {
            break;
        }
		
        rsleep (100);   // simulating all kind of activities...
		
        // TODO:
        // * put the item into buffer[]
        //
        // follow this pseudocode (according to the ConditionSynchronization lecture):
        //      mutex-lock;
        //      while not condition-for-this-producer
        //          wait-cv;
        //      critical-section;
        //      possible-cv-signals;
        //      mutex-unlock;
        //
        // (see condition_test() in condition_basics.c how to use condition variables)

        pthread_mutex_lock (&mutex);

        // wait if buffer is full or not this item's turn yet
        while (item != next_produce || buf_count == BUFFER_SIZE)
        {
            pthread_cond_wait (&cv_producer, &mutex);
        }

        // put item into the circular buffer
        buffer[buf_head] = item;
        buf_head  = (buf_head + 1) % BUFFER_SIZE;
        buf_count++;

        // next item should now be produced
        next_produce++;

        // wake all producers because next_produce changed
        // if we only wake one, it might be the wrong one
        pthread_cond_broadcast (&cv_producer);

        // buffer not empty anymore → wake consumer
        pthread_cond_signal (&cv_consumer);

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
        //
        // follow this pseudocode (according to the ConditionSynchronization lecture):
        //      mutex-lock;
        //      while not condition-for-this-consumer
        //          wait-cv;
        //      critical-section;
        //      possible-cv-signals;
        //      mutex-unlock;

        pthread_mutex_lock (&mutex);

        // stop when everything has been consumed
        if (items_consumed == NROF_ITEMS)
        {
            pthread_mutex_unlock (&mutex);
            break;
        }

        // wait until there is something in the buffer
        while (buf_count == 0)
        {
            pthread_cond_wait (&cv_consumer, &mutex);
        }

        // take next item from the circular buffer
        ITEM item = buffer[buf_tail];
        buf_tail  = (buf_tail + 1) % BUFFER_SIZE;
        buf_count--;
        items_consumed++;

        // one spot got free → producers can try again
        pthread_cond_broadcast (&cv_producer);

        pthread_mutex_unlock (&mutex);

        rsleep (100);   // simulating all kind of activities...

        printf ("%d\n", item);
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

    // start consumer
    pthread_create (&cons, NULL, consumer, NULL);

    // start all producers
    for (int i = 0; i < NROF_PRODUCERS; i++)
    {
        pthread_create (&producers[i], NULL, producer, NULL);
    }

    // wait for all producers
    for (int i = 0; i < NROF_PRODUCERS; i++)
    {
        pthread_join (producers[i], NULL);
    }

    // wait for consumer
    pthread_join (cons, NULL);

    return (0);
}