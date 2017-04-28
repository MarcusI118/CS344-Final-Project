#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */

#define MAXPENDING 5    /* Maximum outstanding connection requests */
#define RCVBUFSIZE 32   /* Size of receive buffer */
#define NAME_SIZE 21 /*Includes room for null */

struct menu{
  unsigned char line1[20];
  unsigned char line2[20];
  unsigned char line3[20];
} men;

void DieWithError(char *errorMessage);  /* Error handling function */
void HandleTCPClient(int clntSocket);   /* TCP client handling function */
void get(int, void *, unsigned int);
void put(int, void *, unsigned int);
unsigned int sendMenuAndWaitForResponse(int);



void askForPasswordSignup(int , char *, unsigned int);
void askForFullNameSingup(int , char *, unsigned int);
void askForUsernameSignup(int , char * , unsigned int );
void writeUserNameToFile(char *, char *, char *);

void askForUsernameLogin(int, char *, unsigned int);
void askForPasswordLogin(int, char *, unsigned int);

int logedIn(char *, char *);


int main(int argc, char *argv[])
{
    int servSock;                    /* Socket descriptor for server */
    int clntSock;                    /* Socket descriptor for client */
    struct sockaddr_in echoServAddr; /* Local address */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned short echoServPort;     /* Server port */
    unsigned int clntLen;            /* Length of client address data structure */

    if (argc != 2)     /* Test for correct number of arguments */
    {
        fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
        exit(1);
    }

    echoServPort = atoi(argv[1]);  /* First arg:  local port */

    /* Create socket for incoming connections */
    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");
      
    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(echoServPort);      /* Local port */

    /* Bind to the local address */
    if (bind(servSock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("bind() failed");

    /* Mark the socket so it will listen for incoming connections */
    if (listen(servSock, MAXPENDING) < 0)
        DieWithError("listen() failed");

    for (;;) /* Run forever */
    {
        /* Set the size of the in-out parameter */
        clntLen = sizeof(echoClntAddr);

        /* Wait for a client to connect */
        if ((clntSock = accept(servSock, (struct sockaddr *) &echoClntAddr, 
                               &clntLen)) < 0)
            DieWithError("accept() failed");

        /* clntSock is connected to a client! */

        printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));

        HandleTCPClient(clntSock);
    }
    /* NOT REACHED */
}
void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}
void get(int sock, void * buffer, unsigned int size)
{
  unsigned int totalBytesRcvd = 0;
  unsigned int bytesRcvd = 0;

  while(totalBytesRcvd < size)
  {
     if((bytesRcvd = recv(sock, buffer, size, 0)) <= 0)
       DieWithError("recv() failed or connection closed prematurely");
     totalBytesRcvd += bytesRcvd;
  }
}
void HandleTCPClient(int clntSocket)
{
    int recvMsgSize;                    /* Size of received message */
    unsigned int response = 0;

    unsigned char errorMsg[] = "Invalid Choice";
    unsigned char bye[] = "Exiting Work Project Tool!";

    unsigned char userName[NAME_SIZE]; 
    unsigned char userPassword[NAME_SIZE];
    unsigned char userFullName[NAME_SIZE];

    unsigned char userNameLogin[NAME_SIZE]; 
    unsigned char userPasswordLogin[NAME_SIZE];

    int success;



    response = sendMenuAndWaitForResponse(clntSocket);
    while(response != 3)
    {
        switch(response)
        {
            case 1: printf("Sing-up\n");
		    askForFullNameSingup(clntSocket, userFullName, NAME_SIZE);
                    askForUsernameSignup(clntSocket, userName, NAME_SIZE);
		    askForPasswordSignup(clntSocket,userPassword, NAME_SIZE);
		    writeUserNameToFile(userName, userPassword, userFullName);
                    break;
            case 2: printf("Log-in\n");
		    askForUsernameLogin(clntSocket, userNameLogin, NAME_SIZE);
		    askForPasswordLogin(clntSocket, userPasswordLogin, NAME_SIZE);
		    success = logedIn(userNameLogin, userPasswordLogin);
			printf("success %d", success);
	
	
                    break;
            default: printf("Client selected junk.\n"); put(clntSocket, errorMsg, sizeof(errorMsg)); break;
        }
        response = sendMenuAndWaitForResponse(clntSocket);
    }//end while

    put(clntSocket, bye, sizeof(bye));
    close(clntSocket);    /* Close client socket */
    printf("Connection with client %d closed.\n", clntSocket);
}

unsigned int sendMenuAndWaitForResponse(int clntSocket)
{
    struct menu mainMenu;
    unsigned int response = 0;
    memset(&mainMenu, 0, sizeof(struct menu));   /* Zero out structure */
    strcpy(mainMenu.line1,"1)Sign-up\n");
    strcpy(mainMenu.line2, "2)Login\n");
    strcpy(mainMenu.line3, "3)Quit\n");
    printf("Sending menu\n");
    put(clntSocket, &mainMenu, sizeof(struct menu));
    get(clntSocket, &response, sizeof(unsigned int));
    return ntohl(response);
}
void askForFullNameSingup(int sock, char * fullname, unsigned int size)
{
    unsigned char msg[21];
    memset(msg, 0, sizeof(msg));
    strcpy(msg, "Enter fullname\n");
    put(sock, msg, sizeof(msg));
    memset(fullname, 0, NAME_SIZE);
    get(sock, fullname, NAME_SIZE);
}

void askForUsernameSignup(int sock, char * username, unsigned int size)
{
    unsigned char msg[21];
    memset(msg, 0, sizeof(msg));
    strcpy(msg, "Enter username\n");
    put(sock, msg, sizeof(msg));
    memset(username, 0, NAME_SIZE);
    get(sock, username, NAME_SIZE);
}
void askForPasswordSignup(int sock, char * password, unsigned int size)
{
    unsigned char msg[21];
    memset(msg, 0, sizeof(msg));
    strcpy(msg, "Enter password\n");
    put(sock, msg, sizeof(msg));
    memset(password, 0, NAME_SIZE);
    get(sock, password, NAME_SIZE);
}



void askForUsernameLogin(int sock, char * username_l, unsigned int size)
{
    unsigned char msg[21];
    memset(msg, 0, sizeof(msg));
    strcpy(msg, "Enter username\n");
    put(sock, msg, sizeof(msg));
    memset(username_l, 0, NAME_SIZE);
    get(sock, username_l, NAME_SIZE);
}
void askForPasswordLogin(int sock, char * password_l, unsigned int size)
{
    unsigned char msg[21];
    memset(msg, 0, sizeof(msg));
    strcpy(msg, "Enter password\n");
    put(sock, msg, sizeof(msg));
    memset(password_l, 0, NAME_SIZE);
    get(sock, password_l, NAME_SIZE);
}


void writeUserNameToFile(char * userName, char * userPassword, char *userFullName)
{
	FILE *fp;
	fp = fopen("Users.txt", "w+");
	//fputs(userName,  fp);
	fprintf(fp,"%s %s %s",userName, userPassword, userFullName);
	fclose(fp);

}

int logedIn(char * userAuth, char * passAuth)
{

	FILE *fp;
	fp = fopen("Users.txt", "r");
	
	char passCheck[NAME_SIZE];
	char userCheck[NAME_SIZE];
	
	
	char line[NAME_SIZE];
	while(fgets(line, sizeof(line), fp) != NULL)
	{
		printf("file line: %s", line);
		sscanf(line, "%s %s", userCheck, passCheck);
		if(strcmp(userCheck, userAuth) == 0 && strcmp(passCheck, passAuth) == 0)	
		{
			fclose(fp);	
			return 1;
			break; 
			
		}
		printf("check u %s check p %s", userCheck, passCheck);
	}
}

	
void put(int sock, void * buffer, unsigned int size)
{
    if (send(sock, buffer, size, 0) != size)
        DieWithError("send() failed");
}

