/*
g++ -o client client.cpp
./client Transactions.txt localhost 12340
*/

#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>

#define MAXRECORDS 10000
#define SERVER_ADDR "localhost"
#define CREATE 0
#define QUERY 1
#define UPDATE 2
#define QUIT 3
#define OK 0
#define ERR 1
#define FILENAME "Transactions.txt"


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

/*
This function reads the transaction file and prepares the array of structures to pass it to the server in order to perform transactions
*/
struct command* getRecords(char file[], int *totalTransactions)
{
	FILE *client;
	char type[10];
	int id,i;
	int amt;
	struct command *cmd = (struct command *)malloc(MAXRECORDS * sizeof(struct command));

	client = fopen(file,"r");
	if(client == NULL)
	{
		printf("unable to open file\n");
		return 0;
	}
	
	i=0;
	while ( !feof(client) )
	{
		fscanf(client,"%s ",type);
		if(!strcmp(type,"CREATE"))
		{
			cmd[i].type = CREATE;
			fscanf(client,"%d\n",&amt);
			cmd[i].bal = amt;
			cmd[i].id = -1;
		}
		else if(!strcmp(type,"UPDATE"))
		{
			cmd[i].type = UPDATE;
			fscanf(client,"%d %d\n",&id,&amt);
			cmd[i].bal = amt;
			cmd[i].id = id;
		}
		else if(!strcmp(type,"QUERY"))
		{
			cmd[i].type = QUERY;
			fscanf(client,"%d\n",&id);
			cmd[i].bal = -1;
			cmd[i].id = id;
		}
		else if(!strcmp(type,"QUIT"))
		{
			cmd[i].type = QUIT;
			cmd[i].bal = -1;
			cmd[i].id = -1;
		}
		cmd[i].tsn_id = i;
		//printf("%d %d %c %d\n",ts,id,type,amt);
		i++;
	}
	*totalTransactions = i;
	//printf("%s :Total Transaction :%d,i = %d ",__FUNCTION__,*totalTransactions,i);
	fclose(client);
	return cmd;
}

/*
This function prints the records which the client read from the transaction file
*/
void printRecords(struct command *rec, int no_of_tsn)
{
	int i;
	for ( i=0;i<no_of_tsn;i++ )
	{
		if(rec[i].type == CREATE)
		{
			printf("\nCREATE %d",rec[i].bal);
		}
		else if(rec[i].type == UPDATE)
		{
			printf("\nUPDATE %d %d",rec[i].id,rec[i].bal);
		}
		else if(rec[i].type == QUERY)
		{
			printf("\nQUERY %d",rec[i].id);
		}
		else if(rec[i].type == QUIT)
		{
			printf("\nQUIT");
		}
	}
	printf("\n");
}

/*
It prints the message recieved from the server on the client terminal
*/
void printRecvMsg(struct reply resp)
{
	if(resp.type == OK)
	{
		printf("\nOK %d",resp.val);
	}
	else
	{
		printf("\nERR %d",resp.val);
	}
	printf("\n");
}


/*
Main function to start a client
*/
int main(int argc,char *argv[])
{
	
	
	struct command* cmd;
	struct reply response;
	char filename[80];
	int cli_sock;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	void *msg;
	int port, totalCmd;
	int i,byte_read,byte_write = -1;

	setbuf(stdout,NULL);
	strcpy(filename,argv[1]);
	
	cmd = getRecords(filename,&totalCmd);
	if(cmd == NULL)
	{
		printf("\nunable to open client file");
		return -1;
	}

	printRecords(cmd,totalCmd);	

	//Make Clients
	cli_sock = socket(AF_INET,SOCK_STREAM,0);
	if(cli_sock == -1)
	{
		printf("\nUnable to create Socket Descriptor");
		return -1;
	}

	//Prepare Server Structure to Connect
	server = gethostbyname(argv[2]);
	port = atoi(argv[3]);
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

	//Send Transactions to SERVER
	for(i = 0;i < totalCmd; i++)
	{

		byte_write = write(cli_sock, (void *)&cmd[i], sizeof(struct command));
		byte_read = read(cli_sock,&response,sizeof(response));

		if(byte_read > 0)
		{
			//printRecvMsg(response);
			printf("\n%d %d",response.type,response.val);
		}
	}

	close(cli_sock);

	return 0;
}
