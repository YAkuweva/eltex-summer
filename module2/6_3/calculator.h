#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>

#define MAX_COMMANDS 20
#define LIB_DIR "./plugins/"

typedef struct {
    char name[20];
    char symbol;
    double (*func)(double, double);
    void* handle;
} Command;

double execute_command(Command cmd, double a, double b, int* error);
void print_result(double result, int error);
int load_commands(Command** commands);
void unload_commands(Command* commands, int count);
void show_menu(Command* commands, int count);
void read_numbers(double* a, double* b);

#endif