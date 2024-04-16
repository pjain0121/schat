

#include <stdio.h>


#include <monitor.h>


/* INITIALIZATION CODE */
#define OKtoWrite 1
#define OKtoRead  0
#define numConds  2


static int numReaders = 0;
static int busyWriting =0;

/* MONITOR PROCEDURES */
void Initialize(void)
{
      MonInit(numConds);
}

void StartRead()
{
  MonEnter();

  /* if there is a writer */
  if (busyWriting)
    {
      MonWait(OKtoRead);
    }
  /* once we are signaled, we can continue.
   * There will be at least one reader, so we can be assured
   * that no writers will get to go. No other readers will get
   * to go if we do not signal the rest of the readers, so each
   * reader that wakes up does a signal on OKtoRead. This will be OK
   * since the last signal will have no effect on a late-coming
   * reader.
   */
  numReaders++;
  printf("starting value of numReaders = %d\n", numReaders);
  
  MonSignal(OKtoRead);
  
  MonLeave();

}

void StopRead()
{
  MonEnter();
  numReaders--;
  printf("stopping value of numReaders = %d\n", numReaders);
  /* Only signal the writer when all readers have gone */
  if (numReaders == 0) MonSignal(OKtoWrite);
  MonLeave();
  
  
}

void StartWrite()
{
  MonEnter(); 

  /* if there are any readers or a writer */
  if ((numReaders !=0) || busyWriting)
    MonWait(OKtoWrite);
  busyWriting = 1;
  MonLeave();
}

void StopWrite()
{
  MonEnter();

  busyWriting = 0;

  /* wake up any waiting readers */
  /* this gives priority to readers */
  /* could be a signal to the next writer, if there is one, but
   * then we'd need a mechanism to eventually signal a reader.
   * We couldn't signal a writer and then immediately signal a reader.
  */
  MonSignal(OKtoRead);

  MonLeave();

}
