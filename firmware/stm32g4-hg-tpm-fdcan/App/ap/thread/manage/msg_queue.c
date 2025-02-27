#include "msg_queue.h"

#define MSG_QUEUE_SIZE 32

typedef struct {
    queue_msg_t buf[MSG_QUEUE_SIZE];
    uint32_t head;
    uint32_t tail;
    uint32_t len;
} msg_queue_t;

static msg_queue_t msg_queue;

bool msgQueueInit(void)
{
    msg_queue.head = 0;
    msg_queue.tail = 0;
    msg_queue.len = 0;
    return true;
}

bool msgQueuePut(queue_msg_t *msg)
{
    if (msg_queue.len >= MSG_QUEUE_SIZE)
        return false;

    memcpy(&msg_queue.buf[msg_queue.tail], msg, sizeof(queue_msg_t));
    msg_queue.tail = (msg_queue.tail + 1) % MSG_QUEUE_SIZE;
    msg_queue.len++;
    return true;
}

bool msgQueueGet(queue_msg_t *msg)
{
    if (msg_queue.len == 0)
        return false;

    memcpy(msg, &msg_queue.buf[msg_queue.head], sizeof(queue_msg_t));
    msg_queue.head = (msg_queue.head + 1) % MSG_QUEUE_SIZE;
    msg_queue.len--;
    return true;
}