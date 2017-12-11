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
	if(bal>0)
	{
		customer.id = acc_no;
		customer.bal = bal;
		pthread_mutex_init(&customer.lock,NULL);
		acc.push_back(customer);
		return acc_no;
	}
	return -1;
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

int acc_exists(int id)
{
	int i;
	for(i=0;i<acc.size();i++)
	{
		if(acc[i].id == id)
		{
			return i;
		}
	}
	return -1;
}

int update_acc(int id, int bal)
{
	int i;
	if(bal > 0)
	{
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
	}
	return -1;
}

int wait_for_commit(int csock,struct command cmd)
{
	int flag,byte_written,byte_read;
	struct reply response;
	if(cmd.type == CREATE && cmd.bal<0)
	{
		response.type = ERR;
		response.val = -1;
		printf("\nABORT");
	}
	else if(cmd.type == UPDATE && cmd.bal<0)
	{
		response.type = ERR;
		response.val = -1;
		printf("\nABORT");
	}
	else if(cmd.type == QUERY && acc_exists(cmd.id) == -1)
	{
		response.type = ERR;
		printf("\nABORT");
	}
	else if(cmd.type == UPDATE && acc_exists(cmd.id) == -1)
	{
		response.type = ERR;
		response.val = cmd.id;
		printf("\nABORT");
	}
	else
	{
		response.type = OK;
		printf("\nReady to Commit");
	}
	byte_written = write(csock,&response,sizeof(response));
	
	byte_read = read(csock,&flag,sizeof(flag));
	return flag;
}

void*  perform_tsn(void *arg)
{
	int csock;
	int flag;
	struct command cmd;
	struct reply response;
	int byte_read,byte_written,status;
	struct thrd_data *td = (struct thrd_data *)arg;
	csock = td->csock;
	do
	{
		byte_read = read(csock,&cmd,sizeof(cmd));
		printf("\nINIT State");
		flag = wait_for_commit(csock,cmd);
		if(flag == OK)
		{
			if(cmd.type == CREATE)
			{	//CREATE
				status = create_acc(cmd.id,cmd.bal);
				if(status < 0)
				{
					printf("\nUnable to create Account!!");
					response.type = ERR;
					response.val = -1;
				}
				else
				{
					response.type = OK;
					response.val = status;
					printf("\nCommit",status,cmd.bal);
				}
			}
			else if(cmd.type == QUERY)
			{	//QUERY
				status = query_acc(cmd.id);
				if(status == -1)
				{
					printf("\nAccount doesn't exist!!");
					response.type = ERR;
				}
				else
				{
					response.type = OK;
					response.val = status;
				}
				printf("\nCommit",cmd.id,status);
			}
			else if(cmd.type == UPDATE)
			{	//UPDATE
				status = update_acc(cmd.id,cmd.bal);
				if(status == -1)
				{
					printf("\nUnable to Update!!");
					response.type = ERR;
					if(cmd.bal<0)
					{
						response.val = -1;
					}
					else
					{
						response.val = cmd.id;
					}
				}
				else
				{
					response.type = OK;
					response.val = status;
				}
				printf("\nnCommit",cmd.id,status);
			}
			byte_written = write(csock,&response,sizeof(response));
		}
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
	int reuse = 1;
	struct thrd_data td[MAX_CLIENTS];

	setbuf(stdout,NULL);

	if((lsock = socket(AF_INET,SOCK_STREAM,0)) == -1)
	{
		printf("\nUnable to get server socket");
		return -1;
	}

	// enable the socket to reuse the address
    if (setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) 
	{
		perror("\nfailed allowing server socket to reuse address");
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
