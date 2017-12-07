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
#include <netdb.h>
#include <vector>
#include <iostream>

using namespace std;

#define MAXRECORDS 10000
#define MAXCLIENTS 200
#define BACKSERVERS 1
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


pthread_cond_t cv;
vector<int> acc;

struct server_details
{
	int sock;
	int port;
	struct log_table* log;
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
	//queue <struct command> logs;
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
		//printf("\ncliCMD:%d %d %d",cmd.type,cmd.id,cmd.bal);
		if(cmd.type == QUIT)
		{
			log->logs[i].type = cmd.type;
			log->index = log->index + 1;
			break;
		}
		pthread_mutex_lock(&log->log_lock);
			log->logs[i].type = cmd.type;
			log->logs[i].bal = cmd.bal;
			log->logs[i].id = cmd.id;
			log->logs[i].tsn_id = i+1;
			log->index = log->index + 1;
			//printf("\nLogIndex:%d",log->index);
		pthread_cond_signal(&cv);
		pthread_mutex_unlock( &log->log_lock);
		response.type = OK;
		response.val = 1234;
		byte_written = write(td->csock,&response,sizeof(response));
		i++;
	}

	//All Transactions from a Client are performed, so closing the connection.
	close(td->csock);
	printLogs(td->log);
}


int connectToServer(char *ip, int port)
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
	if(connect(sock,(struct sockaddr *)&serv_addr,sizeof(serv_addr))<0)
	{
		printf("\nError in Client Connection");
		return -1;
	}

	return sock;
}

int generate_acc_no()
{
	int acc_no,i,f;

	do
	{
		acc_no = rand() % MAXRECORDS;
		f=0;
		for(i=0;i < acc.size(); i++)
		{
			if(acc[i] == acc_no)
			{
				f=1;
				break;
			}
		}
	}while(f);
	acc.push_back(acc_no);
	return acc_no;

}

void * backend_server(void *arg)
{
	int i;
	int tmp;
	int byte_written,byte_read;
	int index;
	struct reply response;
	struct log_table* log;
	struct command cmd;
	struct server_details *serv = (struct server_details *)arg;
	for(i=0;i<BACKSERVERS;i++)
	{
		tmp = connectToServer("127.0.0.1",serv[i].port);
		if(tmp == -1)
		{
			printf("\nUnable to Connect to Backend Server %d",i+1);
			pthread_exit(NULL);
		}
		serv[i].sock = tmp;
	}

	log = serv[0].log;
	index = 0;
	do
	{
		pthread_mutex_lock( &log->log_lock);
		if(index == log->index)
		{
			pthread_cond_wait( &cv, &log->log_lock );
		}
		for(i=0;i<BACKSERVERS;i++)
		{
			cmd = log->logs[index];
			if(cmd.type == CREATE)
			{	//CREATE
				cmd.id = generate_acc_no();
			}
			byte_written = write(serv[i].sock,&cmd,sizeof(cmd));
			byte_read = read(serv[i].sock,&response,sizeof(response));
			printf("\nCmd:%d %d %d",cmd.type,cmd.id,cmd.bal);
			printf("\n%d %d",response.type,response.val);
		}
		index++;
		pthread_mutex_unlock( &log->log_lock);
	}while(1);
	
}

/*
Main fuction to start the Server.
*/
int main(int argc,char *argv[])
{
	struct thrd_data td[MAX_CLIENTS];
	struct log_table log;
	int lsock,csock[MAX_CLIENTS];
	struct server_details server[BACKSERVERS];
	socklen_t clilen;
	struct sockaddr_in server_addr,cli_addr;
	int port,i,no_of_clients,byte_written;
	log.index = 0;
	pthread_t thrd;
	pthread_t server_thrd;
	setbuf(stdout,NULL);

	pthread_mutex_init(&log.log_lock,NULL);
	pthread_cond_init(&cv,NULL);

	//Connect to backend Servers
	for(i=0;i<BACKSERVERS;i++)
	{
		server[i].port = atoi(argv[i+2]);
		server[i].log = &log;
	}
	pthread_create(&server_thrd,NULL,backend_server,(void *)&server[0]);

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