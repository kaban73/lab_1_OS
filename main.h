#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

#define LSH_TOK_BUFSIZE 64 // максимальный размер буфера
#define LSH_TOK_DELIM " \t\r\n\a" // разделители, по которым будет разбиваться строка
extern char **environ; // указатель на переменные окружения

// объявление функций команд
int ksh_cd(char **args);
int ksh_clr();
int ksh_dir(char **args);
int ksh_environ();
int ksh_echo(char **args);
int ksh_help(char **args);
int ksh_pause();
int ksh_quit();