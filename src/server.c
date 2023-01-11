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
#include <stdint.h>
#include "caesar.h"

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 20

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

/* Thread-specific data */
pthread_key_t key_seed;
int key = 0;

/* Thread function for generating key */
void *keyGeneration(void *args);

void sig_chld(int sig);

void sendAllOtherClients(ClientList *np, char tmp_buffer[]);

void sendAllClients(char buffer[]);

/* Thread function for handling clients */
void handleClient(void *client_X);

int main(int argc, char *argv[])
{
    int server_sockfd = 0;
    int sin_size;
    pthread_t threads[MAX_CLIENTS], key_thread;

    if (argc < 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return 0;
    }

    /* Create thread-specific data key */
    pthread_key_create(&key_seed, NULL);

    /* Create key generation thread */
    pthread_create(&key_thread, NULL, keyGeneration, NULL);

    signal(SIGCHLD, sig_chld);

    /* Create a socket */
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
    server_info.sin_port = htons(atoi(argv[1]));

    /* Bind the socket to an IP and port */
    if (bind(server_sockfd, (struct sockaddr *)&server_info, s_addrlen) != 0)
    {
        printf("socket bind failed...\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("Socket successfully binded..\n");

    /* Listen for incoming connections */
    if (listen(server_sockfd, MAX_CLIENTS) != 0)
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
    while (no_threads < MAX_CLIENTS)
    {
        sin_size = sizeof(struct sockaddr_in);

        /* Accept incoming connections */
        int client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_info, (socklen_t *)&sin_size);
        puts("Connection accepted");

        getpeername(client_sockfd, (struct sockaddr *)&client_info, (socklen_t *)&c_addrlen);
        printf("Client %s:%d come in.\n", inet_ntoa(client_info.sin_addr), ntohs(client_info.sin_port));

        ClientList *c = newNode(client_sockfd, inet_ntoa(client_info.sin_addr));
        c->prev = now;
        now->link = c;
        now = c;

        /* Create a new thread to handle the client */
        if (pthread_create(&threads[no_threads], NULL, (void *)handleClient, (void *)c) < 0)
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

        /* Increment the client counter */
        no_threads++;
    }

    int k = 0;
    for (k = 0; k < 20; k++)
    {
        pthread_join(threads[k], NULL);
    }

    /* Close the server socket */
    close(server_sockfd);

    return 0;
}

/* Thread function for generating key */
void *keyGeneration(void *args)
{
    /* Create seed specific to this thread */
    unsigned int seed = (unsigned int)time(0);
    pthread_setspecific(key_seed, &seed);
    while (1)
    {
        /* Generate a random key between 1 and 25 */
        key = rand() % 25 + 1;
        printf("Generated key: %d\n", key);

        /* Convert key(int) to str(string) */
        char str[10];
        sprintf(str, "%d", key);

        /* Send key to all connected clients */
        printf("Sending KEY to all client.....\n");
        sendAllClients(str);

        /* Sleep for 20 seconds */
        struct timespec ts = {20, 0};
        nanosleep(&ts, NULL);
    }
    return NULL;
}

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

void sendAllOtherClients(ClientList *np, char tmp_buffer[])
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

void sendAllClients(char buffer[])
{
    ClientList *tmp = root->link;
    while (tmp != NULL)
    {
        send(tmp->socket, buffer, LENGTH_SEND, 0);
        tmp = tmp->link;
    }
}

/* Thread function for handling clients */
void handleClient(void *client_X)
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

        /* Convert key(int) to str(string) */
        char str[10];
        sprintf(str, "%d", key);

        /* Send key to all connected clients */
        send(np->socket, str, LENGTH_SEND, 0);

        sprintf(send_buffer, "%s join the chatroom.", np->name);
        printf("%s\n", send_buffer);
        encrypt_caesar(send_buffer, key);
        sendAllOtherClients(np, send_buffer);
    }

    // Conversation
    while (1)
    {
        if (leave_flag)
            break;

        /* Receive message from client */
        int receive = recv(np->socket, recv_buffer, LENGTH_MSG, 0);

        /* Check for errors */
        if (receive > 0)
        {
            if (strlen(recv_buffer) == 0 || strcmp(recv_buffer, "exit") == 0)
            {
                continue;
            }
            strcpy(nickname, np->name);
            encrypt_caesar(nickname, key);
            sprintf(send_buffer, "%s: %s", nickname, recv_buffer);
        }
        else if (receive == 0 || strcmp(recv_buffer, "exit") == 0)
        {
            printf("%s leave the chatroom.\n", np->name);
            sprintf(send_buffer, "%s leave the chatroom.", np->name);
            encrypt_caesar(send_buffer, key);
            leave_flag = 1;
        }
        else
        {
            printf("Fatal Error: -1\n");
            leave_flag = 1;
        }

        /* Send the encrypted/decrypted message back to the all clients */
        sendAllOtherClients(np, send_buffer);
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