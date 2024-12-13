#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in server_address;
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE];

    // Soket oluþturma
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Connection failed");
        return -1;
    }

    printf("Connected to the server. Enter words to play:\n");

    fd_set readfds;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sock, &readfds);
        int max_sd = sock;

        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("Select error");
            exit(EXIT_FAILURE);
        }

        // Kullanýcýdan giriþ
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            fgets(message, BUFFER_SIZE, stdin);
            message[strcspn(message, "\n")] = '\0';
            send(sock, message, strlen(message), 0);
        }

        // Server'dan mesaj alýmý
        if (FD_ISSET(sock, &readfds)) {
            int valread = read(sock, buffer, BUFFER_SIZE);
            if (valread > 0) {
                buffer[valread] = '\0';  // Null terminator ekle

                // Gelen mesajýn kontrolü
                if (strcmp(buffer, "You are disqualified!\n") == 0 || strcmp(buffer, "You win!\n") == 0) {
                    printf("%s", buffer);  // Sadece diskalifiye veya kazandý mesajý
                } else {
                    printf("Message from another client: %s\n", buffer);  // Diðer mesajlar
                }
            } else if (valread == 0) {
                // Sunucu baðlantýsý kesildi
                printf("Server disconnected.\n");
                close(sock);
                break;
            }
        }
    }

    return 0;
}

