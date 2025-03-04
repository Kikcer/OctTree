#include "stdlib.h"
#include "stdio.h"
#include "stdbool.h"
#include "circularQueue.h"
#include "crossSystem_tool.h"
void initQueue(Queue_t *q) {
    q->front = 0;
    q->tail = 0;
    q->len = 0;
}

void push(Queue_t *queue, short data){
    if(isQueueFull(queue)){
        printF("Queue is full!\n");
        return;
    }
    queue->data[queue->tail] = data;
    queue->tail = (queue->tail + 1) % MAX_QUEUE_SIZE;
    ++queue->len;
}

short pop(Queue_t *queue){
    if(isQueueEmpty(queue)){
        printF("Queue is empty!\n");
        return -1;
    }
    short data = queue->data[queue->front];
    queue->front = (queue->front + 1) % MAX_QUEUE_SIZE;
    --queue->len;
    return data;
}

bool isQueueEmpty(Queue_t *queue){
    // return queue->front == queue->tail;
    return queue->len == 0;
}

bool isQueueFull(Queue_t *queue){
    // return (queue->tail + 1) % MAX_QUEUE_SIZE == queue->front;
    return queue->len == MAX_QUEUE_SIZE;
}
