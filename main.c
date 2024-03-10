#include "main.h"

char *builtin_str[] = { // массив команд оболочки
        "cd",
        "clr",
        "dir",
        "environ",
        "echo",
        "help",
        "pause",
        "quit"
};

char *description_comm[] = { // массив с командами оболочки и их описанием
        "cd <directory> - change the current default directory to <directory>. If the <directory> argument is missing, print the current directory.",
        "clr - clearing the screen.",
        "dir <directory> - displays the contents of the <directory> directory.",
        "environ - output of all environment variables.",
        "echo <comment> - output to the <comment> screen, after which the transition to a new line is performed.",
        "help - output of the user's guide.",
        "pause - suspends shell operations until the <Enter> key is pressed.",
        "quit - exit the shell."
};

int (*builtin_func[]) (char**) = { // массив указателей на функции команд
        &ksh_cd,
        &ksh_clr,
        &ksh_dir,
        &ksh_environ,
        &ksh_echo,
        &ksh_help,
        &ksh_pause,
        &ksh_quit
};

int ksh_num_builtins() { // функция выводящая количество команд оболочки
    return sizeof(builtin_str) / sizeof (char*);
}

int ksh_cd(char **args) { // функция для перемещения в другую директорию
    if (args[1] == NULL) { // если директория не указана, то выводится путь текущей директории
        char *current_dir = getcwd(NULL, 0); // получение пути к текущему рабочему каталогу
        if (current_dir) { // если все удачно - выводим путь, иначе ошибку
            printf("Current directory: %s\n", current_dir);
            free(current_dir);
        } else {
            perror("ksh");
        }
    } else { // если директория указана
        if (chdir(args[1]) != 0) { // изменяем текущий каталог на новый
            perror("ksh"); // если возникла ошибка при смене директории - выводим оишбку
        } else {
            setenv("PWD", args[1], 1); // устанавливаем переменную окружения PWD в новый рабочий каталог
        }
    }
    return 1;
}

int ksh_clr() { // функция очистки содержимого консоли
    printf("\033c"); // команда для очистки экрана терминала
    return 1;
}

int ksh_dir(char **args) { // функция вывода содержимого указанной директории
    if (args[1] == NULL) { // если после dir отсутствует путь к директории
        fprintf(stderr, "Usage: dir <directory>\n");
    } else {
        DIR *dir;
        struct dirent *entry; // указатели необходимые для работы с директориями

        dir = opendir(args[1]); // попытка открыть директорию
        if (dir == NULL) { // неудачная попытка открыть директорию
            perror("opendir");
            fprintf(stderr, "Error: unable to open directory %s\n", args[1]);
        } else { // удачная
            while ((entry = readdir(dir)) != NULL) { // проход по каждому элементу директории и его вывод в консоль
                printf("%s\n", entry->d_name);
            }
            closedir(dir); // закрытие директории
        }
    }
    return 1;
}

int ksh_environ() { // эта функция выводит в коносль все переменные окружения в оболочке командной строки
    char **env = environ;
    while (*env != NULL) {
        printf("%s\n", *env);
        env++;
    }
    return 1;
}

int ksh_echo(char **args) { // функция предназначена для вывода комментария в консоль
    if (args[1] == NULL) {  // если комментарий был пустым, то просто выводит пустую строку
        printf("\n");
    } else { // иначе выводит каждый аргумент комментария через пробел
        int i;
        for (i = 1; args[i] != NULL; i++) {
            printf("%s", args[i]);
            if (args[i + 1] != NULL) {
                printf(" ");
            }
        }
        printf("\n");
    }
    return 1;
}

int ksh_help(char **args) { // функция выводящая массив со списком команд и их описания
    printf("List of available commands:\n");
    for (int i = 0; i < ksh_num_builtins();i++) {
        printf(" %s\n", description_comm[i]);
    }
    return 1;
}

int ksh_pause() { // функция паузы которая входит в бесконечный цикл, пока не будет нажат <Enter>
    printf("Press <Enter> to continue...");
    while (getchar() != '\n');
    return 1;
}

int ksh_quit() { // выход из оболочки, завершение процесса
    return 0;
}

// запуск процесса
int ksh_launch(char **args) { // в этой функции запускается новый процесс , который обрабатывает ошибки и дожидается завершения дочерних процессов
    pid_t pid, wpid; // переменные, необходимые для идентификации процессов
    int status;

    pid = fork(); // системный вызов, создание нового процесса
    if (pid == 0) { // дочерний процесс
        if (execvp(args[0], args) == -1) { // замена текущего процесса дочерним
            fprintf(stderr, "ksh: command not found: %s\n", args[0]);
        }
        exit(EXIT_FAILURE);
    } else if(pid < 0) { // ошибка в создании нового процесса
        perror("ksh");
    } else { // Выполнение родительского процесса
        do { // родительский процесс ожидает завершения дочернего процесса
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}

int ksh_execute(char **args) { // выполнение аргументов
    if (args[0] == NULL) { // пустая команда
        return 1;
    }

    for (int i = 0; i < ksh_num_builtins(); i++) { // перебор всех доступных команд
        if (strcmp(args[0], builtin_str[i]) == 0) { // сравнение аргумента с командой из списка
            return (*builtin_func[i])(args); // если аргумент и команда совпали то вызывается определенная функция
        }
    }

    return ksh_launch(args); // если команда не была найдена вызывается внешний процесс
}

char **ksh_split_line(char *line) { // разбиение строки на аргументы
    int bufsize = LSH_TOK_BUFSIZE; // задание начального размера буфера для хранения токенов
    int position = 0;
    char **tokens = malloc(bufsize * sizeof(char*)); // массив для хранения токенов-аргументов
    char *token; // временное хранение токенов входной строки

    if (!tokens) { // проверка на выделение памяти для токенов
        fprintf(stderr, "ksh: memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, LSH_TOK_DELIM); // разделение строки на токены по делителям
    while(token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) { // проверка достиг ли массив токенов своего макс размера
            bufsize += LSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*)); // изменение размера выделенной памяти для токенов
            if (!tokens) {
                fprintf(stderr, "ksh: memory allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, LSH_TOK_DELIM); // получение следующего токена
    }
    tokens[position] = NULL; // обозначение конца массива
    return tokens;
}

char *ksh_read_line() { // считывание строки
    char *line = NULL;
    ssize_t bufsize = 0;
    getline(&line, &bufsize, stdin); // считывание строки
    return line;
}

// цикл командной оболочки
void ksh_loop() {
    char *line; // строка введенная пользователем
    char **args; // аргументы строки, введенной пользователем
    int status; // статус выполнения команды

    do {
        printf("> ");
        line = ksh_read_line(); // считывание строки
        args = ksh_split_line(line); // разбиение ее по аргументам
        status = ksh_execute(args); // выполнение аргументов и изменение статуса

        free(line); // освобождение памяти
        free(args);
    } while (status);
}

// начало программы
int main(int argc, char **argv) {
    char buf[PATH_MAX]; // массив для пути к исполняемому файлу
    realpath(argv[0], buf); // сохранение полного пути к файлу в buf
    setenv("shell", buf, 1); // установка переменно окружения "shell" со значением buf

    ksh_loop(); // запуск команд

    return EXIT_SUCCESS;
}
