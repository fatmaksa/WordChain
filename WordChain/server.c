#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_CLIENTS 2
#define BUFFER_SIZE 1024

int is_valid_word(const char *word, const char *last_word) {
    if (last_word[0] != '\0' && word[0] != last_word[strlen(last_word) - 1]) {
        return 0;
    }
    return 1;
}

int main() {
    int server_fd, new_socket, client_sockets[MAX_CLIENTS] = {0};
    struct sockaddr_in address;
    char buffer[BUFFER_SIZE];
    fd_set readfds;
    int max_sd, activity, i, valread;
    socklen_t addrlen = sizeof(address);
    char last_word[BUFFER_SIZE] = "";

    // Soket oluþturma
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Adres ayarlama
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind iþlemi
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Dinlemeye baþlama
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is running, waiting for connections...\n");

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        for (i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (sd > 0) FD_SET(sd, &readfds);
            if (sd > max_sd) max_sd = sd;
        }

        // Aktif soketlerin kontrolü
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0) {
            perror("Select error");
            exit(EXIT_FAILURE);
        }

        // Yeni baðlantý
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }

            printf("New client connected: socket %d\n", new_socket);

            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }

        // Mevcut client'larýn kontrolü
        for (i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];

            if (FD_ISSET(sd, &readfds)) {
                // Mesaj alýndýysa
                if ((valread = read(sd, buffer, BUFFER_SIZE)) == 0) {
                    close(sd);
                    client_sockets[i] = 0;
                    printf("Client %d disconnected.\n", sd);
                } else {
                    buffer[valread] = '\0';
                    printf("Client %d sent: %s\n", sd, buffer);

                    if (!is_valid_word(buffer, last_word)) {
                        // Geçersiz kelime -> Diskalifiye
                        printf("Client %d disqualified.\n", sd);
                        send(sd, "You are disqualified!\n", strlen("You are disqualified!\n"), 0);
                        close(sd);
                        client_sockets[i] = 0;

                        // Diðer client'a kazandý mesajý gönder
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (client_sockets[j] != 0) {
                                send(client_sockets[j], "You win!\n", strlen("You win!\n"), 0);
                                printf("Client %d won the game.\n", client_sockets[j]);  // Kazanan yazdýrýlýyor
                            }
                        }
                    } else {
                        // Geçerli kelime -> Son kelimeyi güncelle
                        strcpy(last_word, buffer);

                        // Mesajý yalnýzca diðer client'a gönder
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (client_sockets[j] != 0 && client_sockets[j] != sd) {
                                char msg[BUFFER_SIZE];
                                snprintf(msg, BUFFER_SIZE, "%s", buffer);
                                send(client_sockets[j], msg, strlen(msg), 0);
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}

