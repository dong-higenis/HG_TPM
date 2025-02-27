#ifndef MSG_QUEUE_H_
#define MSG_QUEUE_H_

#include "def.h"

typedef struct {
    uint32_t msg_id;
    uint8_t data[8];
    uint8_t length;
} queue_msg_t;

bool msgQueueInit(void);
bool msgQueuePut(queue_msg_t *msg);
bool msgQueueGet(queue_msg_t *msg);

#endif