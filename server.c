#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pcap.h>
#include <sys/socket.h>

// Maximale Message größe
#define MAX_MESSAGE_SIZE 1024

// Ruft Fehlermeldung auf und beendet Programm
void error(const char *msg) {
    perror(msg);
    exit(1);
}


int main() {
    //Deklaration von Variablen
    /*
    sockfd = Socket für Server-Socket
    newsockfd = Socket für angenommenen Client-Socket
    */
    int sockfd, newsockfd, n;
    socklen_t clilen, addrlen;
    char buffer[MAX_MESSAGE_SIZE];
    struct sockaddr_in serv_addr, cli_addr;

    // Eingabe der Übertragungsart, des Dienstnamens und des Dateipfads
    char transport[10];
    char service_name[100];
    char filepath[100];

    // Serviceinformationen abrufen
    struct servent *service = NULL;
    while (1) {
        // Eingabe Übertragungsart
        printf("Welche Übertragungsart möchten Sie verwenden? (TCP/UDP): ");
        scanf("%s", transport);

        //Einge Dienstnamen (Über welchen Port wird kommuniziert)
        printf("Geben Sie den gemeinsamen Dienstnamen ein: ");
        scanf("%s", service_name);

        // Überprüfen und Abrufen des Dienstes anhand des Dienstnamens und der Übertragungsart
        // Durchsucht /etc/services nach Dienstnamen
        service = getservbyname(service_name, strcmp(transport, "TCP") == 0 ? "tcp" : "udp");
        if (service != NULL)
            break;

        fprintf(stderr, "Fehler: Ungültige Übertragungsart oder Dienstname\n");
    }

    // Socket erstellen
    // Funktion "socket" erstellt neuen Socket abhängig von gewählter Übertragungsart
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

    // Setzen des Dateipfads für den Mitschnitt
    // Hier musst du deinen eigenen Benutzernamen anstelle von "julian" verwenden
    snprintf(filepath, sizeof(filepath), "/home/julian/Desktop/mitschnitt.pcap");

    // Erstellen des Befehls für das Mitschnitt-Tool "tcpdump" und Ausführen des Befehls
    char command[256];
    snprintf(command, sizeof(command), "%s -i lo %s port %d -w %s &", 
        "tcpdump",
        strcmp(transport, "TCP") == 0 ? "" : "udp",
        ntohs(serv_addr.sin_port),
        filepath
    );
    // Ausführen des Mitschnitts im Hintergrund
    system(command);

    int isConnectionClosed = 0;
    if (strcmp(transport, "TCP") == 0) {
        // Wenn die Übertragungsart TCP ist

        // Auf eingehende Verbindungen warten
        listen(sockfd, 5);
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
            error("Fehler beim Akzeptieren der Verbindung");

        while (!isConnectionClosed) {
            // Nachricht vom Client lesen
            memset(buffer, 0, MAX_MESSAGE_SIZE);
            n = read(newsockfd, buffer, MAX_MESSAGE_SIZE - 1);
            if (n < 0)
                error("Fehler beim Lesen vom Socket");

            printf("Client: %s\n", buffer);

            if (strncmp(buffer, "bye", 3) == 0)
                isConnectionClosed = 1;
            else {
                // Antwort an den Client senden
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
        // Wenn die Übertragungsart UDP ist

        while (!isConnectionClosed) {
            // Nachricht vom Client empfangen
            memset(buffer, 0, MAX_MESSAGE_SIZE);
            addrlen = sizeof(cli_addr);
            n = recvfrom(sockfd, buffer, MAX_MESSAGE_SIZE - 1, 0, (struct sockaddr *) &cli_addr, &addrlen);
            if (n < 0)
                error("Fehler beim Lesen vom Socket");

            printf("Client: %s\n", buffer);

            if (strncmp(buffer, "bye", 3) == 0)
                isConnectionClosed = 1;
            else {
                // Antwort an den Client senden
                printf("Server: ");
                memset(buffer, 0, MAX_MESSAGE_SIZE);
                fgets(buffer, MAX_MESSAGE_SIZE - 1, stdin);
                n = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *) &cli_addr, addrlen);
                if (n < 0)
                    error("Fehler beim Schreiben auf den Socket");
            }
        }
    }

    // Prozess des Mitschnitt-Tools beenden
    snprintf(command, sizeof(command), "pkill tcpdump");
    system(command);

    close(sockfd);
    return 0;
}
