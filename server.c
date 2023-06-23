#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pcap.h>
#include <sys/socket.h>

#define MAX_MESSAGE_SIZE 1024

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void packetHandler(u_char *userData, const struct pcap_pkthdr *pkthdr, const u_char *packetData) {
    printf("Packet captured\n");
}

int main() {
    int sockfd, newsockfd, n;
    socklen_t clilen, addrlen;
    char buffer[MAX_MESSAGE_SIZE];
    struct sockaddr_in serv_addr, cli_addr;

    // Eingabe der Übertragungsart, des Dienstnamens und der Datei-Pfad
    char transport[10];
    char service_name[100];
    char filepath[100];

    // Serviceinformationen abrufen
    struct servent *service = NULL;
    while (1) {
        printf("Welche Übertragungsart möchten Sie verwenden? (TCP/UDP): ");
        scanf("%s", transport);

        printf("Geben Sie den gemeinsamen Dienstnamen ein: ");
        scanf("%s", service_name);

        service = getservbyname(service_name, strcmp(transport, "TCP") == 0 ? "tcp" : "udp");
        if (service != NULL)
            break;

        fprintf(stderr, "Fehler: Ungültige Übertragungsart oder Dienstname\n");
    }

    // Socket erstellen
    sockfd = socket(AF_INET, strcmp(transport, "TCP") == 0 ? SOCK_STREAM : SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("Fehler beim Öffnen des Sockets");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = service->s_port;

    // Binden des Sockets an die Serveradresse
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("Fehler beim Binden");

    // Das "julian" muss durch den eigenen Benutzernamen ausgetauscht werden
    snprintf(filepath, sizeof(filepath), "/home/julian/Desktop/mitschnitt.pcap");

    char command[256];
    snprintf(command, sizeof(command), "%s -i lo %s port %d -w %s &", 
        "tcpdump",
        strcmp(transport, "TCP") == 0 ? "" : "udp",
        ntohs(serv_addr.sin_port),
        filepath
    );
    system(command);

    int isConnectionClosed = 0;
    if (strcmp(transport, "TCP") == 0) {
        listen(sockfd, 5);
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
            error("Fehler beim Akzeptieren der Verbindung");

        while (!isConnectionClosed) {
            memset(buffer, 0, MAX_MESSAGE_SIZE);
            n = read(newsockfd, buffer, MAX_MESSAGE_SIZE - 1);
            if (n < 0)
                error("Fehler beim Lesen vom Socket");

            printf("Client: %s\n", buffer);

            if (strncmp(buffer, "bye", 3) == 0)
                isConnectionClosed = 1;
            else {
                printf("Server: ");
                memset(buffer, 0, MAX_MESSAGE_SIZE);
                fgets(buffer, MAX_MESSAGE_SIZE - 1, stdin);
                n = write(newsockfd, buffer, strlen(buffer));
                if (n < 0)
                    error("Fehler beim Schreiben auf den Socket");
            }
        }

        close(newsockfd);
    } else if (strcmp(transport, "UDP") == 0) {
        while (!isConnectionClosed) {
            memset(buffer, 0, MAX_MESSAGE_SIZE);
            addrlen = sizeof(cli_addr);
            n = recvfrom(sockfd, buffer, MAX_MESSAGE_SIZE - 1, 0, (struct sockaddr *) &cli_addr, &addrlen);
            if (n < 0)
                error("Fehler beim Lesen vom Socket");

            printf("Client: %s\n", buffer);

            if (strncmp(buffer, "bye", 3) == 0)
                isConnectionClosed = 1;
            else {
                printf("Server: ");
                memset(buffer, 0, MAX_MESSAGE_SIZE);
                fgets(buffer, MAX_MESSAGE_SIZE - 1, stdin);
                n = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *) &cli_addr, addrlen);
                if (n < 0)
                    error("Fehler beim Schreiben auf den Socket");
            }
        }
    }

    snprintf(command, sizeof(command), "pkill tcpdump");
    system(command);

    close(sockfd);
    return 0;
}
