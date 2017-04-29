#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */

#define MAXPENDING 5    /* Maximum outstanding connection requests */
#define RCVBUFSIZE 32   /* Size of receive buffer */
#define MAX_SIZE 51 /*Includes room for null */

struct menu
{
  unsigned char line1[20];
  unsigned char line2[20];
  unsigned char line3[20];
  unsigned char line4[20];
  unsigned char line5[20];
} men;


struct project
{		
  unsigned char line1[20];
  unsigned char line2[20];
  unsigned char line3[20];
  unsigned char line4[20]; 
  int line5;
}displayProject;

typedef struct ListData 
{
	char projectname[100];
	char projectdescription[1000];
	char date[8];
	char duedate[8];
	int numusers;
	struct ListData *next;
}DataInList;

void DieWithError(char *errorMessage);  /* Error handling function */
void HandleTCPClient(int clntSocket);   /* TCP client handling function */
void get(int, void *, unsigned int);
void put(int, void *, unsigned int);
unsigned int sendMenuAndWaitForResponse(int);
unsigned int sendMenuAuth(int);
void HandleTCPClientAuth(int);
void askForPasswordSignup(int , char *, unsigned int);
void askForFullNameSingup(int , char *, unsigned int);
void askForUsernameSignup(int , char * , unsigned int );
void writeUserNameToFile(char *, char *, char *);
void askForUsernameLogin(int, char *, unsigned int);
void askForPasswordLogin(int, char *, unsigned int);
void askForProjectInfo(int, char *, char *, char *, char *, int *);
int logedIn(char *, char *);
DataInList * appendToList(DataInList *, char[], char [], char [], char [], int);
void editList(DataInList *, char[], char [], char [], char [], int);
void displayList(DataInList *);

void testList(char * , char * , char * , char * , int, int);
void toFileList(char * , char * , char * , char * , int);
void deleteItem(DataInList *, char *);
void sendList(int, DataInList*, char *);


void askForSearch(int, char*);

void writeList(DataInList *);

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

    unsigned char userName[MAX_SIZE]; 
    unsigned char userPassword[MAX_SIZE];
    unsigned char userFullName[MAX_SIZE];

    unsigned char userNameLogin[MAX_SIZE]; 
    unsigned char userPasswordLogin[MAX_SIZE];

    int success;

    response = sendMenuAndWaitForResponse(clntSocket);
    while(response != 3)
    {
        switch(response)
        {
            case 1: printf("Sign-up\n");
		    askForFullNameSingup(clntSocket, userFullName, MAX_SIZE);
                    askForUsernameSignup(clntSocket, userName, MAX_SIZE);
		    askForPasswordSignup(clntSocket,userPassword, MAX_SIZE);
		    writeUserNameToFile(userName, userPassword, userFullName);
                    break;
            case 2: printf("Log-in\n");
		    askForUsernameLogin(clntSocket, userNameLogin, MAX_SIZE);
		    askForPasswordLogin(clntSocket, userPasswordLogin, MAX_SIZE);
		    success = logedIn(userNameLogin, userPasswordLogin);
		    if(success == 1)
			HandleTCPClientAuth(clntSocket);
			if(success ==0)
				HandleTCPClient(clntSocket);

                    break;
            default: printf("Client selected junk.\n"); put(clntSocket, errorMsg, sizeof(errorMsg)); break;
        }
        response = sendMenuAndWaitForResponse(clntSocket);//add sign
    }//end while

    put(clntSocket, bye, sizeof(bye));
    close(clntSocket);    /* Close client socket */
    printf("Connection with client %d closed.\n", clntSocket);
}

void HandleTCPClientAuth(int clntSocket)
{
    int recvMsgSize;                    /* Size of received message */
    unsigned int response = 0;

    unsigned char projectName[100];
    unsigned char projectDescription[1000];
    unsigned char projectDate[8];
    unsigned char projectDueDate[8];

	unsigned char search[100];
    int numUsers;
    
    DataInList *LinkedList = NULL;
    unsigned char bye[] = "Exiting Work Project Tool!";
	
    response = sendMenuAuth(clntSocket);
    while(response != 3)
    {
        switch(response)
        {
            case 1: printf("Add \n");
		    askForProjectInfo(clntSocket, projectName, projectDescription, projectDate, projectDueDate, &numUsers);
		     LinkedList = appendToList(LinkedList, projectName, projectDescription, projectDate, projectDueDate, numUsers);
		     displayList(LinkedList);
	            //printf("%s %s %s %s %d", projectName,projectDescription, projectDate, projectDueDate, numUsers);
                    break;
            case 2: printf("Edit\n");
		    askForProjectInfo(clntSocket, projectName, projectDescription, projectDate, projectDueDate, &numUsers);
		    editList(LinkedList, projectName, projectDescription, projectDate, projectDueDate, numUsers);
	    case 3: printf("Quit");
                    break;
	    case 4: printf("Search");
				askForSearch(clntSocket, search);
		   sendList(clntSocket,LinkedList, search);
			displayList(LinkedList);
	break;
		case 5: printf("Delete");
		askForProjectInfo(clntSocket, projectName, projectDescription, projectDate, projectDueDate, &numUsers);
		deleteItem(LinkedList, projectName);
		break;
            default: 
	    break; 
        }
        response = sendMenuAuth(clntSocket);
    }
		writeList(LinkedList);
    put(clntSocket, bye, sizeof(bye));
    close(clntSocket); 
    printf("Connection with client %d closed.\n", clntSocket);
}


void editList(DataInList *head, char pname[], char pdes[], char pd[], char pdd[], int users)
{
	int found = 0;
	while(head != NULL)
	{
		if(strcmp(head->projectname, pname) == 0)
		{
			found = 1;
			strcpy(head->projectdescription, pdes);
			strcpy(head->date, pd);
			strcpy(head->duedate, pdd);	
			head->numusers = users;
		}
		head = head->next;
	}
	if(found == 0)
	{
		printf("Project Not Found");
	}
}

DataInList * appendToList(DataInList *head, char pname[], char pdes[], char pd[], char pdd[], int users)
{

	DataInList *newNode = (DataInList *)malloc(sizeof(DataInList));
	DataInList *current = head;

        strcpy(newNode->projectname, pname);
	strcpy(newNode->projectdescription, pdes);
	strcpy(newNode->date, pd);
	strcpy(newNode->duedate, pdd);
	newNode->numusers = users;

	if(head == NULL)
	{
		head = newNode;
		head->next = NULL;
	}
	else
	{
		newNode->next = NULL;
		while(current->next)
		{
			current = current->next;
		}
		current->next = newNode;
		
	}

	return head;
}


void askForSearch(int sock, char *search)
{
	unsigned char msg[21];

	memset(msg, 0, sizeof(msg));
	strcpy(msg, "Search");
	put(sock, msg, sizeof(msg));
	memset(search, 0, 100);
	get(sock, search, 100);
}
void askForProjectInfo(int sock, char * projectName, char * projectDescription, char * projectDate, char * projectDueDate, int * numUsers)
{
	unsigned char msg[21];
	int numIn = 0;

	memset(msg, 0, sizeof(msg));
	strcpy(msg, "Project Name");
	put(sock, msg, sizeof(msg));
	memset(projectName, 0, 100);
	get(sock, projectName, 100);

	memset(msg, 0, sizeof(msg));
	strcpy(msg, "Project Description");
	put(sock, msg, sizeof(msg));
	memset(projectDescription, 0, 100);
	get(sock, projectDescription, 100);

	memset(msg, 0, sizeof(msg));
	strcpy(msg, "Project Date");
	put(sock, msg, sizeof(msg));
	memset(projectDate, 0, 8);
	get(sock, projectDate, 8);


	memset(msg, 0, sizeof(msg));
	strcpy(msg, "Project Due Date");
	put(sock, msg, sizeof(msg));
	memset(projectDueDate, 0, 8);
	get(sock, projectDueDate, 8);

	memset(msg, 0, sizeof(msg));
	strcpy(msg, "Num Users");
	put(sock, msg, sizeof(msg));
	get(sock, &numIn, sizeof(int));
	*numUsers = ntohl(numIn);
}

unsigned int sendMenuAuth(int clntSocket)
{
    struct menu projectMenu;
    unsigned int response = 0;
    memset(&projectMenu, 0, sizeof(struct menu));   /* Zero out structure */
    strcpy(projectMenu.line1,"1)Add\n");
    strcpy(projectMenu.line2, "2)Edit\n");
    strcpy(projectMenu.line3, "3)Quit\n");
    strcpy(projectMenu.line4, "4)Search\n");
	strcpy(projectMenu.line5, "5)Delete\n");
    printf("Sending menu\n");
    put(clntSocket, &projectMenu, sizeof(struct menu));
    get(clntSocket, &response, sizeof(unsigned int));
    return ntohl(response);	
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
    memset(fullname, 0, MAX_SIZE);
    get(sock, fullname, MAX_SIZE);
}

void askForUsernameSignup(int sock, char * username, unsigned int size)
{
    unsigned char msg[21];
    memset(msg, 0, sizeof(msg));
    strcpy(msg, "Enter username\n");
    put(sock, msg, sizeof(msg));
    memset(username, 0, MAX_SIZE);
    get(sock, username, MAX_SIZE);
}
void askForPasswordSignup(int sock, char * password, unsigned int size)
{
    unsigned char msg[21];
    memset(msg, 0, sizeof(msg));
    strcpy(msg, "Enter password\n");
    put(sock, msg, sizeof(msg));
    memset(password, 0, MAX_SIZE);
    get(sock, password, MAX_SIZE);
}


void askForUsernameLogin(int sock, char * username_l, unsigned int size)
{
    unsigned char msg[21];
    memset(msg, 0, sizeof(msg));
    strcpy(msg, "Enter username\n");
    put(sock, msg, sizeof(msg));
    memset(username_l, 0, MAX_SIZE);
    get(sock, username_l, MAX_SIZE);
}
void askForPasswordLogin(int sock, char * password_l, unsigned int size)
{
    unsigned char msg[21];
    memset(msg, 0, sizeof(msg));
    strcpy(msg, "Enter password\n");
    put(sock, msg, sizeof(msg));
    memset(password_l, 0, MAX_SIZE);
    get(sock, password_l, MAX_SIZE);
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
	
	char passCheck[MAX_SIZE];
	char userCheck[MAX_SIZE];
	
	
	char line[MAX_SIZE];
	while(fgets(line, sizeof(line), fp) != NULL)
	{
		sscanf(line, "%s %s", userCheck, passCheck);
		if(strcmp(userCheck, userAuth) == 0 && strcmp(passCheck, passAuth) == 0)	
		{
			fclose(fp);	
			return 1;
			break; 		
		}
	}
}

void displayList(DataInList *head)
{
	printf("Projects\n");
	while(head != NULL)
	{
		printf("Name:%s Description:%s Date:%s Due Date:%s Users:%d\n\n", head->projectname, head->projectdescription, head->date, head->duedate, head->numusers);
		head = head->next;
		

	}
	printf("\n");
}



void sendList(int socket, DataInList *head, char * search)
{
	while(head != NULL)
	{
 		
	if(strcmp(search, head->projectname) == 0)
	{	
		testList(head->projectname,head->projectdescription, head->date, head->duedate, 
		head->numusers, socket);
	}
		
		head = head->next;
	}
}

void writeList(DataInList *head)
{
	while(head != NULL)
	{
		toFileList(head->projectname, head->projectdescription, head->date, head->duedate, head->numusers);
		head = head->next;
		
	}


}

void toFileList(char * name, char * des, char * date, char * due, int num)
{
	printf("%s %s %s %s %d", name, des, date, due, num);

	FILE * fp;
	fp = fopen("projects.txt", "w+");
	fprintf(fp, "%s %s %s %s %d\n", name, des, date, due, num);
	fclose(fp);
}

void testList(char * name, char * des, char * date, char * due, int num, int sock)
{
	printf("%s %s %s %s %d", name, des, date, due, num);

	char msg[21];

   struct project projectDisplay;
   memset(&projectDisplay, 0, sizeof(struct project));  
   get(sock, msg, sizeof(msg));
   printf("%s", msg);

   strcpy(projectDisplay.line1,name);
   strcpy(projectDisplay.line2, des);
   strcpy(projectDisplay.line3, date);
   strcpy(projectDisplay.line4, due);
   projectDisplay.line5 = num;
   put(sock, &projectDisplay, sizeof(struct project));
}



void deleteItem(DataInList *head, char *name)
{
	DataInList * current  = head;
	DataInList * prev = NULL;
	
	if(current == NULL)
	{
	}
	else
	{	
		while(current != NULL)
		{
			if(strcmp(current->projectname, name) == 0)
			{
				printf("Deleting: project with name: %s", current->projectname);
				if(prev == NULL){
				head = current->next;
				}
				else {
					prev->next = current-> next;
				} 
			}
			prev = current;
			current = current ->next;
				
		}
	}
}
	
void put(int sock, void * buffer, unsigned int size)
{
    if (send(sock, buffer, size, 0) != size)
        DieWithError("send() failed");
}

