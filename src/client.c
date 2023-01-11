#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio_ext.h>
#include <sys/wait.h>
#include "caesar.h"

#define STRING_NAME_LENGTH 30
#define BUFF_SIZE 1024

#define TRUE 1
#define FALSE 0

// Global variables
volatile sig_atomic_t flag = 0;
int client_sock;                       // client socket
char nickname[STRING_NAME_LENGTH + 1]; // nickname
int caesarKey = 1;

/**
 * Config message
 */
void str_trim_lf(char *, int);

void str_overwrite_stdout();

/**
 * Exit on key function
 */
void sig_chld(int);

void recieveMessageHandler();

void sendMessageHandler();

int main(int argc, char *argv[])
{
    signal(SIGCHLD, sig_chld);

    // Step 0: Get data from command
    int SERVER_PORT;
    char SERVER_ADDR[BUFF_SIZE + 1];

    if (argc != 3)
    {
        printf("Wrong command!\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        SERVER_PORT = atoi(argv[2]);
        strcpy(SERVER_ADDR, argv[1]);
    }

    // Step 1: Construct socket
    struct sockaddr_in server_info, client_info;

    // Create socket
    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock == -1)
    {
        printf("Fail to create a socket.");
        exit(EXIT_FAILURE);
    }

    server_info.sin_family = AF_INET;
    server_info.sin_addr.s_addr = inet_addr(SERVER_ADDR);
    server_info.sin_port = htons(SERVER_PORT);

    // Step 3: Request to connect server
    if (connect(client_sock, (struct sockaddr *)&server_info, sizeof(struct sockaddr)) < 0)
    {
        printf("\nError!Can not connect to sever! Client exit imediately! ");
        exit(EXIT_FAILURE);
    }

    // Get user name
    printf("Please enter your nickname: ");
    memset(nickname, '\0', (strlen(nickname) + 1));
    fgets(nickname, BUFF_SIZE, stdin);
    nickname[strcspn(nickname, "\r\n")] = 0;

    if (strlen(nickname) < 1 || strlen(nickname) >= STRING_NAME_LENGTH)
    {
        printf("\nName length is between 1 to 30 characters.\n");
        exit(EXIT_FAILURE);
    }

    // Get socket addr
    int s_addrlen = sizeof(server_info);
    int c_addrlen = sizeof(client_info);
    memset(&server_info, 0, s_addrlen);
    memset(&client_info, 0, c_addrlen);

    getsockname(client_sock, (struct sockaddr *)&client_info, (socklen_t *)&c_addrlen);
    getpeername(client_sock, (struct sockaddr *)&server_info, (socklen_t *)&s_addrlen);
    printf("Connect to Server: %s:%d\n", inet_ntoa(server_info.sin_addr), ntohs(server_info.sin_port));
    printf("You are: %s:%d\n", inet_ntoa(client_info.sin_addr), ntohs(client_info.sin_port));

    // send nickname
    send(client_sock, nickname, strlen(nickname), 0);

    pthread_t send_msg_thread;
    if (pthread_create(&send_msg_thread, NULL, (void *)sendMessageHandler, NULL) != 0)
    {
        printf("Create pthread error!\n");
        exit(EXIT_FAILURE);
    }

    pthread_t recv_msg_thread;
    if (pthread_create(&recv_msg_thread, NULL, (void *)recieveMessageHandler, NULL) != 0)
    {
        printf("Create pthread error!\n");
        exit(EXIT_FAILURE);
    }

    while (TRUE)
    {
        if (flag)
        {
            printf("\nBye\n");
            break;
        }
    }

    close(client_sock);
    return 0;
}

void str_trim_lf(char *arr, int length)
{
    int i;
    for (i = 0; i < length; i++)
    { // trim \n
        if (arr[i] == '\n')
        {
            arr[i] = '\0';
            break;
        }
    }
}

void str_overwrite_stdout()
{
    printf("\r%s", "You: ");
    fflush(stdout);
}

void sig_chld(int sig)
{
    flag = 1;
}

void recieveMessageHandler()
{
    char receiveMessage[BUFF_SIZE];
    while (TRUE)
    {
        int receive = recv(client_sock, receiveMessage, BUFF_SIZE, 0);
        if (receive > 0)
        {
            int key = atoi(receiveMessage);
            if (key != 0)
            {
                caesarKey = key;
            }
            else
            {
                decrypt_caesar(receiveMessage, caesarKey);
                printf("\r%s\n", receiveMessage);
                str_overwrite_stdout();
            }
        }
        else if (receive == 0)
        {
            printf("Connection closed.\n");
        }
        else
            perror("\nError: ");
    }
}

void sendMessageHandler()
{
    char message[BUFF_SIZE];
    while (TRUE)
    {
        str_overwrite_stdout();
        while (fgets(message, BUFF_SIZE, stdin) != NULL)
        {
            str_trim_lf(message, BUFF_SIZE);
            if (strlen(message) == 0)
            {
                str_overwrite_stdout();
            }
            else
            {
                break;
            }
        }
        
        if (strcmp(message, "exit") == 0)
        {
            send(client_sock, message, BUFF_SIZE, 0);
            break;
        }
        else
        {
            encrypt_caesar(message, caesarKey);
            send(client_sock, message, BUFF_SIZE, 0);
        }
    }
    sig_chld(2);
}
