#include "calculator.h"

double execute_command(Command cmd, double a, double b, int* error) {
    double result = cmd.func(a, b);

    if (cmd.symbol == '/' && b == 0) {
        *error = 1;
        return 0.0;
    }

    *error = 0;
    return result;
}

void print_result(double result, int error) {
    if (error) {
        printf("Error!!! Division by zero!\n");
    }
    else {
        printf(" %.2f\n", result);
    }
}

static int load_library(const char* lib_path, Command* cmd) {
    void* handle;
    double (*func)(double, double);
    char func_name[50];
    char name[50];
    char symbol_char;

    handle = dlopen(lib_path, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Error loading %s: %s\n", lib_path, dlerror());
        return 0;
    }

    char* start = strrchr(lib_path, '/');
    if (start == NULL) {
        start = (char*)lib_path;
    }
    else {
        start++;
    }

    if (strncmp(start, "lib", 3) == 0) {
        start += 3;
    }

    char* dot = strchr(start, '.');
    if (dot != NULL) {
        int len = dot - start;
        strncpy(func_name, start, len);
        func_name[len] = '\0';
    }
    else {
        strcpy(func_name, start);
    }

    func = (double (*)(double, double)) dlsym(handle, func_name);
    if (!func) {
        fprintf(stderr, "Error getting symbol %s: %s\n", func_name, dlerror());
        dlclose(handle);
        return 0;
    }

    if (strcmp(func_name, "add") == 0) {
        symbol_char = '+';
        strcpy(name, "Add");
    }
    else if (strcmp(func_name, "subtract") == 0) {
        symbol_char = '-';
        strcpy(name, "Subtract");
    }
    else if (strcmp(func_name, "multiply") == 0) {
        symbol_char = '*';
        strcpy(name, "Multiply");
    }
    else if (strcmp(func_name, "divide") == 0) {
        symbol_char = '/';
        strcpy(name, "Divide");
    }
    else if (strcmp(func_name, "power") == 0) {
        symbol_char = '^';
        strcpy(name, "Power");
    }
    else {
        symbol_char = func_name[0];
        strcpy(name, func_name);
        if (name[0] >= 'a' && name[0] <= 'z') {
            name[0] = name[0] - 'a' + 'A';
        }
    }

    strcpy(cmd->name, name);
    cmd->symbol = symbol_char;
    cmd->func = func;
    cmd->handle = handle;

    return 1;
}

int load_commands(Command** commands) {
    DIR* dir;
    struct dirent* entry;
    int count = 0;

    *commands = (Command*)malloc(MAX_COMMANDS * sizeof(Command));
    if (*commands == NULL) {
        fprintf(stderr, "Memory allocation failed!\n");
        return 0;
    }

    dir = opendir(LIB_DIR);
    if (dir == NULL) {
        fprintf(stderr, "Cannot open directory: %s\n", LIB_DIR);
        free(*commands);
        return 0;
    }

    while ((entry = readdir(dir)) != NULL) {
        char* name = entry->d_name;
        char lib_path[256];

        if (strncmp(name, "lib", 3) != 0) {
            continue;
        }

        char* ext = strrchr(name, '.');
        if (ext == NULL) {
            continue;
        }

        if (strcmp(ext, ".so") != 0) {
            continue;
        }

        sprintf(lib_path, "%s%s", LIB_DIR, name);

        printf("Loading: %s", name);

        if (load_library(lib_path, &(*commands)[count])) {
            count++;
            printf("  : success!\n");
        }
        else {
            printf("  : error!!\n");
        }
    }

    closedir(dir);

    if (count == 0) {
        printf("No libraries found in %s\n", LIB_DIR);
        free(*commands);
        return 0;
    }

    return count;
}

void unload_commands(Command* commands, int count) {
    for (int i = 0; i < count; i++) {
        if (commands[i].handle != NULL) {
            dlclose(commands[i].handle);
        }
    }
    free(commands);
}