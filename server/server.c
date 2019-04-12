#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <sys/utsname.h>
#include "rdwrn.h"
#define PORT 50031 //as required
#define MAX 80

/*
 * SOURCES USED
 * https://www.geeksforgeeks.org/measure-execution-time-with-high-precision-in-c-c/
 * https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.3.0/com.ibm.zos.v2r3.bpxbd00/rtuna.htm
 * https://gist.github.com/aspyct/3462238
 */

//function definitions to be used in the server
void *client_handler(void *);
void displayStudentDetails_handler(int);
void randomNumberGenerator_handler(int);
void systemInfo_handler(int);
void fileList_handler(int);
void fileCopy_handler(int);
void signal_handler(int signal);
int getRandom(int,int);
void message_sender(int,char*,size_t);


//structs definition
typedef struct {
    char sysname[100];
    char release[100];
    char version[100];
}
uts;
struct timeval tv;
struct timeval tv1;

//gloabla variables
char* address;
int connfd;

/*
 * The main function in the server side handles four main functionalities: of the server
 * 1. Setting the server socket
 * 2. Listen to the client after binding operation is finished
 * 3. Handles user requests when the connection has been established with the client
 * 4. Close thread once connection with client ENDS
 */
int main(void)
{

    //definition of time to measure elapsed time
    gettimeofday(&tv, NULL);
    struct sigaction sa;

    // signal handler set up
    sa.sa_handler = &signal_handler;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error: cannot handle SIGINT");
    }

    int listenfd = 0, connfd = 0;

    struct sockaddr_in serv_addr;
    struct sockaddr_in client_addr;
    socklen_t socksize = sizeof(struct sockaddr_in);
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);

    bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

    if (listen(listenfd, 10) == -1) {
        perror("Failed to listen");
        exit(EXIT_FAILURE);
    }
    // end socket setup

    //Accept and incoming connection
    puts("** Waiting for incoming connections...");
    while (1) {
        printf("** Waiting for a client to connect...\n");
        connfd =
            accept(listenfd, (struct sockaddr *) &client_addr, &socksize);
        printf("** Connection accepted...\n");
        address = inet_ntoa(client_addr.sin_addr); //client address assigned to global variable to be accessible
        pthread_t sniffer_thread;

        if (pthread_create
                (&sniffer_thread, NULL, client_handler,(void *) &connfd) < 0) {
            perror("could not create thread");
            exit(EXIT_FAILURE);
        }
        //Now join the thread , so that we dont terminate before the thread
        pthread_join( sniffer_thread , NULL);
        printf("** Handler assigned\n");
    }
    exit(EXIT_SUCCESS);
} // end main()


/*
 * creates an instance of all the clients which it has established a connection with
 * receives requests from clients and handles by assigning the appropriate function to handle the request
 * keeps listening to client requests
 * closes the connection when a client leaves
 */
void *client_handler(void *socket_desc)
{
    //Get the socket descriptor
    int connfd = *(int *) socket_desc;
    do {
        char resp[1024] =  {0};
        
        printf("\nWAITING FOR REQUESTS FROM %s\n",address);
        size_t size;
        int error =
            readn(connfd, (unsigned char *) &size, sizeof(size_t));
        if (error<0) {
            perror("Client read failed\n");
        }
        int error1=
            readn(connfd, (unsigned char *) resp, size);
        if (error1<0) {
            perror("Client read failed\n");
        }
        printf("\n** Data received: %zu bytes\n", size);
        printf("** Processing request number %s\n", resp);
        int request = atoi(resp);
        switch (request) {
        case 1: {
            displayStudentDetails_handler(connfd);
            break;
        }
        case 2: {
            randomNumberGenerator_handler(connfd);
            break;
        }
        case 3: {

            systemInfo_handler(connfd);
            break;
        }
        case 4: {
            fileList_handler(connfd);
            break;
        }
        case 5: {
            fileCopy_handler(connfd);
            break;
        }
        default:
            return;
        }
    } while(1);


    shutdown(connfd, SHUT_RDWR);
    close(connfd);
    printf("** Thread %lu exiting\n", (unsigned long) pthread_self());

    shutdown(connfd, SHUT_RDWR);
    close(connfd);

    return 0;
}  // end client_handler()

/*
 * Function sends messages through socket
 * reduces complexity and redundancy of other functions by having a single function that sends strings
 * Gets the message size to display on the server
 */
void message_sender(int socket,char* msg,size_t size) {
    int error =
        writen(socket, (unsigned char *) &size, sizeof(size_t));
    if (error < 0) {
        perror("Client write failed\n");
        exit(EXIT_FAILURE);
    }
    int error1 =
        writen(socket, (unsigned char *) msg, size);
    if (error1 < 0) {
        perror("Client write failed\n");
        exit(EXIT_FAILURE);
    }

}


/*
 * client_handler() service function
 * used by the client_handle() to send a string containing name, student ID and IP address
 * obtains the ip address from gloabl variable "address"
 * makes use of message_sender() to send strings to the client
 */
void displayStudentDetails_handler(int socket)
{
    char msg[100] = "\n      NAME: Azeite Chaleca \nSTUDENT_ID: S1719009 \nIP_address: ";
    strcat(msg, address);
    size_t size = strlen(msg) + 1;
    message_sender(socket,msg,size);
    printf("REQUEST HANDLED SUCCESSFULLY!\n");
}


/*
 * client_handler() service function
 * sends 5 random numbers between 0-1000
 * it uses the number getRandom function to generate the numbers
 * sends the numbers in a string format using the message_sender() function
 */
void randomNumberGenerator_handler(int socket)
{
    int first = 0;
    int last = 1000;
    int count = 5;
    int numbers[count];
    char msg[200];
    int i;
    for (i = 0; i < count; i++) {
        int num = getRandom(first,last);
        numbers[i]= num;
    }
    sprintf(msg, "The 5 random numbers between %d and %d are: \n%d , %d , %d , %d and %d\n"
            ,first,last,numbers[0],numbers[1],numbers[2],numbers[3],numbers[4]);

    size_t size = strlen(msg) + 1;
    message_sender(socket,msg,size);
    printf("REQUEST HANDLED SUCCESSFULLY!\n");
}


/*
 * randomNumberGenerator_handler() service method
 * getRandom is generates random numbers
 * it gets a low number and high number and uses to generate different numbers
 */
int getRandom(int first, int last)
{
    int num = (rand() %
               (last - first + 1)) + first;
    return (num);
}


/*
 * client_handler() service function
 * used to display system details
 * uses a utsname structure provided by <sys/utsname.h> and populates the structure using uname function
 * the populated variables of the structure are then sent to the client as strings through sockets
 */
void systemInfo_handler(int socket)
{
    struct utsname systemInfo;
    uname(&systemInfo);
    uts *structure;
    structure = (uts *) malloc(sizeof(uts));
    strcpy(structure->sysname, systemInfo.sysname);
    strcpy(structure->release, systemInfo.release);
    strcpy(structure->version, systemInfo.version);
    size_t payload_length = sizeof(uts);

    int error =
        writen(socket, (unsigned char *) &payload_length, sizeof(size_t));
    if (error < 0) {
        perror("CLIENT FAILED\n");
        exit(EXIT_FAILURE);
    }
    int error1 =
        writen(socket, (unsigned char *)structure, payload_length);
    if (error1 < 0) {
        perror("CLIENT FAILED\n");
        exit(EXIT_FAILURE);
    }
    printf("REQUEST HANDLED SUCCESSFULLY!\n");
    free(structure);
}


/*
 * client_handler() service function
 * used to check files in the upload directory
 * creates a struct called list
 * scans in the current directory if there is a directory naed upload
 * checks if there are any files in the upload directory, if no files are found it notifies the client by simply printing a message
 * if files are found it appends using the strcat and sends the strings through sockets using the message_sender() function
 */
void fileList_handler(int socket)
{
    char msg[1024]= "UPLOAD DIRECTORY FILES: \n";

    struct dirent **list;
    int dir;

    if ((dir = scandir("./upload", &list, NULL, alphasort)) == -1)
    {
        char error[50]= "No such directory!";
        size_t size = strlen(error) + 1;
        message_sender(socket,error,size);
        perror("FAILED TO LOAD FILE");
    } else {
        if (chdir("./upload") == -1)
            perror("Failed to upload ");
        while (dir--) {
            struct stat sb;

            if (stat(list[dir]->d_name, &sb) == -1) {
                perror("stat");
                exit(EXIT_FAILURE);
            }
            if ((sb.st_mode & S_IFMT) == S_IFREG) {
                strcat(msg, list[dir]->d_name);
                strcat(msg, "\n");
            }

            free(list[dir]);
        }
        chdir("..");
        free(list);
        size_t m = strlen(msg) + 1;
        message_sender(socket,msg,m);
    }

    printf("REQUEST HANDLED SUCCESSFULLY!\n");
}

/*
 * client_handler() service function
 * used to make a copy of a file the content given file name
 * receives notification from client
 * waits for clients response with FILENAME
 * checks in the upload folder
 * it opens the file and checks if its able to read file content
 * if its able to read it starts copying file content
 * the content is copied in chunks of size 256 this means when the chunck size is less than 256 it means its the last chunk
 * otherwise it displays an error to the client that the file is not present in the upload folder using the message_sender()
 */
void fileCopy_handler(int socket)
{
    char resp[1024] =  {0};
    size_t size;
    int error =
        readn(socket, (unsigned char *) &size, sizeof(size_t));
    if (error<0) {
        perror("CLIENT FAILED\n");
    }
    int error1=
        readn(socket, (unsigned char *) resp, size);
    if (error1<0) {
        perror("FAILED\n");
    }
    printf("\nDATA RECEIVED: %zu BYTES\n", size);

    char filename[200] ="./upload/";
    strcat(filename,resp);

    FILE *file;
    if (file = fopen(filename, "r")) {
        printf("FILE DETECTED\n");
        char* msg = "1\n";
        size_t size1 = strlen(msg) + 1;
        message_sender(socket,msg,size1);

        while(1)
        {
            /* First read file in chunks of 256 bytes */
            unsigned char buff[256]= {0};
            int nread = fread(buff,1,256,file);

            /* If read was success, send data. */
            if(nread > 0)
            {
                write(socket, buff, nread);
            }

            if (nread < 256)
            {
                if (feof(file))

                    if (ferror(file))
                        printf("ERROR reading file\n");
                break;
            }

        }
        printf("PROCESSING FILE!\n");
        fclose(file);
        printf("FILE SENT!\n");
    } else {
        printf("** NO SUCH FILE <%s> in this directory\n",filename);
        char* msg1 = "2\n";
        size_t size2 = strlen(msg1) + 1;
        message_sender(socket,msg1,size2);

    }
}

/*
 * Triggered by Ctrl+C SIGINT action by the user
 * makes use of sigaction structure created in the main method
 * when the user press "Ctrl+C" signal, the function is called by the sigaction struct
 * checks for signal type and notifies user that a signal has been is caught
 * prints the execution time which is stored in gettimeofday() and exits
 * shuts down and closes any connections
 * cleans up and exits
 */
void signal_handler(int signal)
{
    sigset_t pending;
    if (signal == SIGINT)
        gettimeofday(&tv1, NULL);
    printf("\n\nCaught SIGINT ACTION => DISCONNECTING FROM SERVER! \n");
    printf("TOTAL EXECUTION TIME: %.2f SECONDS\n",difftime(tv1.tv_sec, tv.tv_sec ));
    printf("SHUTTING DOWN SERVER\n");
    shutdown(connfd, SHUT_RDWR);
    close(connfd);
    exit(0);
}
