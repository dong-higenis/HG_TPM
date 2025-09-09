#include "ap.h"



void apInit(void)
{
  threadInit();
  msgQueueInit();
}

void apMain(void)
{
  while(1)
  {
    threadUpdate();    
  }
}

