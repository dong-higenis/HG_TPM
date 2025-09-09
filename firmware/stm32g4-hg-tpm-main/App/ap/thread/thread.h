#ifndef THREAD_H_
#define THREAD_H_


#include "ap_def.h"



typedef struct thread_t_
{
  const char *name; // const char name[16];
  bool is_enable;

  bool (*init)(void);
  bool (*update)(void);
} thread_t;


bool threadInit(void);
bool threadUpdate(void);

#endif