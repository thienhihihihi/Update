#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

int empty(struct queue_t * q) {
    if (q == NULL) return 1;
    return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
    if (q->size < MAX_QUEUE_SIZE) {
        q->proc[q->size] = proc;
       // printf("Enqueuing process %d with priority %d. Queue size before: %d\n", proc->pid, proc->prio, q->size);
        q->size++;
     //   printf("Queue size after enqueue: %d\n", q->size);
    } else {
     //   printf("Queue is full, cannot enqueue process %d with priority %d.\n", proc->pid, proc->prio);
    }

    // Debug: Print queue state after enqueue
   // printf("Queue state after enqueue: ");
    //for (int i = 0; i < q->size; i++) {
      //  printf("(%d, %d) ", q->proc[i]->pid, q->proc[i]->prio);
   // }
  //  printf("\n");
}

struct pcb_t * dequeue(struct queue_t * q) {
    if (empty(q)) {
      //  printf("Queue is empty, cannot dequeue.\n");
        return NULL;
    }

    struct pcb_t * proc = q->proc[0];
  //  printf("Dequeuing process %d with priority %d. Queue size before: %d\n", proc->pid, proc->prio, q->size);

    // Shift all elements to the left to maintain FIFO order
    for (int i = 1; i < q->size; i++) {
        q->proc[i - 1] = q->proc[i];
    }

    q->proc[q->size - 1] = NULL; // Clear the last slot
    q->size--;

 //   printf("Queue size after dequeue: %d\n", q->size);

    // Debug: Print queue state after dequeue
 //   printf("Queue state after dequeue: ");
    for (int i = 0; i < q->size; i++) {
     //   printf("(%d, %d) ", q->proc[i]->pid, q->proc[i]->prio);
    }
  //  printf("\n");

    return proc;
}
