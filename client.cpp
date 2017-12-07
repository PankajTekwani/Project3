/*
g++ -o client client.cpp
./client Transactions.txt localhost 12340
*/

#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string>
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

using namespace std;

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
char str[] ="- This, a sample string.";
  char * pch;
  printf ("Splitting string \"%s\" into tokens:\n",str);
  pch = strtok (str," ,.-");
  while (pch != NULL)
  {
    printf ("%s\n",pch);
    pch = strtok (NULL, " ,.-");
  }
*/
struct command get_command(string *str)
{
	struct command cmd;
	char s[30];
	char *pch;
	int a, b;

	cin.getline(s,30);
	pch = strtok (s," ");
	if(pch!=NULL)
	{
		*str = string(pch);
	}
	pch = strtok (NULL," ");
	if(pch!=NULL)
	{
		a = atoi(pch);
	}
	pch = strtok (NULL," ");
	if(pch!=NULL)
	{
		b = atoi(pch);
	}

	
	for(int j = 0; j < (*str).length(); j++)
       		(*str)[j] = toupper((*str)[j]);

	if(!(*str).compare(string("CREATE")))
	{
		cmd.type = CREATE;
		cmd.bal = a;
		cmd.id = -1;
	}
	else if(!(*str).compare(string("UPDATE")))
	{
		cmd.type = UPDATE;
		cmd.bal = b;
		cmd.id = a;
	}
	else if(!(*str).compare(string("QUERY")))
	{
		cmd.type = QUERY;
		cmd.bal = -1;
		cmd.id = a;
	}
	else if(!(*str).compare(string("QUIT")))
	{
		cmd.type = QUIT;
		cmd.bal = -1;
		cmd.id = -1;
	}
	
	return cmd;	
}

/*
Main function to start a client
*/
int main(int argc,char *argv[])
{
	
	
	//struct command* cmd;
	struct command cmd;
	struct reply response;
	char filename[80];
	int cli_sock;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	int port, totalCmd;
	int i,byte_read,byte_write = -1;
	string str;
	int a,b;
	setbuf(stdout,NULL);


	//Make Clients
	cli_sock = socket(AF_INET,SOCK_STREAM,0);
	if(cli_sock == -1)
	{
		printf("\nUnable to create Socket Descriptor");
		return -1;
	}

	//Prepare Server Structure to Connect
	server = gethostbyname(argv[2]);
	port = atoi(argv[1]);
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
	//for(i = 0;i < totalCmd; i++)
	do
	{

		cmd = get_command(&str);
		byte_write = write(cli_sock, (void *)&cmd, sizeof(struct command));
		cout<<"cmd:"<<cmd.type<<" "<<cmd.id<<" "<<cmd.bal<<endl;
		byte_read = read(cli_sock,&response,sizeof(response));

		if(byte_read > 0)
		{
			//printRecvMsg(response);
			printf("response:%d %d\n",response.type,response.val);
		}
	}while(str.compare(string("QUIT")));

	close(cli_sock);

	return 0;
}
