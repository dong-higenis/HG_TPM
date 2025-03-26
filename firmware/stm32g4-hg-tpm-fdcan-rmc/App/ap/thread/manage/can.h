#ifndef CAN_THREAD_H_
#define CAN_THREAD_H_


#include "ap_def.h"

typedef struct
{
  bool (*isOpen)(void);
  bool (*getTxUpdate)(void);
  bool (*getRxUpdate)(void);
} can_obj_t;


can_obj_t *canObj(void);
void canHandleTxMessage(can_msg_t *p_msg);
#endif
