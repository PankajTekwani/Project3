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

// Function to read Records.txt File and prepare the A/C records data structure
/*
struct acc_record* readServerFile(char file[])
{
	FILE *record;
	float bal;
	int id,i;

	struct acc_record *rec = (struct acc_record *)malloc(MAXRECORDS * sizeof(struct acc_record));

	record = fopen(file,"r");
	if(record == NULL)
	{
		cout<<"Unable to open Record File"<<endl;
		return NULL;
	}
	
	i=0;
	while ( !feof(record) )
	{
		fscanf(record,"%d %f\n",&id,&bal);
		rec[i].id = id;
		rec[i].bal = bal;
		//pthread_mutex_init(&rec[i].lock,NULL);
		//rec[i].lock = PTHREAD_MUTEX_INITIALIZER;
		i++;
	}
	//Dummy Record
	rec[i].id = -1;
	rec[i].bal = 0;
	
	fclose(record);
	return rec;
}
*/


/*
Return account no to be assigned.
*/

int create_acc(struct accounts *acc,int acc_no,int bal)
{
	acc->account[acc->total_accounts].id = acc_no;
	acc->account[acc->total_accounts].bal = bal;
	acc->total_accounts = acc->total_accounts + 1;
	return acc_no;
}

int query_acc(struct accounts *acc,int id)
{
	int i;
	for(i=0;i<acc->total_accounts;i++)
	{
		if(acc->account[i].id == id)
		{
			return acc->account[i].bal;
		}
	}
	return -1;
}

int update_acc(struct accounts *acc,int id, int bal)
{
	int i;
	for(i=0;i<acc->total_accounts;i++)
	{
		if(acc->account[i].id == id)
		{
			acc->account[i].bal = bal;
			return bal;
		}
	}
	return -1;
}

/*
Main fuction to start the Server.
*/
int main(int argc,char *argv[])
{
	struct accounts acc;
	struct command cmd;
	struct reply response;
	char msg[50];
	int lsock,csock;
	socklen_t clilen;
	struct sockaddr_in server_addr,cli_addr;
	int port,byte_written,byte_read;
	int status;

	setbuf(stdout,NULL);
	acc.total_accounts = 0;

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
	csock = accept(lsock,(struct sockaddr *)&cli_addr,&clilen);
	if (csock < 0) 
    {
		printf("\nConnection Request from Coordinator Denied!!");
	}
	//strcpy(msg,"Connection Accepted");
	//byte_written = write(csock, (void *)msg, strlen(msg) + 1);
	while(1)
	{	
		byte_read = read(csock,&cmd,sizeof(cmd));

		if(cmd.type == CREATE)
		{	//CREATE
			status = create_acc(&acc,cmd.id,cmd.bal);
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
			status = query_acc(&acc,cmd.id);
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
			status = update_acc(&acc,cmd.id,cmd.bal);
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
	}

	close(lsock);

	return 0;
}
