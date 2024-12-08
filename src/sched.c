#include "queue.h"

#include "sched.h"

#include <pthread.h>

#include <stdlib.h>

#include <stdio.h>



static struct queue_t ready_queue;

static struct queue_t run_queue;

static pthread_mutex_t queue_lock;

static unsigned long slot[140];

#ifdef MLQ_SCHED

static struct queue_t mlq_ready_queue[MAX_PRIO];

int prio_slot[MAX_PRIO];

#endif



int IsEmpty()

{

        for(int i=0;i<MAX_PRIO;i++){

        if(mlq_ready_queue[i].size>0)return 0;

        }

        return 1;

}

void init_slot(){

        for(int i=0;i<MAX_PRIO;i++)

        {

        prio_slot[i]=MAX_PRIO-i;

        }

}



int queue_empty(void) {

#ifdef MLQ_SCHED

    unsigned long prio;

    for (prio = 0; prio < MAX_PRIO; prio++) {

        if (!empty(&mlq_ready_queue[prio])) {

            printf("Queue not empty at priority %lu\n", prio);

            return 0; // Return 0 if at least one queue is not empty

        }

    }

    printf("All queues are empty\n");

    return 1; // All queues are empty

#else

    return (empty(&ready_queue) && empty(&run_queue));

#endif

}



void init_scheduler(void) {

#ifdef MLQ_SCHED

    int i;

    for (i = 0; i < MAX_PRIO; i++) {

        mlq_ready_queue[i].size = 0;

    }

    printf("Scheduler initialized with %d priority levels.\n", MAX_PRIO);

#endif

    ready_queue.size = 0;

    run_queue.size = 0;

    pthread_mutex_init(&queue_lock, NULL);

}



#ifdef MLQ_SCHED

/* 

 *  Retrieve the highest-priority process based on MLQ policy.

 *  Locks the queue while accessing shared resources to prevent race conditions.

 */
struct pcb_t * get_mlq_proc(void)

{

    struct pcb_t *proc = NULL;

	/*TODO: get a process from PRIORITY [ready_queue].

	 * Remember to use lock to protect the queue.

	 * */

	 pthread_mutex_lock(&queue_lock);

	 if(!IsEmpty())

	 for(int i=0;i<MAX_PRIO;i++)

	 {

	       if(mlq_ready_queue[i].size>0&&prio_slot[i]>0)

	       {

	       proc=dequeue(&mlq_ready_queue[i]);

	       prio_slot[i]--;

	       break;

	       }

	       if(i==MAX_PRIO-1)

	       {

	       if(IsEmpty())break;

	       i=-1;

	       init_slot();

	       }

	 }

	 pthread_mutex_unlock(&queue_lock);

	return proc;

}





/* Place a process back into its priority queue */

void put_mlq_proc(struct pcb_t * proc) {

    // pthread_mutex_lock(&queue_lock);



    // // Check if the process is already in the queue to prevent duplicates

    // int found = 0;

    // for (int i = 0; i < mlq_ready_queue[proc->prio].size; i++) {

    //     if (mlq_ready_queue[proc->prio].proc[i] == proc) {

    //         found = 1;

    //         break;

    //     }

    // }



    // if (!found) {

    //   //  printf("Putting process %d with priority %d back into the queue.\n", proc->pid, proc->prio);

    //     enqueue(&mlq_ready_queue[proc->prio], proc);

    // } else {

    //   //  printf("Process %d with priority %d is already in the queue. Skipping enqueue.\n", proc->pid, proc->prio);

    // }



   // pthread_mutex_unlock(&queue_lock);
   pthread_mutex_lock(&queue_lock);
	enqueue(&mlq_ready_queue[proc->prio], proc);
	pthread_mutex_unlock(&queue_lock);

}





/* Add a new process to the correct priority queue */

void add_mlq_proc(struct pcb_t * proc) {

    pthread_mutex_lock(&queue_lock);

    printf("Adding process %d with priority %d to the queue.\n", proc->pid, proc->prio);

    enqueue(&mlq_ready_queue[proc->prio], proc);

    pthread_mutex_unlock(&queue_lock);

}



struct pcb_t * get_proc(void) {

    return get_mlq_proc();

}



void put_proc(struct pcb_t * proc) {

    put_mlq_proc(proc);

}



void add_proc(struct pcb_t * proc) {

    add_mlq_proc(proc);

}



#else

/* Retrieve a process from the ready queue */

struct pcb_t * get_proc(void) {

    struct pcb_t * proc = NULL;



    pthread_mutex_lock(&queue_lock);

    if (!empty(&ready_queue)) {

        proc = dequeue(&ready_queue);

        printf("Retrieved process %d from the ready queue.\n", proc->pid);

    } else {

        printf("Ready queue is empty.\n");

    }

    pthread_mutex_unlock(&queue_lock);



    return proc;

}



/* Place a process back into the run queue */

void put_proc(struct pcb_t * proc) {

    pthread_mutex_lock(&queue_lock);

    printf("Putting process %d back into the run queue.\n", proc->pid);

    enqueue(&run_queue, proc);

    pthread_mutex_unlock(&queue_lock);

}



/* Add a new process to the ready queue */

void add_proc(struct pcb_t * proc) {

    pthread_mutex_lock(&queue_lock);

    printf("Adding process %d to the ready queue.\n", proc->pid);

    enqueue(&ready_queue, proc);

    pthread_mutex_unlock(&queue_lock);

}

#endif

