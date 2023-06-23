#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>

#define MAX_MESSAGE_SIZE 1024

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main() {
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[MAX_MESSAGE_SIZE];

    // Eingabe der Übertragungsart, Servernamen und Dienstnamen
    char transport[10], server_name[16], service_name[16];
    int valid_input = 0;
    while (!valid_input) {
        printf("Welche Übertragungsart möchten Sie verwenden? (TCP/UDP): ");
        scanf("%s", transport);
        printf("Geben Sie den Servernamen ein ('srv' für 127.0.0.1): ");
        scanf("%s", server_name);
        printf("Geben Sie den gemeinsamen Dienstnamen ein: ");
        scanf("%s", service_name);

        // Serviceinformationen abrufen
        struct servent *service;
        if (strcmp(transport, "TCP") == 0) {
            service = getservbyname(service_name, "tcp");
        } else if (strcmp(transport, "UDP") == 0) {
            service = getservbyname(service_name, "udp");
        } else {
            fprintf(stderr, "Fehler: Ungültige Übertragungsart\n");
            continue;
        }

        if (service == NULL) {
            fprintf(stderr, "Fehler: Unbekannter Dienst\n");
            continue;
        }

        portno = ntohs(service->s_port);

        // Servername "srv" wurde eingegeben, verwenden Sie 127.0.0.1 als IP-Adresse
        if (strcmp(server_name, "srv") == 0) {
            server = gethostbyname("localhost");
        } else {
            server = gethostbyname(server_name);
        }

        if (server == NULL) {
            fprintf(stderr, "Fehler: Unbekannter Server\n");
            continue;
        }

        valid_input = 1;
    }

    // Socket erstellen
    if (strcmp(transport, "TCP") == 0) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
    } else if (strcmp(transport, "UDP") == 0) {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    }

    if (sockfd < 0)
        error("Fehler beim Öffnen des Sockets");

    // Serveradresse initialisieren
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    // Verbindung zum Server herstellen (TCP)
    if (strcmp(transport, "TCP") == 0) {
        if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
            error("Fehler beim Verbindungsaufbau");
    }

    // Zustandsvariable für die Client-Ausgabe
    int isFirstMessage = 1;

    // Kommunikationsschleife
    while (1) {
        // Nachricht vom Benutzer eingeben und an den Server senden
        memset(buffer, 0, MAX_MESSAGE_SIZE);
        fgets(buffer, MAX_MESSAGE_SIZE - 1, stdin);

        if (strcmp(transport, "TCP") == 0) {
            n = write(sockfd, buffer, strlen(buffer));
        } else if (strcmp(transport, "UDP") == 0) {
            n = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        }

        if (n < 0)
            error("Fehler beim Schreiben auf den Socket");

        // Prüfen, ob Benutzer die Verbindung beenden möchte
        if (strncmp(buffer, "bye", 3) == 0)
            break;

        // Antwort des Servers empfangen
        memset(buffer, 0, MAX_MESSAGE_SIZE);
        if (strcmp(transport, "TCP") == 0) {
            n = read(sockfd, buffer, MAX_MESSAGE_SIZE - 1);
        } else if (strcmp(transport, "UDP") == 0) {
            socklen_t serv_len = sizeof(serv_addr);
            n = recvfrom(sockfd, buffer, MAX_MESSAGE_SIZE - 1, 0, (struct sockaddr *)&serv_addr, &serv_len);
        }

        if (n < 0)
            error("Fehler beim Lesen vom Socket");

        // Empfangene Nachricht anzeigen
        if (!isFirstMessage) {
            printf("Server: %s\n", buffer);
        }

        // "Client:" nur einmal anzeigen, nachdem die Verbindung hergestellt wurde
        isFirstMessage = 0;

        // Nachricht vom Benutzer eingeben
        printf("Client: ");
        fflush(stdout);
    }

    // Verbindung schließen und Programm beenden
    close(sockfd);
    return 0;
}
