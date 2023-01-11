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


typedef struct ClientNode
{
    int socket;
    struct ClientNode *prev;
    struct ClientNode *link;
    char ip[16];
    char name[31];
} ClientList;

ClientList *newNode(int sockfd, char *ip)
{
    ClientList *np = (ClientList *)malloc(sizeof(ClientList));
    np->socket = sockfd;
    np->prev = NULL;
    np->link = NULL;
    strcpy(np->ip, ip);
    strcpy(np->name, "NULL");
    return np;
}

// Global variables
ClientList *root, *now;
#define LENGTH_NAME 31
#define LENGTH_MSG 101
#define LENGTH_SEND 201

void sig_chld(int sig)
{
    ClientList *tmp;
    while (root != NULL)
    {
        printf("\nClose socketfd: %d\n", root->socket);
        close(root->socket); // close all socket include server_sockfd
        tmp = root;
        root = root->link;
        free(tmp);
    }
    printf("Bye\n");
    exit(EXIT_SUCCESS);
}

void sendMessage(ClientList *np, char tmp_buffer[])
{
    ClientList *tmp = root->link;
    printf("Sending to all client.....\n");
    while (tmp != NULL)
    {
        if (np->socket != tmp->socket)
        {
            send(tmp->socket, tmp_buffer, LENGTH_SEND, 0);
        }
        tmp = tmp->link;
    }
}

void clientHandler(void *client_X)
{
    int leave_flag = 0;
    char nickname[LENGTH_NAME] = {};
    char recv_buffer[LENGTH_MSG] = {};
    char send_buffer[LENGTH_SEND] = {};
    ClientList *np = (ClientList *)client_X;

    // Naming
    if (recv(np->socket, nickname, LENGTH_NAME, 0) <= 0 || strlen(nickname) < 1 || strlen(nickname) >= LENGTH_NAME - 1)
    {
        printf("%s didn't input name.\n", np->ip);
        leave_flag = 1;
    }
    else
    {
        strncpy(np->name, nickname, LENGTH_NAME);
        printf("%s join the chatroom.\n", np->name);
        sprintf(send_buffer, "%s join the chatroom.", np->name);
        sendMessage(np, send_buffer);
    }

    // Conversation
    while (1)
    {
        if (leave_flag)
        {
            break;
        }
        int receive = recv(np->socket, recv_buffer, LENGTH_MSG, 0);
        if (receive > 0)
        {
            if (strlen(recv_buffer) == 0)
            {
                continue;
            }
            sprintf(send_buffer, "%s：%s", np->name, recv_buffer);
        }
        else if (receive == 0 || strcmp(recv_buffer, "exit") == 0)
        {
            printf("%s leave the chatroom.\n", np->name);
            sprintf(send_buffer, "%s leave the chatroom.", np->name);
            leave_flag = 1;
        }
        else
        {
            printf("Fatal Error: -1\n");
            leave_flag = 1;
        }
        sendMessage(np, send_buffer);
    }

    // Remove Node
    close(np->socket);
    if (np == now)
    { // remove an edge node
        now = np->prev;
        now->link = NULL;
    }
    else
    { // remove a middle node
        np->prev->link = np->link;
        np->link->prev = np->prev;
    }
    free(np);
}

int main()
{
    int server_sockfd = 0;
    int sin_size;

	signal(SIGCHLD, sig_chld);

    // Create socket
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sockfd == -1)
    {
        printf("Fail to create a socket.");
        exit(EXIT_FAILURE);
    }
    else
        printf("Server socket created successfully\n");

    // Socket information
    struct sockaddr_in server_info, client_info;

    int s_addrlen = sizeof(server_info);
    int c_addrlen = sizeof(client_info);

    memset(&server_info, 0, s_addrlen);
    memset(&client_info, 0, c_addrlen);
    server_info.sin_family = AF_INET;
    server_info.sin_addr.s_addr = INADDR_ANY;
    server_info.sin_port = htons(5500);

    // bind the socket to the specified IP addr and port
    if (bind(server_sockfd, (struct sockaddr *)&server_info, s_addrlen) != 0)
    {
        printf("socket bind failed...\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("Socket successfully binded..\n");

    // Listen
    if (listen(server_sockfd, 20) != 0)
    {
        printf("Listen failed...\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("Server listening..\n");

    // Print Server IP
    getsockname(server_sockfd, (struct sockaddr *)&server_info, (socklen_t *)&s_addrlen);
    printf("Start Server on: %s:%d\n", inet_ntoa(server_info.sin_addr), ntohs(server_info.sin_port));

    // Initial linked list for clients
    root = newNode(server_sockfd, inet_ntoa(server_info.sin_addr));
    now = root;

    int no_threads = 0;
    pthread_t threads[20];
    while (no_threads < 20)
    {
        sin_size = sizeof(struct sockaddr_in);

        int client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_info, &sin_size);
        puts("Connection accepted");

        getpeername(client_sockfd, (struct sockaddr *)&client_info, (socklen_t *)&c_addrlen);
        printf("Client %s:%d come in.\n", inet_ntoa(client_info.sin_addr), ntohs(client_info.sin_port));

        ClientList *c = newNode(client_sockfd, inet_ntoa(client_info.sin_addr));
        c->prev = now;
        now->link = c;
        now = c;

        if (pthread_create(&threads[no_threads], NULL, (void *)clientHandler, (void *)c) < 0)
        {
            perror("Could not create thread");
            return 1;
        }
        if (client_sockfd < 0)
        {
            printf("server acccept failed...\n");
            exit(EXIT_FAILURE);
        }
        else
            printf("Server acccept the client...\n");
        puts("Handler assigned");
        no_threads++;
    }

    int k = 0;
    for (k = 0; k < 20; k++)
    {
        pthread_join(threads[k], NULL);
    }

    close(server_sockfd);

    return 0;
}