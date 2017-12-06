/*
g++ -o cord cordinator.cpp -lpthread
./cord 5342 3456 2334 5423
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <iostream>

#define MAXRECORDS 10000
#define MAXCLIENTS 200
#define BACKSERVERS 3
#define MAXLOGS 10000
#define FILENAME "Records.txt"
#define MAXTRANSACTIONS 1000000
#define MAX_CLIENTS 200
#define CREATE 0
#define QUERY 1
#define UPDATE 2
#define QUIT 3
#define OK 0
#define ERR 1


//Server Account Records
struct acc_record
{
	int id;
	float bal;
};

struct accounts
{
	struct acc_record account[10000];
	int total_accounts;
};

struct command
{
	int tsn_id;
	int type;
	int id;
	int bal;
};

struct reply
{
	int type;
	int val;
};

struct log_table
{
	struct command logs[MAXLOGS];
	int index;
	pthread_mutex_t log_lock;
};

//Thread Data
struct thrd_data
{
	int csock;
	struct log_table* log;
}thrd_data;


void printLogs(struct log_table *log)
{
	int i;
	for(i=0;i<log->index;i++)
	{
		printf("\n%d %d %d %d",log->logs[i].tsn_id,log->logs[i].type,log->logs[i].id,log->logs[i].bal);
	}
	printf("\n");
}

/*
Function to perform all transaction of single client. After accepting the connection from each client the Server creates a thread for the clien connection and assigns this function to it to perform all transaction of that client. 
*/
void * perform_cli_tsn(void *arg)
{
	struct command cmd;
	struct reply response;
	struct thrd_data *td = (struct thrd_data *)arg;
	struct log_table* log;
	int i = 0;
	int byte_written,byte_read;
	/* Thread Operation*/	
	log = td->log;
	i=0;
	while(1)
	{
		byte_read = read(td->csock, &cmd, sizeof(cmd));
		if(cmd.type == QUIT)
		{
			log->logs[i].type = cmd.type;
			log->index = log->index + 1;
			break;
		}
		log->logs[i].type = cmd.type;
		log->logs[i].bal = cmd.bal;
		log->logs[i].id = cmd.id;
		log->logs[i].tsn_id = i+1;
		log->index = log->index + 1;
		
		response.type = OK;
		response.val = 1234;
		byte_written = write(td->csock,&response,sizeof(response));
		i++;
	}

	printLogs(td->log);

	//All Transactions from a Client are performed, so closing the connection.
	close(td->csock);
	//printRecords(td->acc);
}

/*
int connectToServer(char ip[], int port)
{

	struct sockaddr_in serv_addr;
	struct hostent *server;
	int sock;

	sock = socket(AF_INET,SOCK_STREAM,0);

	//Prepare Server Structure to Connect
	server = gethostbyname(ip);
	memset(&serv_addr,'0',sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	bcopy((char *)server->h_addr,(char *)&serv_addr.sin_addr.s_addr,server->h_length);

	//Connecct to Server
	if(connect(cli_sock,(struct sockaddr *)&serv_addr,sizeof(serv_addr))<0)
	{
		printf("\nError in Client Connection");
		return -1;
	}

	return sock;
}
*/


/*
Main fuction to start the Server.
*/
int main(int argc,char *argv[])
{
	struct thrd_data td[MAX_CLIENTS];
	struct log_table log;
	int lsock,csock[MAX_CLIENTS];
	int ssock[BACKSERVERS];
	socklen_t clilen;
	struct sockaddr_in server_addr,cli_addr;
	int port,i,no_of_clients,byte_written;
	log.index = 0;
	pthread_t thrd;
	pthread_t interest_thrd;
	setbuf(stdout,NULL);

/*
	//Connect to backend Servers
	for(i=0;i<BACKSERVERS;i++)
	{
		tmp = connectToServer("127.0.0.1",atoi(argv[i+2]));
		if(tmp == -1)
		{
			printf("\nUnable to Connect to Backend Server %d",i+1);
			exit(1);
		}
		ssock[i] = tmp;
	}
*/
	//Create Listen Socket for Clients
	if((lsock = socket(AF_INET,SOCK_STREAM,0)) == -1)
	{
		printf("\nUnable to get server socket");
		return -1;
	}

	port = atoi(argv[1]);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);

	if(bind(lsock,(struct sockaddr *)&server_addr,sizeof(server_addr)))
	{
		printf("\nUnable to Bind");
	}

	listen(lsock,10);
	clilen = sizeof(cli_addr);
	no_of_clients = 0;
	do
	{
	
	//Accept Connections from Client
		csock[no_of_clients] = accept(lsock,(struct sockaddr *)&cli_addr,&clilen);
		if (csock[no_of_clients] < 0) 
        	{
			printf("\nClient accept request denied!!");
			continue;
		}
		
		//Prepare thread data and create New Thread
		td[no_of_clients].log = &log;
		td[no_of_clients].csock = csock[no_of_clients];
		pthread_create(&thrd,NULL,perform_cli_tsn,(void*)&td[no_of_clients]);
		no_of_clients = (no_of_clients + 1)%200 ;
	}while(1);

	close(lsock);

	return 0;
}
