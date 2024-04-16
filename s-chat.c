
#include <stdio.h>                                                              
#include <stdlib.h>                                                                                                                          
#include <os.h>                                                                 
#include <standards.h>                                                          
#include <stdbool.h>                                                            
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <list.h>
#include <sys/time.h>
#include <time.h>
char *receiverPort;
char *senderPort;


/*
#define SERVERPORT1 "30001"
#define SERVERPORT2 "40000"*/
/*TODO: freeaddrinfo*/
/*to store our global sender, writer id's and their busy flags*/
PID senderId;
PID serverId;
PID writerId;
PID receiverId;
int senderBusy;
int writerBusy;

enum rType {
	READER,
	WRITER,
	SENDER,
	RECEIVER,
	TERMINATE
};
typedef struct timeMessage
{
	char userMessage[1024];
	size_t messageSize;
	int32_t sec;
	int32_t usec;
}TimeMessage;

typedef struct Message{
	enum rType type;
	char *input;	
}message;
PROCESS writer()
{
	/*to store the reply message*/
	void *replyMsg;

	/*message to send to server*/
	message data;

	/*to store len of message*/
	int len;

	/*to store writer fd set*/
	fd_set wfds;

	/*timeout struct*/
	struct timeval tv;

	/*to check the return value from select*/
	int retval;
	
	/*to check what write returns*/
	ssize_t writeret;


	/*store writer's id*/
	writerId = MyPid();

	/*set data's type to writer to indicate it is from a writer*/
	data.type = WRITER;
	
	/*set the len to size of message*/
	len = sizeof(message);


	for(;;)
	{		
		/*Do a send to server and wait for it's reply*/
		replyMsg = Send(serverId, &data, &len);
		if((*(int *)replyMsg) == NOSUCHPROC)
		{
			perror("send() to server failed\n");

			/*start loop again*/
			continue;
		}

		/*set writerbusy to true when the server replies*/
		writerBusy = 1;


		/*to make it non-blocking use select() because it tells us
		 * when std output is ready and we can do a write without blocking*/

		/*using another infinite loop here to keep calling select() until
		 * the stdout is ready*/

		/*clear wfd*/
		FD_ZERO(&wfds);

		/*add std out's fd to wfds*/
		FD_SET(STDOUT_FILENO, &wfds);

		for(;;)
		{
			/*set timeout values to 0*/
			tv.tv_sec = 0;

			tv.tv_usec = 0;

			/*call select()*/
			retval = select(STDOUT_FILENO+1, NULL, &wfds, NULL, &tv);



			if(retval == -1)
			{
				perror("select() returned -1 while checking for std_out\n");
			}

			else if(retval)
			{
				/*we are ready to write*/	


				/*write the message replied by the server on std_out*/
			
				if((writeret = write(STDOUT_FILENO, (char *)replyMsg, 
				strlen(replyMsg))) == -1)
				{
					perror("write() error\n");
				}

				
				/*break the write loop*/
				break;
			}

			else
			{
				/*just continue*/
				continue;
			}

		}

			/*free the memory*/
			free(replyMsg);
	}

}

PROCESS UDPSender(void *sendAddress)
{
	/*to store the length of sent message*/
	int len;


	/*to store the reply message*/
	void *replyMsg;

	/*to store socket stuff*/
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	/*to store result*/
	int rv;
	int numbytes;


	/*store id in senderId first*/
	senderId = MyPid();	

	/*initialize hints to 0*/
	memset(&hints, 0, sizeof hints);
	
	hints.ai_family = AF_UNSPEC; /* set to AF_INET to use IPv4*/
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo((char *)sendAddress, senderPort, &hints, 
	&servinfo)) !=0) 
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	}

	/*loop through all the results and make a socket*/
	for(p = servinfo; p != NULL; p = p->ai_next) 
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) 
		{
			perror("talker: socket");
			continue;
		}

		break;
	}

	if (p == NULL) 
	{
		fprintf(stderr, "talker: failed to create socket\n");
	}
	
	for(;;)
	{
		/*forever loop*/

		/*do a send to the server and wait until server replies*/
		/*will have to send a struct message*/
		message senderMessage;

		/*to store time*/
		TimeMessage stm;

		struct timeval tv;

		senderMessage.type = SENDER;

		/*store the size of struct message*/
		len = sizeof(message);

		replyMsg = Send(serverId, &senderMessage, &len);

		/*set sender to busy*/
		senderBusy = 1;


		/*when the server replies*/

		/*get the current time*/
		gettimeofday(&tv, NULL);

		stm.messageSize = strlen(replyMsg);

		/*copy the message to stm's userMessage*/
		/*strcpy(stm.userMessage, m);*/
		strcpy(stm.userMessage, replyMsg);

		/*convert the bytes from host to network byte order*/
		stm.messageSize = htonl(stm.messageSize);

		stm.sec = tv.tv_sec;

		stm.sec = htonl(stm.sec);

		stm.usec = tv.tv_usec;

		stm.usec = htonl(stm.usec);

		/*send the data to the receiver*/
		if ((numbytes = sendto(sockfd, &stm, sizeof(TimeMessage), 0,
				 p->ai_addr, p->ai_addrlen)) == -1) 
		{
			perror("talker: sendto");
		}

		/*free replyMess*/
		free(replyMsg);

		/*if we were able to send the message*/
	}
	freeaddrinfo(servinfo);
	close(sockfd);

}

PROCESS UDPReceiver()
{
	/*we will get our struct addrinfor ready first and then wait for message
	 * inside the loop*/

	/*our structs for getaddrinfo, list and loop*/
	struct addrinfo hints, *servinfo, *p;

	/*to store the result returned by getaddrinfo()*/
	int rv;

	int len;

	long receivedSeconds;

	socklen_t addr_len;

	message data;

	/*to store time returned by asctime()*/
	char *timeBuf;

	struct tm *newtime;

	int sockfd;
	struct sockaddr_storage their_addr;
	int numbytes;

	/*memset hints*/
	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;	/*use IPv4 or IPv6*/
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;


	if((rv = getaddrinfo(NULL, receiverPort, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));	
	}
	
	/*loop through all the results and bind to the first we can*/
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}

		break;
	}


	if (p == NULL) {
		perror("listener: failed to bind socket\n");
	}
	
	for(;;)
	{
		/*malloc a struct and receive a TimeMessage into it*/
		TimeMessage stm;	
		

		len = sizeof(message);
		addr_len = sizeof their_addr;
		memset(&stm, 0, sizeof(stm));
		
		if ((numbytes = recvfrom(sockfd, &stm, sizeof(TimeMessage) , 
		MSG_DONTWAIT, (struct sockaddr *)&their_addr, &addr_len)) == -1) 
		{	
			if(errno == EAGAIN || errno == EWOULDBLOCK){
				/*no data available loop again*/
				continue;
			}
		}
		if (numbytes > 0){
			/*when a message comes, send it to the server*/
			data.type = RECEIVER;

			

			/*convert back to host order*/
			stm.messageSize = ntohl(stm.messageSize);

			receivedSeconds = ntohl(stm.sec);

			stm.usec = ntohl(stm.usec);

			/*allocalte size of message + 37 extra bytes for timestamp*/
			data.input = calloc(stm.messageSize+37, sizeof(char));

			memcpy(data.input, stm.userMessage, stm.messageSize);

			strcat(data.input, "Timestamp: ");

			newtime = localtime(&receivedSeconds);

			timeBuf = asctime(newtime);

			strcat(data.input, timeBuf);

			Send(serverId, &data, &len);
		}

	}

	freeaddrinfo(servinfo);

	close(sockfd);

}
PROCESS keyboardReader(){
	/*a structure that contains the file descriptors*/
	fd_set readfds;
	struct timeval timeout;
	int result;
	/*to put the read string to compare*/
	char cmp[14];
	message data;

	timeout.tv_sec = 0;
	timeout.tv_usec = 0;


	/*set timer of select to 0 to make select return immediately making read
	 * non-blocking*/
	for(;;){
		/*Initializes the file descriptor set to contain no file descriptors
		 * for select to repopulate them*/
		FD_ZERO(&readfds);
		
		/**This function adds a file descriptor to a file descriptor set.*/
		FD_SET(STDIN_FILENO, &readfds);

		result = select(STDIN_FILENO +1, &readfds, NULL, NULL, &timeout);
	/*	int original_flags = fcntl(STDIN_FILENO, F_GETFL, 0);*/
        if (result == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }
  		else if (result == 0) {
            /*No data available for reading, continue or do other work*/
        }
		else {
			/*Checks to see if the STDIN_FILENO is part o set after select 
			 * returns*/
            if (FD_ISSET(STDIN_FILENO, &readfds)) {
                char buffer[1024];
				ssize_t bytesRead; 

				/*Initialize the buffer to all zeroes*/
				memset(buffer, 0, sizeof(buffer));
				bytesRead = read(STDIN_FILENO, buffer, sizeof(buffer));

                if (bytesRead > 0) {
					buffer[bytesRead] = '\0';	
					/*see if it's our special string*/
					if(buffer[0] == ':')
					{
						memcpy(cmp, buffer, 13);
						if(strcmp(cmp, ":end-session!") == 0)
						{
							/*notify the server to stop*/
							data.type = TERMINATE;
							Send(serverId, &data, 0);
							/*exit when the server replies*/
							Pexit();
						}
					} 
					data.type = READER;
					data.input = calloc(bytesRead+1, sizeof(char));
					strcpy(data.input, buffer);
					Send(serverId, &data, 0);
				

                } else if (bytesRead == 0) {

                    /* End of input*/

                    break;
                } else {
                    perror("read");
                    exit(EXIT_FAILURE);
                }
            }

        }
	
	}
}

PROCESS server()
{
	/*to point to message sent*/
	message *recMessage;
	
	/*to store the PID of sender and length of message*/
	PID senderPid;

	int len;

	/*to store the message we are sending, i.e. items of lists*/
	char *listMess;

	/*to store return value of reply*/
	int replyRet;

	LIST *senderQueue;
	LIST *writerQueue;

	/*initialize 2 lists for 2 queues*/

	senderQueue = ListCreate();

	writerQueue = ListCreate();

	/*loop forever*/
	for(;;)
	{
		/*do a receive and wait*/
		recMessage = Receive(&senderPid, &len);
	
		/*when a message is received*/
		

		/*check who sent it*/
		switch(recMessage->type)
		{
			
			case READER:

				/*reply to the reader*/
				if((replyRet = Reply(senderPid, NULL, 0)) == -1)
				{
					perror("reply() to reader failed\n");
				}

				/*check if sender not busy and sender queue > 0*/
				if(!senderBusy && (ListCount(senderQueue) > 0))
				{
					if((listMess = ListFirst(senderQueue)) == NULL)
					{
						perror("error in retrieving message from senderQueue\n"
						);
					}

					else if(ListRemove(senderQueue) == NULL)
					{
						perror("error in removing data from senderQueue\n");
					}

					/*reply to sender*/
					if((replyRet = Reply(senderId, listMess, sizeof(listMess
					))) == -1)
					{
						perror("reply() to sender failed\n");
					}
					
					if(ListAppend(senderQueue, recMessage->input) == -1)
					{
						perror("adding message to senderQueue failed\n");
					}
				}

				/*if sender not busy and there are no messages in the queue*/
				else if(!senderBusy && (ListCount(senderQueue)== 0))
				{
					/*reply to sender with current message*/
					if((replyRet = Reply(senderId, recMessage->input, 
					strlen(recMessage->input))) == -1)
					{
						perror("reply() to sender failed\n");
					}
				}
				
				/*else if sender is busy just enqueue the message to senderQ*/
				else
				{
					if(ListAppend(senderQueue, recMessage->input) == -1)
					{
						perror("adding message to senderQueue failed\n");
					}

				}

				break;

			case WRITER:

				if(ListCount(writerQueue) > 0)
				{

					/*get message from the writer queue and reply to writer*/
					if((listMess = ListFirst(writerQueue)) == NULL)
					{
						perror("error in retrieving message from writerQueue\n"
						);
					}

					else if(ListRemove(writerQueue) == NULL)
					{
						perror("error in removing data from writerQueue\n");
					}

					/*reply to the writer  with the message*/
					if((replyRet = Reply(writerId, listMess, sizeof(listMess
					))) == -1)
					{
						perror("reply() to writer failed\n");
					}

					
				}
				else
				{
					/*set writerbusy to false, so that we know a writer is
					 * available*/
					writerBusy = 0;
				}



				break;

			case SENDER:
	
				if(ListCount(senderQueue) > 0)
				{
					/*if we already have stuff we need to send*/

					/*get that from the queue and send it*/

					if((listMess = ListFirst(senderQueue)) == NULL)
					{
						perror("error in removing data from Sender Queue\n");
					}
					/*reply to the sender*/
					else if (ListRemove(senderQueue) == NULL ) 
					{
						perror("error in removing data from Sender Queue\n");
					}
					if((replyRet = Reply(senderId, listMess, sizeof(listMess
					)))== -1)
					{
						perror("reply() to sender failed\n");
					}

				}

				else
				{
					/*set sender to not busy*/
					senderBusy = 0;
				}
				break;

			case RECEIVER:
		
				/*Assuming receiver has the done malloc properly
				 * reply to the receiver*/
				if((replyRet = Reply(senderPid, NULL, 0)) == -1)
				{
					perror("reply() to receiver failed\n");
				}

				if(!writerBusy && (ListCount(writerQueue) > 0))
				{
					if((listMess = ListFirst(writerQueue)) == NULL)
					{
						perror("writerQueue should have the message but it"
						" does not\n");
					}

					else if(ListRemove(writerQueue) == NULL)
					{
						perror("ListRemove() failed on writerQueue\n");
					}

					if((replyRet = Reply(writerId, listMess, sizeof(listMess
					))) == -1)
					{
						perror("reply() to writer failed\n");
					}
					/*If queue is not empty i.e. writer has been replied so 
					 * append the current request*/
					if((ListAppend(writerQueue, recMessage->input)) == -1)
					{
						perror("Appending to the writerQueue failed\n");
					}

				}

				else if(!writerBusy && (ListCount(writerQueue) == 0))
				{
					if((replyRet = Reply(writerId, recMessage->input, 
					sizeof(TimeMessage))) == -1)
					{
						perror("error in replying to writer\n");
					}
				}

				else
				{
					if((ListAppend(writerQueue, recMessage->input)) == -1)
					{
						perror("Appending to the writerQueue failed\n");
					}
				}
				break;


			case TERMINATE:
				/*reply to the terminater*/
				if((replyRet = Reply(senderPid, NULL, 0)) == -1)
				{
					perror("reply() to terminate failed\n");
				}

				/*kill the other 3 threads*/
				if(Kill(writerId) == PNUL)
				{
					perror("Error killing the writer\n");
				}

				if(Kill(senderId) == PNUL)
				{
					perror("Error killing the sender\n");
				}

				if(Kill(receiverId) == PNUL)
				{
					perror("Error killing the receiver\n");
				}

				/*free the queues*/
				ListFree(writerQueue, free);
				ListFree(senderQueue, free);

				/*exit*/
				Pexit();




		}
	}



}
void mainp(int argc, char** argv)
{
	/*check if correct number of arguments are passed*/
	if(argc != 4)
	{
		perror("3 arguments needed: port1, ipv4, port2\n");
		exit(1);
	}

	/*check if correct ports are passed*/
	if(((atoi(argv[1]) < 30001) || (atoi(argv[1]) > 40000)) || 
		((atoi(argv[3]) < 30001) || (atoi(argv[3]) > 40000)))
	{
		perror("correct ports not passed\n");
		exit(1);
	}

	if(atoi(argv[1]) == atoi(argv[3]))
	{
		perror("Need to send and receive on different ports\n");
		exit(1);
	}

	printf("enter \":end-session!\" to exit\n");

	/*set ports*/
	receiverPort = argv[1];
	senderPort = argv[3];

	serverId = Create((void(*)()) server, 16000, "server", NULL, NORM, USR);
	
	Create((void(*)()) keyboardReader, 16000, "reader", NULL, NORM, USR);
	Create((void(*)()) writer, 16000, "writer", NULL, NORM, USR);

	/*create a sender thread*/
	
	Create((void(*)()) UDPSender, 16000, "sender", (void *)argv[2], NORM, USR);

	receiverId = Create((void(*)()) UDPReceiver, 16000, "receiver", NULL, 
			NORM, USR);

}
