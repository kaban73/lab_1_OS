#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

// КОМАНДЫ
int ksh_cd(char **args);
int ksh_help(char **args);
int ksh_exit(char **args);

char *builtin_str[] = {
    "cd",
    "help",
    "exit"
};

int (*builtin_func[]) (char**) = {
        &ksh_cd,
        &ksh_help,
        &ksh_exit,
};

int ksh_num_builtins() {
    return sizeof(builtin_str) / sizeof (char*);
}
// КОМАНДЫ

int ksh_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "ksh: an argument is expected for \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("ksh");
        }
    }
    return 1;
}

int ksh_help(char **args) {
    printf("list of available commands:\n");
    for (int i = 0; i < ksh_num_builtins();i++) {
        printf(" %s\n", builtin_str[i]);
    }
    return 1;
}

int ksh_exit(char **args) {
    return 0;
}


// запуск процесса
int ksh_launch(char **args) {
    pid_t pid, wpid;
    int status;

    pid = fork(); // системный вызов
    if (pid == 0) { // child
        if (execvp(args[0], args) == -1) {
            perror("ksh");
        }
        exit(EXIT_FAILURE);
    } else if(pid < 0) { // error
        perror("ksh");
    } else { // parent
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}

int ksh_execute(char **args) {
    if (args[0] == NULL) { // пустая команда
        return 1;
    }

    for (int i = 0; i < ksh_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return ksh_launch(args);
}

char **ksh_split_line(char *line) {
    int bufsize = LSH_TOK_BUFSIZE;
    int position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

    if (!tokens) {
        fprintf(stderr, "ksh: memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, LSH_TOK_DELIM);
    while(token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += LSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "ksh: memory allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, LSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

char *ksh_read_line() {
    char *line = NULL;
    ssize_t bufsize = 0;
    getline(&line, &bufsize, stdin);
    return line;
}

// цикл командной оболочки
void ksh_loop() {
    char *line;
    char **args;
    int status;

    do {
        printf("> ");
        line = ksh_read_line();
        args = ksh_split_line(line);
        status = ksh_execute(args);

        free(line);
        free(args);
    } while (status);
}

int main(int argc, char **argv) {

    ksh_loop(); // запуск команд

    return EXIT_SUCCESS;
}
