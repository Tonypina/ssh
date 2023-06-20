/**
 * @file servidor.c
 * @author Piña Rossette Marco Antonio
 * @brief Servidor de terminal remota (Linux)
 * @version 1.0
 * @date 2023-06-19
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define LENGTH 20000
#define SIGFIN "SIGFIN\0"

int main(int argc, char *argv[]) {
    int numbytes;
    char buf[100];

    int server_fd, cliente_fd;
    struct sockaddr_in servidor;
    struct sockaddr_in cliente;
    int sin_size_servidor;
    int sin_size_cliente;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == -1) {
        perror("Server-setsockopt() error!");
        exit(1);
    } else {
        printf("Server-setsockopt is OK...\n");
    }

    servidor.sin_family = AF_INET;
    servidor.sin_port = htons(atoi(argv[1]));
    servidor.sin_addr.s_addr = INADDR_ANY;
    memset(&(servidor.sin_zero), '\0', 8);

    sin_size_servidor = sizeof(servidor);
    if (bind(server_fd, (struct sockaddr *)&servidor, sin_size_servidor) == -1) {
        perror("bind");
        exit(1);
    }

    while (1) {

        if (listen(server_fd, 1) == -1) {
            perror("listen");
            exit(1);
        }

        sin_size_cliente = sizeof(cliente);

        if ((cliente_fd = accept(server_fd, (struct sockaddr *)&cliente, &sin_size_cliente)) == -1) {
            perror("accept");
            exit(1);
        }

        printf("server: conexion cliente desde %s\n", inet_ntoa(cliente.sin_addr));

        while (1) {
            memset(buf, '\0', 100);

            if ((numbytes = recv(cliente_fd, buf, 100-1, 0)) == -1) {
                perror("recv");
                exit(1);
            }

            if (strcmp("exit", buf) == 0) {
                char *msg_salida = "[Server] Saliendo...";

                if (send(cliente_fd, msg_salida, strlen(msg_salida)-1, 0) < 0) {
                    printf("ERROR: al enviar la salida del comando al cliente\n");
                    exit(1);
                } else {
                    printf("[Server] El cliente salió.\n");
                    break;
                }
            }

            int pipefd[2];
            pid_t pid;

            if (pipe(pipefd) == -1) {
                perror("pipe");
                exit(1);
            }

            pid = fork();
            if (pid == -1) {
                perror("fork");
                exit(1);
            
            } else if (pid == 0) {  // Proceso hijo
                close(pipefd[0]);  // Cerramos el extremo de lectura del pipe

                // Redireccionamos la salida estándar al extremo de escritura del pipe
                if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(1);
                }

                // Ejecutamos el comando usando exec
                if (execlp("/bin/sh", "sh", "-c", buf, NULL) == -1) {
                    perror("execlp");
                    char *error_msg = "Comando no encontrado o no se pudo ejecutar.\n";
                    write(pipefd[1], error_msg, strlen(error_msg));
                    close(pipefd[1]);
                    exit(1);
                }

            } else {  // Proceso padre
                close(pipefd[1]);  // Cerramos el extremo de escritura del pipe

                printf("[Server] Ejecutando comando...\n");

                // Leemos la salida del comando del extremo de lectura del pipe
                char sdbuf[LENGTH];
                int total_bytes = 0;
                int bytes_read;

                while ((bytes_read = read(pipefd[0], sdbuf + total_bytes, LENGTH - total_bytes)) > 0) {
                    total_bytes += bytes_read;
                }

                if (bytes_read == -1) {
                    perror("read");
                    exit(1);
                }

                close(pipefd[0]);

                printf("[Server] Enviando salida al Cliente...\n");

                // Enviamos la salida al cliente
                int bytes_sent = 0;
                int bytes_remaining = total_bytes;

                if (total_bytes == 0) {
                    char *error_msg = "Comando no encontrado o no se pudo ejecutar.\n";
                    bytes_remaining = strlen(error_msg);
                    strncpy(sdbuf, error_msg, LENGTH);
                }

                while (bytes_remaining > 0) {
                    bytes_sent = send(cliente_fd, sdbuf + bytes_sent, bytes_remaining, 0);
                    if (bytes_sent == -1) {
                        perror("send");
                        exit(1);
                    }
                    bytes_remaining -= bytes_sent;
                }

                printf("[Server] Respuesta enviada!\n");
            }
        }
    
    }
    
    close(server_fd);
    close(cliente_fd);
    shutdown(server_fd, SHUT_RDWR);

    exit(0);
}
