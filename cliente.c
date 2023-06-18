/**
 * @file cliente.c
 * @author Piña Rossette Marco Antonio
 * @brief Cliente de servicio SSH
 * @version 0.1
 * @date 2023-06-16
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdbool.h>

#define MAXDATASIZE 100
#define MAX_BUFFER_SIZE 20000

int main(int argc, char *argv[])
{
    bool SEXIT = true;

    // Estos 2 son para el comando
    char comando[MAXDATASIZE];
    int len_comando;
    // Estos 2 son para la respuesta
    int numbytes;
    char buf[MAX_BUFFER_SIZE];
    int sockfd;  
    struct hostent *he;
    struct sockaddr_in cliente; // información de la dirección de destino 
    
    if (argc != 3) {
        fprintf(stderr,"usage: client hostname puerto\n");
        exit(1);
    }
    
    if ((he=gethostbyname(argv[1])) == NULL) {  // obtener información de host servidor 
        perror("gethostbyname");
        exit(1);
    }
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    
    cliente.sin_family = AF_INET;    // Ordenación de bytes de la máquina 
    cliente.sin_port = htons( atoi(argv[2]) );  // short, Ordenación de bytes de la red 
    cliente.sin_addr = *((struct in_addr *)he->h_addr);
    memset(&(cliente.sin_zero), '\0',8);  // poner a cero el resto de la estructura 
    
    if (connect(sockfd, (struct sockaddr *)&cliente, sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }
    
    
    while (1) {

        printf("\nServer$ ");

        /* Se lee el comando del teclado */
        fgets(comando,MAXDATASIZE-1,stdin);

        /* Se deja el ultimo caracter vacio */
        len_comando = strlen(comando) - 1;
        comando[len_comando] = '\0';

        /* Se envia el comando al server */
        if (send(sockfd,comando, len_comando, 0) == -1) {
            perror("send()");
            exit(1);
        }

        if (strcmp("exit", comando) == 0) {
            break;
        }

        // Si el send no devuelve error continua y lo que falta por hacer
        // es leer la respuesta
        if (numbytes = recv(sockfd, buf, MAX_BUFFER_SIZE - 1, 0) == -1) {
            perror("recv");
            exit(1);
        }

        printf("%s", buf);
        memset(buf, '\0', MAX_BUFFER_SIZE);

        buf[numbytes] = '\0';
    }
    
    // Si el recv no devuelve error continua y lo que falta por hacer
    // es cerrar el file descriptor del cliente
    close(sockfd);
    return 0;
} 