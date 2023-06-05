#include <stdio.h>

int main() {
    char eingabe [100]; // Array für die Eingabe

    printf("Bitte erstes Wort eingeben: ");
    scanf("%s", eingabe); 

    printf("Ausgabe: %s \n", eingabe);

    return 0;
}