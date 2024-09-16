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

int ksh_launch(char **args, int background) { // добавляем параметр background
    pid_t pid, wpid; // переменные, необходимые для идентификации процессов
    int status;
    pid = fork(); // системный вызов, создание нового процесса
    if (pid == 0) { // дочерний процесс
        if (execvp(args[0], args) == -1) { // замена текущего процесса дочерним
            fprintf(stderr, "ksh: command not found: %s\n", args[0]);
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) { // ошибка в создании нового процесса
        perror("ksh");
    } else { // Выполнение родительского процесса
        // Если это не фоновый процесс, ожидаем завершения
        if (!background) {
            do { // родительский процесс ожидает завершения дочернего процесса
                wpid = waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }
    }
    return 1;
}


int ksh_execute(char **args) { // выполнение аргументов
    if (args[0] == NULL) { // пустая команда
        return 1;
    }
    // Проверка на наличие фонового выполнения
    int background = 0; // Переменная для хранения состояния фона
    int last_arg_index = -1;
    // Найдем индекс последнего аргумента
    for (int i = 0; args[i] != NULL; i++) {
        last_arg_index = i;
    }
    // Если последний аргумент - амперсанд, убираем его и устанавливаем флаг
    if (last_arg_index >= 0 && strcmp(args[last_arg_index], "&") == 0) {
        background = 1;
        args[last_arg_index] = NULL; // Убираем амперсанд из аргумент списка
    }
    for (int i = 0; i < ksh_num_builtins(); i++) { // перебор всех доступных команд
        if (strcmp(args[0], builtin_str[i]) == 0) { // сравнение аргумента с командой из списка
            return (*builtin_func[i])(args); // если аргумент и команда совпали, то вызывается определенная функция
        }
    }

    // Теперь вызываем ksh_launch с указанием, является ли процесс фоновым
    return ksh_launch(args, background);

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

char *ksh_read_line_from_file(FILE *file) {
    char *line = NULL;
    size_t bufsize = 0;
    if (getline(&line, &bufsize, file) == -1) {
        free(line); // Освободить память, если это последний вызов
        return NULL; // Возвращаем NULL, когда достигнут конец файла
    }
    return line; // Возвращаем считанную строку
}

void ksh_loop_from_file(FILE *file) {
    char *line; // строка, считанная из файла
    char **args; // аргументы строки, считанной из файла
    int status; // статус выполнения команды
    while ((line = ksh_read_line_from_file(file)) != NULL) {
        args = ksh_split_line(line); // разбить строку на аргументы
        status = ksh_execute(args); // выполнить аргументы и изменить статус
        free(line); // освободить память
        free(args);
        if (status == 0) break; // Если команда "quit", выйти
    }
}

// начало программы
int main(int argc, char **argv) {
    char buf[PATH_MAX]; // массив для пути к исполняемому файлу
    realpath(argv[0], buf); // сохранение полного пути к файлу в buf
    setenv("shell", buf, 1); // установка переменно окружения "shell" со значением buf

    if (argc > 1) { // Если аргумент передан
        FILE *file = fopen(argv[1], "r");
        if (!file) { // Проверка на ошибку открытия файла
            perror("Error opening file");
            return EXIT_FAILURE;
        }
        ksh_loop_from_file(file); // Запуск цикла для обработки команд из файла
        fclose(file); // Закрытие файла после завершения
    } else {
        ksh_loop(); // Запуск команд в интерактивном режиме
    }

    return EXIT_SUCCESS;
}