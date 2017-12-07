/*
g++ -o server server.cpp -lpthread
./server Records.txt 12340
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
#include <vector>
#include <iostream>

#define MAXRECORDS 10000
#define FILENAME "Records.txt"
#define MAXTRANSACTIONS 1000000
#define MAX_CLIENTS 200
#define CREATE 0
#define QUERY 1
#define UPDATE 2
#define QUIT 3
#define OK 0
#define ERR 1

using namespace std;

//static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

//Server Account Records
struct acc_record
{
	int id;
	int bal;
	pthread_mutex_t lock;
};

//Thread Data
struct thrd_data
{
	int csock;
	vector<struct acc_record> acc;		//<---Account Records at Server
}thrd_data;

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

vector<struct acc_record> acc;

/*
Return account no to be assigned.
*/

int create_acc(int acc_no,int bal)
{
	struct acc_record customer;
	customer.id = acc_no;
	customer.bal = bal;
	pthread_mutex_init(&customer.lock,NULL);
	acc.push_back(customer);
	return acc_no;
}

int query_acc(int id)
{
	int i;
	for(i=0;i<acc.size();i++)
	{
		if(acc[i].id == id)
		{
			return acc[i].bal;
		}
	}
	return -1;
}

int update_acc(int id, int bal)
{
	int i;
	for(i=0;i<acc.size();i++)
	{
		if(acc[i].id == id)
		{
			pthread_mutex_lock(&acc[i].lock);
			acc[i].bal = bal;
			pthread_mutex_unlock(&acc[i].lock);
			return bal;
		}
	}
	return -1;
}


void*  perform_tsn(void *arg)
{
	int csock;
	struct command cmd;
	struct reply response;
	int byte_read,byte_written,status;
	struct thrd_data *td = (struct thrd_data *)arg;
	csock = td->csock;
	do
	{	
		byte_read = read(csock,&cmd,sizeof(cmd));

		if(cmd.type == CREATE)
		{	//CREATE
			status = create_acc(cmd.id,cmd.bal);
			if(status < 1)
			{
				printf("\nUnable to create Account!!");
				response.type = ERR;
			}
			response.type = OK;
			response.val = status;
			printf("\nCreate %d %d %d",cmd.type,status,cmd.bal);
		}
		else if(cmd.type == QUERY)
		{	//QUERY
			status = query_acc(cmd.id);
			if(status == -1)
			{
				printf("\nAccount doesn't exist!!");
				response.type = ERR;
			}
			response.type = OK;
			response.val = status;
			printf("\nQuery %d %d %d",cmd.type,cmd.id,status);
		}
		else if(cmd.type == UPDATE)
		{	//UPDATE
			status = update_acc(cmd.id,cmd.bal);
			if(status == -1)
			{
				printf("\nUnable to Update!!");
				response.type = ERR;
			}
			response.type = OK;
			response.val = status;
			printf("\nUpdate %d %d %d",cmd.type,cmd.id,status);
		}

		byte_written = write(csock,&response,sizeof(response));
	}while(cmd.type!=QUIT);

	close(csock);
}

/*
Main fuction to start the Server.
*/
int main(int argc,char *argv[])
{
	struct command cmd;
	struct reply response;
	int lsock,csock[MAX_CLIENTS];
	socklen_t clilen;
	struct sockaddr_in server_addr,cli_addr;
	int port,byte_written,byte_read;
	int status,no_of_clients;
	pthread_t thrd;
	struct thrd_data td[MAX_CLIENTS];

	setbuf(stdout,NULL);

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

	//Listen
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
		//td[no_of_clients].acc = acc;
		td[no_of_clients].csock = csock[no_of_clients];
		pthread_create(&thrd,NULL,perform_tsn,(void*)&td[no_of_clients]);
		no_of_clients = (no_of_clients + 1)%200 ;
	}while(1);

	close(lsock);

	return 0;
}
