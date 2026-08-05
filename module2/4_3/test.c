#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "contacts.h"

void clearTree(Contact* tree) {
    if (tree == NULL) return;
    clearTree(tree->left);
    clearTree(tree->right);
    free(tree);
}

void test_add_contact(void) {
    printf("Test 1: add contact    ");

    if (root != NULL) {
        clearTree(root);
        root = NULL;
    }

    Contact* c = malloc(sizeof(Contact));
    strcpy(c->lastname, "Ivanov");
    strcpy(c->name, "Ivan");
    strcpy(c->patronymic, "Ivanovich");
    strcpy(c->workplace, "Company");
    strcpy(c->position, "Manager");
    strcpy(c->phone, "123456789");
    strcpy(c->email, "ivan@mail.ru");
    strcpy(c->social, "vk.com/ivan");
    strcpy(c->messenger, "@ivan");
    c->left = NULL;
    c->right = NULL;

    root = insert(root, c);

    assert(root != NULL);
    assert(strcmp(root->lastname, "Ivanov") == 0);
    assert(strcmp(root->name, "Ivan") == 0);

    printf("Done!\n");
}

void test_multiple_contacts(void) {
    printf("Test 2: add 15 contacts with balancing    ");

    if (root != NULL) {
        clearTree(root);
        root = NULL;
    }

    for (int i = 0; i < 15; i++) {
        Contact* c = malloc(sizeof(Contact));
        sprintf(c->lastname, "LastName%d", i);
        sprintf(c->name, "Name%d", i);
        strcpy(c->patronymic, "Patr");
        strcpy(c->workplace, "Work");
        strcpy(c->position, "Pos");
        strcpy(c->phone, "123");
        strcpy(c->email, "a@a.ru");
        strcpy(c->social, "soc");
        strcpy(c->messenger, "mess");
        c->left = NULL;
        c->right = NULL;

        root = insert(root, c);
    }

    assert(countNodes(root) == 15);

    printf("Done!\n");
}


void test_delete_contact(void) {
    printf("Test 3: delete contact    ");

    if (root != NULL) {
        clearTree(root);
        root = NULL;
    }

    Contact* c1 = malloc(sizeof(Contact));
    strcpy(c1->lastname, "AAA");
    strcpy(c1->name, "A");
    c1->left = NULL;
    c1->right = NULL;
    root = insert(root, c1);

    Contact* c2 = malloc(sizeof(Contact));
    strcpy(c2->lastname, "BBB");
    strcpy(c2->name, "B");
    c2->left = NULL;
    c2->right = NULL;
    root = insert(root, c2);

    Contact* c3 = malloc(sizeof(Contact));
    strcpy(c3->lastname, "CCC");
    strcpy(c3->name, "C");
    c3->left = NULL;
    c3->right = NULL;
    root = insert(root, c3);

    printf("\nBefore deletion (3):");
    printTreeStructure();

    assert(countNodes(root) == 3);

    root = deleteNode(root, c2);
    assert(countNodes(root) == 2);

    printf("\nAfter deletion (2):");
    printTreeStructure();

    printf("Done!\n");
}

void test_balance(void) {
    printf("Test 4: balance tree    ");

    if (root != NULL) {
        clearTree(root);
        root = NULL;
    }

    Contact* c1 = malloc(sizeof(Contact));
    strcpy(c1->lastname, "Z");
    strcpy(c1->name, "Z");
    c1->left = NULL;
    c1->right = NULL;
    root = insert(root, c1);

    Contact* c2 = malloc(sizeof(Contact));
    strcpy(c2->lastname, "Y");
    strcpy(c2->name, "Y");
    c2->left = NULL;
    c2->right = NULL;
    root = insert(root, c2);

    Contact* c3 = malloc(sizeof(Contact));
    strcpy(c3->lastname, "X");
    strcpy(c3->name, "X");
    c3->left = NULL;
    c3->right = NULL;
    root = insert(root, c3);

    printf("\nBefore balancing:");
    printTreeStructure();

    balanceTree();

    printf("\nAfter balancing:");
    printTreeStructure();

    assert(countNodes(root) == 3);

    printf("Done!\n");
}

void test_periodic_balancing(void) {
    printf("Test 5: periodic balancing     ");

    if (root != NULL) {
        clearTree(root);
        root = NULL;
    }

    for (int i = 0; i < 15; i++) {
        Contact* c = malloc(sizeof(Contact));
        sprintf(c->lastname, "Z%d", i);
        sprintf(c->name, "N%d", i);
        strcpy(c->patronymic, "P");
        strcpy(c->workplace, "W");
        strcpy(c->position, "Pos");
        strcpy(c->phone, "123");
        strcpy(c->email, "a@a.ru");
        strcpy(c->social, "soc");
        strcpy(c->messenger, "mess");
        c->left = NULL;
        c->right = NULL;

        root = insert(root, c);

        if (i == 9) {
            printf("\nAfter 10 operations (before balancing):");
            printTreeStructure();
        }
        if (i == 10) {
            printf("\nAfter 11 operations (after balancing):");
            printTreeStructure();
        }
    }

    assert(countNodes(root) == 15);
    printf("\nFinal tree (15):");
    printTreeStructure();

    printf("Done!\n");
}

int main(void) {
    printf("\n    Tests!!!    \n\n");

    test_add_contact();
    test_multiple_contacts();
    test_delete_contact();
    test_balance();
    test_periodic_balancing();

    if (root != NULL) {
        clearTree(root);
        root = NULL;
    }

    printf("\n  All tests DONE!!!  \n");

    return 0;
}