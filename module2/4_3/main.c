#include <stdio.h>
#include "contacts.h"

int main(void) {
    int choice;
    char input[10];

    do {
        printf("\nPhone Book:\n");
        printf("1. Add contact\n");
        printf("2. Show contacts\n");
        printf("3. Edit contact\n");
        printf("4. Delete contact\n");
        printf("5. Show tree structure\n");
        printf("0. Exit\n");
        printf("Choice: ");

        fgets(input, sizeof(input), stdin);
        sscanf(input, "%d", &choice);

        switch (choice) {
        case 1:
            addContact();
            break;
        case 2:
            showContacts();
            break;
        case 3:
            editContact();
            break;
        case 4:
            deleteContact();
            break;
        case 5:
            printTreeStructure();
            break;
        case 0:
            printf("Goodbye!\n");
            break;
        default:
            printf("Invalid choice.\n");
        }
    } while (choice != 0);

    return 0;
}