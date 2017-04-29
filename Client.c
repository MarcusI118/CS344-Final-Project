#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */

#define RCVBUFSIZE 100   /* Size of receive buffer */
#define MAX_SIZE 51 /*Includes room for null */

struct menu
{
  unsigned char line1[20];
  unsigned char line2[20];
  unsigned char line3[20];
  unsigned char line4[20];
  unsigned char line5[20];
};

struct project
{		
  unsigned char line1[20];
  unsigned char line2[20];
  unsigned char line3[20];
  unsigned char line4[20]; 
  int line5;
}displayProject;

typedef struct
{
  unsigned int x;
  unsigned int y;
  unsigned char oper;
}TRANS_DATA_TYPE;

typedef struct{
  unsigned int x;
  unsigned int y;
}DATA_TYPE;

void DieWithError(char *errorMessage);  /* Error handling function */
void get(int, void *, unsigned int);
void put(int, void *, unsigned int);
void talkToServer(int);
void talkToAuthServer(int);
unsigned int displayMenuAndSendSelection(int);

void sendUserNameSignup(int);
void sendPasswordSignup(int);
void sendUserFullNameSingup(int);

void sendProjectInfo(int);

void askForList(int);
void sendUsernameLogin(int);
void sendPasswordLogin(int);

void sendSearch(int);

int main(int argc, char *argv[])
{
    int sock;                        /* Socket descriptor */
    struct sockaddr_in echoServAddr; /* Echo server address */
    unsigned short echoServPort;     /* Echo server port */
    char *servIP;                    /* Server IP address (dotted quad) */
    char *echoString;                /* String to send to echo server */
    unsigned int echoStringLen;      /* Length of string to echo */
    int bytesRcvd, totalBytesRcvd;   /* Bytes read in single recv()
                                       and total bytes read */
    int answer;

    DATA_TYPE data;
    TRANS_DATA_TYPE incoming;
    memset(&incoming, 0, sizeof(TRANS_DATA_TYPE));

    if ((argc < 2) || (argc > 3))    /* Test for correct number of arguments */
    {
       fprintf(stderr, "Usage: %s <Server IP> [<Echo Port>]\n",
               argv[0]);
       exit(1);
    }

    servIP = argv[1];             /* First arg: server IP address (dotted quad) */

    if (argc == 3)
        echoServPort = atoi(argv[2]); /* Use given port, if any */
    else
        echoServPort = 7;  /* 7 is the well-known port for the echo service */

    /* Create a reliable, stream socket using TCP */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    /* Construct the server address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));     /* Zero out structure */
    echoServAddr.sin_family      = AF_INET;             /* Internet address family */
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);   /* Server IP address */
    echoServAddr.sin_port        = htons(echoServPort); /* Server port */

    /* Establish the connection to the echo server */
    if (connect(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("connect() failed");

   // echoStringLen = strlen(echoString);          /* Determine input length */

    talkToServer(sock);

    close(sock);
    exit(0);
}

void talkToServer(int sock)
{
    unsigned int selection = 0;
    unsigned char bye[5];

    while(1)
    {
        selection = displayMenuAndSendSelection(sock);
        printf("Client selected: %d\n", selection);
        switch(selection)
        {
            case 1:
		sendUserFullNameSingup(sock);
                sendUserNameSignup(sock);
		sendPasswordSignup(sock);
                break;
            case 2:
                sendUsernameLogin(sock);
	        sendPasswordLogin(sock);
		talkToAuthServer(sock);
                break;
            }
        if(selection == 3) break;
    }
    selection = htonl(selection);
    put(sock, &selection, sizeof(unsigned int));
    get(sock, bye, 5);
    printf("%s\n", bye);
}


void talkToAuthServer(int sock)
{
    unsigned int selection = 0;
    unsigned char bye[5];

    while(1)
    {
        selection = displayMenuAndSendSelection(sock);
        printf("Client selected: %d\n", selection);
        switch(selection)
        {
            case 1:
		sendProjectInfo(sock);
                break;
            case 2:
		sendProjectInfo(sock);
                break;
	   case 4:
		sendSearch(sock);
		askForList(sock);
		break;
	   case 5: 
		sendProjectInfo(sock);
		break;
            }
        if(selection == 3) break;
    }
    selection = htonl(selection);
    put(sock, &selection, sizeof(unsigned int));
    get(sock, bye, 5);
    printf("%s\n", bye);
}

void askForList(int sock)
{    
	unsigned char msg[21];

	memset(msg, 0, sizeof(msg));
	strcpy(msg, "Projects");
	put(sock, msg, sizeof(msg));

	struct project projectDisplay;
	memset(&projectDisplay, 0, sizeof(struct project));
	get(sock, &projectDisplay, sizeof(struct project));
	printf("\n");
	printf("Project Info:\n");
	printf("Name:%s\n", projectDisplay.line1);
	printf("Description:%s\n", projectDisplay.line2);
	printf("Date:%s\n", projectDisplay.line3);
	printf("Due Date:%s\n", projectDisplay.line4);
	printf("Users(#): %d\n", projectDisplay.line5);


}



unsigned int displayMenuAndSendSelection(int sock)
{
    struct menu menuBuffer;     /* Buffer for echo string */
    unsigned int response = 0;
    unsigned int output;

    printf("Inside client display menu\n");
    get(sock, &menuBuffer, sizeof(struct menu));  //in this case server is also sending null
    printf("%s\n", menuBuffer.line1);
    printf("%s\n", menuBuffer.line2);
    printf("%s\n", menuBuffer.line3);
    printf("%s\n", menuBuffer.line4); 
    printf("%s\n", menuBuffer.line5);
    scanf("%d", &response);
    output = htonl(response);
    put(sock, &output, sizeof(unsigned int));
    return response;
}

void sendSearch(int sock)
{
	unsigned char msg[21];
	unsigned char search[100];
	memset(msg, 0, sizeof(msg));
	get(sock, msg, sizeof(msg));
	printf("%s\n", msg);

	scanf("%s", search);
	put(sock, search, 100);	
}

void sendProjectInfo(int sock)
{
	unsigned char msg[21];
	unsigned char projectName[100];
	unsigned char projectDescrption[1000];
	unsigned char projectDueDate[8];
	unsigned char projectDate[8];
	int numUsers;
	int numIn;

	memset(msg, 0, sizeof(msg));
	get(sock, msg, sizeof(msg));
	printf("%s\n", msg);
	scanf("%s", projectName);
	put(sock, projectName, 100);
	
	memset(msg, 0, sizeof(msg));
	get(sock, msg, sizeof(msg));
	printf("%s\n", msg);
	scanf("%s", projectDescrption);
	put(sock, projectDescrption, 100); 

	memset(msg, 0, sizeof(msg));
	get(sock, msg, sizeof(msg));
	printf("%s\n", msg);
	scanf("%s", projectDate);
	put(sock, projectDate, 8);
	
	memset(msg, 0, sizeof(msg));
	get(sock, msg, sizeof(msg));
	printf("%s\n", msg);
	scanf("%s", projectDueDate);
	put(sock, projectDueDate, 8); 

	memset(msg, 0, sizeof(msg));
	get(sock, msg, sizeof(msg));
	printf("%s\n", msg);
	scanf("%d", &numUsers);
        numIn = htonl(numUsers);
	put(sock, &numIn, sizeof(int));
}


void sendUserFullNameSingup(int sock)
{
    unsigned char msg[21];
    unsigned char fullname[MAX_SIZE];

    memset(msg, 0, sizeof(msg));
    get(sock, msg, sizeof(msg));
    printf("%s\n", msg);
    memset(fullname, 0, MAX_SIZE);
    scanf("%s", fullname);
    put(sock, fullname, MAX_SIZE);
}

void sendUserNameSignup(int sock)
{
    unsigned char msg[21];
    unsigned char username[MAX_SIZE];

    memset(msg, 0, sizeof(msg));
    get(sock, msg, sizeof(msg));
    printf("%s\n", msg);
    memset(username, 0, MAX_SIZE);
    scanf("%s", username);
    put(sock, username, MAX_SIZE);
}

void sendPasswordSignup(int sock)
{
    unsigned char msg[21];
    unsigned char password[MAX_SIZE];

    memset(msg, 0, sizeof(msg));
    get(sock, msg, sizeof(msg));
    printf("%s\n", msg);
    memset(password, 0, MAX_SIZE);
    scanf("%s", password);
    put(sock, password, MAX_SIZE);
}

void sendUsernameLogin(int sock)
{
    unsigned char msg[21];
    unsigned char username_l[MAX_SIZE];

    memset(msg, 0, sizeof(msg));
    get(sock, msg, sizeof(msg));
    printf("%s\n", msg);
    memset(username_l, 0, MAX_SIZE);
    scanf("%s", username_l);
    put(sock, username_l, MAX_SIZE);
}

void sendPasswordLogin(int sock)
{
    unsigned char msg[21];
    unsigned char password_l[MAX_SIZE];

    memset(msg, 0, sizeof(msg));
    get(sock, msg, sizeof(msg));
    printf("%s\n", msg);
    memset(password_l, 0, MAX_SIZE);
    scanf("%s", password_l);
    put(sock, password_l, MAX_SIZE);
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

void put(int sock, void * buffer, unsigned int size)
{
    if (send(sock, buffer, size, 0) != size)
        DieWithError("send() failed");
}
void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}

