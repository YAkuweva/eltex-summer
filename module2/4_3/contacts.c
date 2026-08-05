#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "contacts.h"

Contact* root = NULL;
static int operationCount = 0;

void removeN(char str[]) {
    int i = 0;
    while (str[i] != '\0') {
        if (str[i] == '\n') {
            str[i] = '\0';
            break;
        }
        i++;
    }
}

static int compare(Contact* a, Contact* b) {
    int cmp = strcmp(a->lastname, b->lastname);
    if (cmp == 0) cmp = strcmp(a->name, b->name);
    return cmp;
}

Contact* insert(Contact* tree, Contact* node) {
    if (tree == NULL) return node;
    if (compare(node, tree) < 0)
        tree->left = insert(tree->left, node);
    else
        tree->right = insert(tree->right, node);
    return tree;
}

int countNodes(Contact* tree) {
    if (tree == NULL) return 0;
    return 1 + countNodes(tree->left) + countNodes(tree->right);
}

void storeInOrder(Contact* tree, Contact** array, int* index) {
    if (tree == NULL) return;
    storeInOrder(tree->left, array, index);
    array[*index] = tree;
    (*index)++;
    storeInOrder(tree->right, array, index);
}

static Contact* buildBalanced(Contact** array, int left, int right) {
    if (left > right) return NULL;
    int mid = (left + right) / 2;
    Contact* node = array[mid];
    node->left = buildBalanced(array, left, mid - 1);
    node->right = buildBalanced(array, mid + 1, right);
    return node;
}

void balanceTree(void) {
    int count = countNodes(root);
    if (count < 2) return;

    Contact** array = malloc(count * sizeof(Contact*));
    if (array == NULL) return;

    int index = 0;
    storeInOrder(root, array, &index);
    root = buildBalanced(array, 0, count - 1);
    free(array);
}

static Contact* findMin(Contact* tree) {
    while (tree->left != NULL) tree = tree->left;
    return tree;
}

Contact* deleteNode(Contact* tree, Contact* node) {
    if (tree == NULL) return NULL;

    int cmp = compare(node, tree);

    if (cmp < 0)
        tree->left = deleteNode(tree->left, node);
    else if (cmp > 0)
        tree->right = deleteNode(tree->right, node);
    else {
        if (tree->left == NULL) {
            Contact* temp = tree->right;
            free(tree);
            return temp;
        }
        if (tree->right == NULL) {
            Contact* temp = tree->left;
            free(tree);
            return temp;
        }
        Contact* temp = findMin(tree->right);
        *tree = *temp;
        tree->right = deleteNode(tree->right, temp);
    }
    return tree;
}

static void printTree(Contact* tree, int* num) {
    if (tree == NULL) return;
    printTree(tree->left, num);

    printf("\n%d.\n", (*num)++);
    printf("Last name: %s\n", tree->lastname);
    printf("Name: %s\n", tree->name);
    printf("Patronymic: %s\n", tree->patronymic);
    printf("Workplace: %s\n", tree->workplace);
    printf("Position: %s\n", tree->position);
    printf("Phone: %s\n", tree->phone);
    printf("Email: %s\n", tree->email);
    printf("Social: %s\n", tree->social);
    printf("Messenger: %s\n", tree->messenger);

    printTree(tree->right, num);
}

void addContact(void) {
    Contact* newNode = malloc(sizeof(Contact));
    if (newNode == NULL) {
        printf("Memory error!\n");
        return;
    }

    newNode->left = NULL;
    newNode->right = NULL;

    printf("\nAdd new contact:\n");

    do {
        printf("Last name: ");
        fgets(newNode->lastname, MAX_NAME, stdin);
        removeN(newNode->lastname);
        if (strlen(newNode->lastname) == 0)
            printf("Last name is required.\n");
    } while (strlen(newNode->lastname) == 0);

    do {
        printf("Name: ");
        fgets(newNode->name, MAX_NAME, stdin);
        removeN(newNode->name);
        if (strlen(newNode->name) == 0)
            printf("Name is required.\n");
    } while (strlen(newNode->name) == 0);

    printf("Patronymic: ");
    fgets(newNode->patronymic, MAX_NAME, stdin);
    removeN(newNode->patronymic);

    printf("Workplace: ");
    fgets(newNode->workplace, MAX_WORK, stdin);
    removeN(newNode->workplace);

    printf("Position: ");
    fgets(newNode->position, MAX_WORK, stdin);
    removeN(newNode->position);

    printf("Phone: ");
    fgets(newNode->phone, MAX_PHONE, stdin);
    removeN(newNode->phone);

    printf("Email: ");
    fgets(newNode->email, MAX_EMAIL, stdin);
    removeN(newNode->email);

    printf("Social network: ");
    fgets(newNode->social, MAX_SOCIAL, stdin);
    removeN(newNode->social);

    printf("Messenger: ");
    fgets(newNode->messenger, MAX_SOCIAL, stdin);
    removeN(newNode->messenger);

    root = insert(root, newNode);
    operationCount++;

    if (operationCount >= 10) {
        balanceTree();
        operationCount = 0;
        printf("\nTree balanced.\n");
    }

    printf("\nContact added.\n");
}

void showContacts(void) {
    if (root == NULL) {
        printf("\nNo contacts found.\n");
        return;
    }
    printf("\nContacts:\n");
    int num = 1;
    printTree(root, &num);
}

void editContact(void) {
    if (root == NULL) {
        printf("No contacts found.\n");
        return;
    }

    int count = countNodes(root);
    Contact** array = malloc(count * sizeof(Contact*));
    if (array == NULL) return;

    int idx = 0;
    storeInOrder(root, array, &idx);

    printf("\nEdit Contact:\n");
    for (int i = 0; i < count; i++)
        printf("%d. %s %s\n", i + 1, array[i]->lastname, array[i]->name);

    int choice;
    char input[10];
    printf("Select contact: ");
    fgets(input, sizeof(input), stdin);
    sscanf(input, "%d", &choice);

    if (choice < 1 || choice > count) {
        printf("Invalid choice.\n");
        free(array);
        return;
    }

    Contact* c = array[choice - 1];
    free(array);

    do {
        printf("\nEdit Menu:\n");
        printf("1. Last name\n2. Name\n3. Patronymic\n4. Workplace\n");
        printf("5. Position\n6. Phone\n7. Email\n8. Social\n9. Messenger\n0. Done\n");
        printf("Choice: ");

        fgets(input, sizeof(input), stdin);
        sscanf(input, "%d", &choice);

        switch (choice) {
        case 1:
            do {
                printf("New last name: ");
                fgets(c->lastname, MAX_NAME, stdin);
                removeN(c->lastname);
                if (strlen(c->lastname) == 0)
                    printf("Last name is required.\n");
            } while (strlen(c->lastname) == 0);
            break;
        case 2:
            do {
                printf("New name: ");
                fgets(c->name, MAX_NAME, stdin);
                removeN(c->name);
                if (strlen(c->name) == 0)
                    printf("Name is required.\n");
            } while (strlen(c->name) == 0);
            break;
        case 3:
            printf("New patronymic: ");
            fgets(c->patronymic, MAX_NAME, stdin);
            removeN(c->patronymic);
            break;
        case 4:
            printf("New workplace: ");
            fgets(c->workplace, MAX_WORK, stdin);
            removeN(c->workplace);
            break;
        case 5:
            printf("New position: ");
            fgets(c->position, MAX_WORK, stdin);
            removeN(c->position);
            break;
        case 6:
            printf("New phone: ");
            fgets(c->phone, MAX_PHONE, stdin);
            removeN(c->phone);
            break;
        case 7:
            printf("New email: ");
            fgets(c->email, MAX_EMAIL, stdin);
            removeN(c->email);
            break;
        case 8:
            printf("New social: ");
            fgets(c->social, MAX_SOCIAL, stdin);
            removeN(c->social);
            break;
        case 9:
            printf("New messenger: ");
            fgets(c->messenger, MAX_SOCIAL, stdin);
            removeN(c->messenger);
            break;
        case 0:
            printf("Contact updated.\n");
            break;
        default:
            printf("Invalid choice.\n");
        }
    } while (choice != 0);
}

void deleteContact(void) {
    if (root == NULL) {
        printf("No contacts found.\n");
        return;
    }

    int count = countNodes(root);
    Contact** array = malloc(count * sizeof(Contact*));
    if (array == NULL) return;

    int idx = 0;
    storeInOrder(root, array, &idx);

    printf("\nDelete Contact:\n");
    for (int i = 0; i < count; i++)
        printf("%d. %s %s\n", i + 1, array[i]->lastname, array[i]->name);

    int choice;
    char input[10];
    printf("Select contact: ");
    fgets(input, sizeof(input), stdin);
    sscanf(input, "%d", &choice);

    if (choice < 1 || choice > count) {
        printf("Invalid choice.\n");
        free(array);
        return;
    }

    Contact* c = array[choice - 1];
    free(array);

    printf("Delete this contact? (y/n): ");
    fgets(input, sizeof(input), stdin);

    if (input[0] == 'y' || input[0] == 'Y') {
        root = deleteNode(root, c);
        operationCount++;

        if (operationCount >= 10) {
            balanceTree();
            operationCount = 0;
            printf("\nTree balanced.\n");
        }

        printf("Contact deleted.\n");
    }
    else {
        printf("Deletion canceled.\n");
    }
}


static void printTreeWithBranches(Contact* tree, int level, int isLeft) {
    if (tree == NULL) return;

    printTreeWithBranches(tree->right, level + 1, 0);

    for (int i = 0; i < level; i++) {
        printf("    ");
    }

    if (level > 0) {
        printf(isLeft ? "+-- " : "+-- ");
    }
    printf("%s %s\n", tree->lastname, tree->name);
    printTreeWithBranches(tree->left, level + 1, 1);
}

void printTreeStructure(void) {
    if (root == NULL) {
        printf("Tree is empty.\n");
        return;
    }
    printf("\nTree: \n");
    printTreeWithBranches(root, 0, 0);
    printf("\n\n");
}