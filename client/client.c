#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <arpa/inet.h>
#include "rdwrn.h"
#define PORT 50031 // Required port 

//function definitions
void user_handler(int);
int userInputHandler();
void message_sender(int,char*,size_t);
void controller(int, char*);
void getSystemInfo(int, char*);
void fileCopy_handler(int, char*);
void signal_handler(int);

//structs definition
typedef struct {
    char sysname[100];
    char release[100];
    char version[100];
} uts;
struct timeval tv;
struct timeval tv1;

//global variables
int connfd;

/*
 * The main method creates the socket
 * Assigns the address to the socket
 * Connects to the server
 * And redirects user to the to be hanlded by other function
 */
int main(void)
{

    //definition of time to measure elapsed time	
    gettimeofday(&tv, NULL);
    struct sigaction sa;

    //signal handler set up
    sa.sa_handler = &signal_handler;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error: cannot handle SIGINT");
    }


    int sockfd = 0;
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error - could not create socket");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;


    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");


    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)  {
        perror("Error - connect failed");
        exit(1);
    } else
        printf("Connected to server...\n");
    connfd = sockfd;

    while(1) {
        user_handler(sockfd);
    }



    close(sockfd);

    exit(EXIT_SUCCESS);
}// end main()


/*
 * handles user
 * prints MENU to the user
 * pronpts user for interaction and validates user input using userInputHandler()
 * And directs each action to the appropriate function
 */
void user_handler(int socket)
{

    do
    {   int option;
        printf("_______________________________________________________________________\n");
        printf("\033[1;34m");
        printf("                             _____________________");
        printf("\n                               APPLICATION MENU\n");
        printf("                             _____________________");
        printf("\n          ENTER 1 to get Name, StudentID and IP address       \n");
        printf("          ENTER 2 to generate 5 random numbers between 0 and 1000\n");
        printf("          ENTER 3 to display the “uname” information of the server\n");
        printf("          ENTER 4 to display a list of files on the upload directory\n");
        printf("          ENTER 5 to copy the contents of a specific file\n");
        printf("          ENTER 6 to END CONNECTION\n");
        printf("\nEnter your option:\n");
        option = userInputHandler();

        switch (option) {
        case 1: {
            char* option1 = "1\n";
            controller(socket, option1);
            break;
        }
        case 2: {
            char* option2 = "2\n";
            controller(socket, option2);
            break;
        }
        case 3: {
            char* option3 = "3\n";
            getSystemInfo(socket, option3);
            break;
        }
        case 4: {
            char* option4 = "4\n";
            controller(socket, option4);
            break;
        }
        case 5: {
            char* option5 = "5\n";
            fileCopy_handler(socket, option5);
            break;
        }
        case 6: {
            char msg[10]="0\n";
            size_t size = sizeof(msg)+1;
            message_sender(socket,msg,size);
            printf("Client disconnected\n");
            exit(0);
        }
        default:
            printf("wrong Input\n");
        }
    } while(1);

}

/*
 * validates user input by checking if the input is a digit
 * validates user input by checking if the input is a single letter
 */
int userInputHandler()
{
    int size,i;
    char userInput[5];

    scanf ("%s", userInput);
    size = strlen (userInput);
    for (i=0; i<size; i++)
        if (!isdigit(userInput[i]))
        {
            return 0;
        }
    int value = atoi(userInput);
    return (value);

}

/*
 * Sends and receives strings through sockets
 * makes use of the message_sender function to send strings/messages
 * reads from server and displays the response to the user
 */
void controller(int socket, char* msg)
{

    size_t size = strlen(msg) + 1;
    message_sender(socket,msg,size);
    char resp[1024] =  {0};
    size_t size1;
    int error =
        readn(socket, (unsigned char *) &size1, sizeof(size_t));
    if (error<0) {
        perror("CLIENT FAILED\n");
    }
    int error1=
        readn(socket, (unsigned char *) resp, size1);
    if (error1<0) {
        perror("CLIENT FAILED\n");
    }
    printf("\nDATARECEIVED: %zu BYTES\n\n", size1);
    printf("RESPONSE:\n%s\n\n", resp);
}

/*
 * Function sends messages through socket
 * reduces complexity and redundancy of other functions by having a single function that sends strings
 * Gets the message size to display on the server
 */
void message_sender(int socket,char* msg,size_t size) {
    int error =
        writen(socket, (unsigned char *) &size, sizeof(size_t));
    if (error < 0) {
        perror("CLIENT FAILED TO SEND MESSAGE\n");
        exit(EXIT_FAILURE);
    }
    int error1 =
        writen(socket, (unsigned char *) msg, size);
    if (error1 < 0) {
        perror("CLIENT FAILED TO SEND MESSAGE\n");
        exit(EXIT_FAILURE);
    }
}


/*
 * Receives a populated UTS structure from the server
 * The structure contains server system data
 * Displays to the user system name, release version
 */
void getSystemInfo(int socket, char* msg)
{
    size_t p = strlen(msg) + 1;
    message_sender(socket,msg,p);
    printf("SERVER NOTIFIED\n");
    uts *structure = (uts *) malloc(sizeof(uts));
    // get utsname the struct
    size_t payload_length;
    size_t n =readn(socket, (unsigned char *) &payload_length, sizeof(size_t));
    printf("payload_length is: %zu (%zu bytes)\n\n", payload_length, n);
    n = readn(socket, (unsigned char *) structure, payload_length);
    // print out details of received & altered struct
    printf("SYSTEM INFORMATION\n");
    printf("==================\n");
    printf("SYSTEM NAME: %s\n", structure->sysname);
    printf("    RELEASE: %s\n", structure->release);
    printf("    VERSION: %s\n\n", structure->version);
}

/*
 * used to copy file from server upload directory to client's current dir
 * prompts user to enter file name and waits server's signal
 * it validates the file name by checking if the file exists in the upload folder and notifies the client to display to user if file doesn't exist
 * if file exists file exists the client is notified
 * a file pointer is created and opens file in write mode and receives data in chunks of 256 bytes
 * checks for last chunck if is less than 256
 * once finished notifies the client
 */
void fileCopy_handler(int socket, char* msg)
{
    char resp[1024] =  {0};

    size_t size = strlen(msg) + 1;
    message_sender(socket,msg,size);
    printf("SERVER NOTIFIED\n");


    char fileName[80];
    printf("\nEnter file name:\n");
    scanf("%s", fileName);

    size_t size2 = strlen(fileName) + 1;
    message_sender(socket,fileName,size2);

    size_t size3;
    int err10 =
        readn(socket, (unsigned char *) &size3, sizeof(size_t));
    if (err10<0) {
        perror("CLIENT FAILED\n");
    }
    int err11=
        readn(socket, (unsigned char *) resp, size3);
    if (err11<0) {
        perror("CLIENT FAILED\n");
    }
    printf("\nResponse data received: %zu bytes\n", size3);

    int request = atoi(resp);
    if (request == 1) {
        printf("FILE DETECTED!\n");
        char recvBuff[256];
        FILE *file;
        file = fopen(fileName, "w");
        if(NULL == file)
        {
            printf("UNABLE TO OPEN FILE");
            return 1;
        }
        int data = 0;

        while((data = read(socket, recvBuff, 256)) > 0)
        {
            fprintf(file,"%s",recvBuff);
            if(data < 256)
            {
                break;
            }
        }
        fclose(file);
        printf("The file %s has been copied!\n", fileName);
    } else {
        perror("No such file in the directory\n");
    }
}

/*
 * Triggered by Ctrl+C SIGINT action by the user
 * makes use of sigaction structure created in the main method
 * when the user press "Ctrl+C" signal, the function is called by the sigaction struct
 * checks for signal type and notifies user that a signal has been is caught
 * it notifies the server that the client is disconnecting from the server
 * prints the execution time which is stored in gettimeofday() and exits
 * cleans up and exits
 */
void signal_handler(int signal)
{
    if (signal == SIGINT) {
        gettimeofday(&tv1, NULL);
        char msg[10] = "0\n";
        size_t i = sizeof(msg)+1;
        printf("\n\nCaught SIGINT ACTION => DISCONNECTING FROM SERVER! \n");
        message_sender(connfd,msg,i);
        printf("TOTAL EXECUTION TIME: %.2f SECONDS\n",difftime(tv1.tv_sec, tv.tv_sec ));

        close(connfd);
        exit(0);
    }
}
