
#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"

#define MAX_QUEUE_SIZE 10

struct queue_t {
    struct pcb_t *proc[MAX_QUEUE_SIZE]; // Array of pointers to processes
    int size;                         // Current number of processes in the queue
    int capacity;                     // Maximum number of processes the queue can hold
};


void enqueue(struct queue_t * q, struct pcb_t * proc);

struct pcb_t * dequeue(struct queue_t * q);

int empty(struct queue_t * q);

#endif

