

#include <rtthreads.h>
#include <list.h>
LIST *enter;
LIST *urgent;
LIST **holder;

/*define 3 semaphores for mutex enterq and urgentq*/
RttSem mutex, enterq, urgentq;

/*will also need an array of semaphores dynamically allocated*/
RttSem *semCV;

/*monBusy*/
int monBusy;

void MonInit(int numCV)
{
	/*loop variable*/
	int i;

	
	/*store the return values of semaphores*/
	int semResult;

	/*We can use RttAllocSem() because we need the mode to be RTTFCFS*/

	/*initialize mutex to 1*/
	semResult = RttNewSem(&mutex, 1);

	/*check the return value*/
	if( semResult != RTTOK)
	{
		perror("mutex initialization failed\n");
	}

	semResult = RttNewSem(&enterq, 0);
	if( semResult != RTTOK)
	{
		perror("enterq initialization failed\n");
	}


	semResult = RttNewSem(&urgentq, 0);
	if( semResult != RTTOK)
	{
		perror("urgentq initialization failed\n");
	}

	/*allocate space for semCV*/
	semCV = (RttSem *)malloc(sizeof(RttSem) * numCV);

	for(i = 0; i < numCV; i++)
	{
		/*initialize semCV*/
		semResult = RttNewSem(&semCV[i], 0);
		if( semResult != RTTOK)
		{
			perror("semCV initialization failed\n");
		}
	}

	/*allocate space for array of Lists*/
	holder = (LIST **) malloc(sizeof(LIST*) * numCV);

	for(i = 0; i <numCV; i++)
	{
		/*initialize holder*/
		holder[i] = ListCreate();
		if( holder == NULL)
		{
			perror("List initialization failed\n");
		}
	
	}

	/*initialize enter and urgent lists*/
	enter = ListCreate();
	if(enter == NULL)
	{
		perror("enter Initialization failed\n");
	}

	urgent = ListCreate();
	if(urgent == NULL)
	{
		perror("enter Initialization failed\n");
	}


	/*set monBusy to 0*/
	monBusy = 0;
	
}

void MonEnter()
{
	/*to check the return value*/
	int result;

	RttThreadId *id;

	/*initialize the mutex*/
	result = RttP(mutex);
	if(result != RTTOK)
	{
		perror("Failed to initialize mutex in MonEnter()\n");
	}

	/*in mutex*/
	if(monBusy)
	{
		/*if the monitor is busy*/

		/*add current threadId to the end of enter list*/

		/*we first need to dynamically allocate memory for it to add in List*/
		id = (RttThreadId *) malloc(sizeof(RttThreadId));

		*id = RttMyThreadId();

		/*append to the enter list*/
		ListAppend(enter, id);

		/*release the mutex*/
		result = RttV(mutex);
		if(result != RTTOK)
		{
			perror("Failed to release mutex MonEnter()\n");
		}

		/*call wait on enterq*/
		result = RttP(enterq);
		if(result != RTTOK)
		{
			perror("Failed to do wait on enterq in MonEnter()\n");
		}

	}
	else
	{
		/*set monBusy to true*/
		monBusy = 1;

		/*unlock mutex*/
		result = RttV(mutex);
		if(result != RTTOK)
		{
			perror("Failed to signal mutex in MonEnter()\n");
		}	
	}

}


void MonLeave()
{
	/*to store results of calling signal to check return value*/
	int result;

	/*check if urgentq list is empty*/
	if(ListCount(urgent) != 0)
	{
		/*set current to first item in the urgent list*/
		if(ListFirst(urgent) == NULL)
		{
			perror("ListFirst failed MonLeave()\n");	
		}

		/*remove the first item*/
		if(ListRemove(urgent) == NULL)
		{
			perror("ListRemove failed MonLeave()\n");	
		}

		/*signal urgentq*/
		result = RttV(urgentq);
		if(result != RTTOK)
		{
			perror("signalling urgentq failed MonLeave()\n");
		}	
	}

	/*check if enterq list is empty*/
	else if (ListCount(enter) != 0)
	{
		/*take an item off of enter list and signal enterq*/

		/*to take first item off of the list we first need to do ListFirst()*/
		if(ListFirst(enter) == NULL)
		{
			perror("ListFirst failed MonLeave()\n");
		}

		/*now we can do listremove*/
		if(ListRemove(enter) == NULL)
		{
			perror("ListRemove failed MonLeave()\n");
		}

		/*signal urgentq*/
		result = RttV(enterq);	
		if(result != RTTOK)
		{
			perror("Failed to signal enterq MonLeave()\n");
		}
	}

	else
	{
		/*set MonBusy to false*/
		monBusy = 0;
	}
}


void MonWait(int cv)
{
	/*to check the return calue*/
	int result;

	/*add current thread's id to cv's list*/
	RttThreadId *currentId;

	currentId = (RttThreadId *)malloc(sizeof(RttThreadId));

	*currentId = RttMyThreadId();

	/*add currentId to urgent queue List*/
	ListAppend(holder[cv], currentId);

	/*check if urgent queue list is non-empty*/
	if(ListCount(urgent) != 0)
	{
		/*take an item off of the urgent queue list*/

		/*first do ListFirst and then ListRemove*/
		if(ListFirst(urgent) != NULL)
		{
			perror("ListFirst failed MonWait()\n");
		}

		if(ListRemove(urgent) != NULL)
		{
			perror("ListRemove failed MonWait()\n");
		}

		/*signal urgentq semaphore*/
		result = RttV(urgentq);
		if(result != RTTOK)
		{
			perror("failed to get waiting thread from urgentq MonWait()\n");
		}

		/*wait on cv's semaphore*/
		result = RttP(semCV[cv]);
		if(result != RTTOK)
		{
			perror("failed to wait on CV MonWait()\n");
		}
	}

	else if(ListCount(enter) != 0)
	{
		/*take an item off of the enter queue list*/

		/*first do ListFirst and then ListRemove*/
		if(ListFirst(enter) != NULL)
		{
			perror("ListFirst failed MonWait()\n");
		}

		if(ListRemove(enter) != NULL)
		{
			perror("ListRemove failed MonWait()\n");
		}

		/*signal enterq semaphore*/
		result = RttV(enterq);
		if(result != RTTOK)
		{
			perror("failed to get waiting thread from urgentq MonWait()\n");
		}

		/*wait on cv's semaphore*/
		result = RttP(semCV[cv]);
		if(result != RTTOK)
		{
			perror("failed to wait on CV MonWait()\n");
		}	
	}

	else
	{
		/*set MonBusy to false*/
		monBusy = 0;

		/*wait on cv*/
		result = RttP(semCV[cv]);
		if(result != RTTOK)
		{
			perror("failed to wait on CV MonWait()\n");
		}
	}
}

void MonSignal(int cv)
{
	/*check if cv's list is non-empty*/
	if(ListCount(holder[cv]) != 0)
	{
		/*to check the return calue*/
		int result;

		/*add current thread's id to urgent list*/
		RttThreadId *currentId;

		currentId = (RttThreadId *)malloc(sizeof(RttThreadId));

		*currentId = RttMyThreadId();	

		/*take item off of cv's list*/

		/*first call ListFirst and then ListRemove*/
		if(ListFirst(holder[cv]) == NULL)
		{
			perror("ListFirst failed in MonSignal()\n");
		}

		if(ListRemove(holder[cv]) == NULL)
		{
			perror("ListRemove failed MonSignal()\n");
		}

		/*add currentId to urgent list*/
		ListAppend(urgent, currentId);

		/*signal cv*/
		result = RttV(semCV[cv]);
		if(result != RTTOK)
		{
			perror("Signalling cv failed MonSignal()\n");
		}

		/*wait on urgent*/
		result = RttP(urgentq);
		if(result != RTTOK)
		{
			perror("waiting on urgentq failed MonSignal()\n");
		}
	}
}
