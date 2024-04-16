
#include <stdio.h>

#include <rtthreads.h>
#include <RttCommon.h>

#include <reader-writer-monitor.h>

#define SLEEPMAX 20
#define STKSIZE 65536

RTTTHREAD reader(void *arg)
{
  long myId;
  myId = (long) arg;
  
  for(;;)
    {
      printf("%ld start read\n", myId);
      StartRead();
      printf("%ld Reading\n", myId);
      RttSleep((int) (rand() % SLEEPMAX));
      StopRead();
      printf("%ld stop read\n", myId);
      RttSleep((int) (rand() % SLEEPMAX));
    }
}

RTTTHREAD writer(void *arg)
{
  long myId;
  myId = (long) arg;
  
  for(;;)
    {
      printf("%ld start write\n", myId);
      StartWrite();
      printf("%ld writing\n", myId);
      RttSleep((int) (rand() % SLEEPMAX*5));
      StopWrite();
      printf("%ld stop write\n", myId);
      RttSleep((int) (rand() % SLEEPMAX*6));
    }
}

int mainp()
{
  int temp, temp2, temp3;
  RttSchAttr 	attr;
  RttThreadId	reader1Pid, reader2Pid, reader3Pid;
  RttThreadId	writer1Pid, writer2Pid;

  attr.startingtime = RTTZEROTIME;
  attr.priority = RTTNORM;
  attr.deadline = RTTNODEADLINE;

  setbuf(stdout, 0);

  srand(71);
  Initialize();
  temp = RttCreate(&reader1Pid, (void(*)()) reader,STKSIZE,"R1", (void *) 1000, 
		      attr, RTTUSR );
  if (temp == RTTFAILED) perror("RttCreate");

  temp2 = RttCreate(&writer1Pid, (void(*)()) writer,STKSIZE,"W1", (void *) 500,
		    attr, RTTUSR );
  if (temp2 == RTTFAILED) perror("RttCreate");
  temp3 = RttCreate(&reader2Pid, (void(*)()) reader,STKSIZE,"R2", (void *) 1001,
		       attr, RTTUSR );
  if (temp3 == RTTFAILED) perror("RttCreate");
  temp3 = RttCreate(&reader3Pid, (void(*)()) reader,STKSIZE,"R3", (void *) 1002,
		    attr, RTTUSR );
  if (temp3 == RTTFAILED) perror("RttCreate");
  temp3 = RttCreate(&writer2Pid, (void(*)()) writer,STKSIZE,"W2", (void *) 501, 
		    attr, RTTUSR );
  if (temp3 == RTTFAILED) perror("RttCreate");
  
  printf("Reader and Writer processes created\n");
    
  return(0);
}
