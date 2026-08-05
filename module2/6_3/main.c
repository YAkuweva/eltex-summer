#include "calculator.h"

void show_menu(Command* commands, int count) {
    printf("\nCalculator (Dynamic): \n");

    for (int i = 0; i < count; i++) {
        printf("  %d. %s (%c)\n", i + 1, commands[i].name, commands[i].symbol);
    }

    printf("  0. Exit\n");
}

void read_numbers(double* a, double* b) {
    printf("Enter first number: ");
    scanf("%lf", a);

    printf("Enter second number: ");
    scanf("%lf", b);

    while (getchar() != '\n');
}

int main() {
    int choice;
    int count;
    double a, b;
    double result;
    int error;
    char input[10];
    Command* commands = NULL;

    count = load_commands(&commands);
    if (count == 0) {
        printf("No commands loaded. Exiting.\n");
        return 1;
    }

    printf("\nLoaded %d commands.\n", count);

    do {
        show_menu(commands, count);
        printf("Enter your choice: ");

        fgets(input, sizeof(input), stdin);
        sscanf(input, "%d", &choice);

        if (choice == 0) {
            printf("Goodbye!\n");
            break;
        }

        if (choice < 1 || choice > count) {
            printf("Invalid choice!\n\n");
            continue;
        }

        read_numbers(&a, &b);

        result = execute_command(commands[choice - 1], a, b, &error);

        printf("\n%.2f %c %.2f = ", a, commands[choice - 1].symbol, b);
        print_result(result, error);
        printf("\n");

    } while (choice != 0);

    unload_commands(commands, count);

    return 0;
}