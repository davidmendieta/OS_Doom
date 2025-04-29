#pragma once
#include "token_node.h"

typedef struct Queue {
    node_t* front;
    node_t* rear;
} queue_t;

// Initialize an empty queue
void init_queue(queue_t* q);

// Enqueue a node
void enqueue(queue_t* q, node_t* new_node);

// Dequeue a node
node_t* dequeue(queue_t* q);

// Check if queue is empty
bool is_empty(queue_t* q);

// Peek front without removing
node_t* peek(queue_t* q);

// Free the queue (does not free the nodes themselves)
void free_queue(queue_t* q);
