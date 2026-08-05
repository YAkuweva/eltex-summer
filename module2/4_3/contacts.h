#ifndef CONTACTS_H
#define CONTACTS_H

#define MAX_NAME 100
#define MAX_PHONE 20
#define MAX_EMAIL 100
#define MAX_WORK 100
#define MAX_SOCIAL 100

typedef struct Contact {
    char lastname[MAX_NAME];
    char name[MAX_NAME];
    char patronymic[MAX_NAME];
    char workplace[MAX_WORK];
    char position[MAX_WORK];
    char phone[MAX_PHONE];
    char email[MAX_EMAIL];
    char social[MAX_SOCIAL];
    char messenger[MAX_SOCIAL];
    struct Contact* left;
    struct Contact* right;
} Contact;

extern Contact* root;

void addContact(void);
void showContacts(void);
void editContact(void);
void deleteContact(void);
void removeN(char str[]);
void printTreeStructure(void);   

Contact* insert(Contact* tree, Contact* node);
Contact* deleteNode(Contact* tree, Contact* node);
void balanceTree(void);
int countNodes(Contact* tree);
void storeInOrder(Contact* tree, Contact** array, int* index);

#endif