#include <stdbool.h>
#include <stddef.h>
#include "token_node.h"
#include "queue.h"


void init_queue(queue_t* q) {
    q->front = NULL;
    q->rear = NULL;
}

void enqueue(queue_t* q, node_t* new_node) {
    new_node->next = NULL;
    if (q->rear == NULL) {
        q->front = new_node;
        q->rear = new_node;
    } else {
        q->rear->next = new_node;
        q->rear = new_node;
    }
}

node_t* dequeue(queue_t* q) {
    if (q->front == NULL) return NULL;

    node_t* temp = q->front;
    q->front = q->front->next;

    if (q->front == NULL) {
        q->rear = NULL;
    }

    temp->next = NULL;
    return temp;
}

bool is_empty(queue_t* q) {
    return q->front == NULL;
}

node_t* peek(queue_t* q) {
    return q->front;
}

void free_queue(queue_t* q) {
    q->front = NULL;
    q->rear = NULL;
}
