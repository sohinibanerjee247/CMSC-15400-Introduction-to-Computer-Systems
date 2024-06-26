#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

int redirect;

// done
void myPrint(char *msg) {
    write(STDOUT_FILENO, msg, strlen(msg));
}

// done
void error() {
    char error_message[30] = "An error has occurred\n";
    write(STDOUT_FILENO, error_message, strlen(error_message));
}

// done
int non_empty_line(char* line) {
    int non_space = 0;
    int index = 0;
    while (line[index]) {
        non_space += (line[index] != ' ' && line[index] != '\t');
        index++;
    }
    return non_space;
}

// done
int is_redirect(char* line) {
    int index = 0;
    int count = 0;
    while (line[index]) {
        if (line[index++] == '>') {
            count++;
        }
    }
    return count;
}

char* trim(char* s) {
    while (s && (*s == ' ' || *s == '\t')) {
        s++;
    }

    while (s && (s[strlen(s) - 1] == ' ' || s[strlen(s) - 1] == '\t')) {
        s[strlen(s) - 1] = '\0';
    }
    return s;
}

void execute_exit(char** args, int arg_count) {
    if (arg_count > 1) {
        error();
    } else {
        exit(0);
    }
}

void execute_pwd(char** args, int arg_count) {
    if (arg_count > 1) {
        error();
        return;
    }
    
    char directory[1000];
    getcwd(directory, sizeof(directory));
    myPrint(directory);
    myPrint("\n");
}

void execute_cd(char** args, int arg_count) {
    if (arg_count > 2) {
        error();
        return;
    }
    
    int works = 1;
    if (arg_count > 1) {
        works = chdir(args[1]);
    } else {
        works = chdir(getenv("HOME"));
    }

    if (works == -1) {
        error();
    }
}

void do_command(char** args, int arg_count, char* red_command) {
    int copy = dup(1);
    if (redirect) {
        int temp_file = open(red_command, O_RDWR | S_IRUSR | S_IWUSR | O_CREAT | O_EXCL); 
        if (temp_file == -1) {
            error();
            return;
        }
        dup2(temp_file, 1);
    }

    if (!strcmp("exit", args[0])) {
        execute_exit(args, arg_count);
    } else if(!strcmp("pwd", args[0])) {
        execute_pwd(args, arg_count);
    } else if(!strcmp("cd", args[0])) {
        execute_cd(args, arg_count);
    } else {
        int pid = fork();
        if (pid == 0) {
            if (execvp(args[0], args) == -1) {
                error();
            }
            exit(0);
        }
        wait(&pid);
    }

    if (redirect) {
        dup2(copy, STDOUT_FILENO);
    }
}

// done
void process_command(char* command, char* red_command) {
    char* split_tokens = " ";
    char* dup_command = strdup(command);

    char* token_ptr = command;
    int arg_count = 0;
    char* token = strtok_r(token_ptr, split_tokens, &token_ptr);
    while (token) {
        arg_count++;
        token = strtok_r(token_ptr, split_tokens, &token_ptr);
    }

    token_ptr = dup_command;
    char** args = (char**)malloc((arg_count + 1) * sizeof(char*));
    int index = 0;
    token = strtok_r(token_ptr, split_tokens, &token_ptr);
    while (token) {
        args[index++] = token;
        token = strtok_r(token_ptr, split_tokens, &token_ptr);
    }
    args[arg_count] = NULL;

    if (!arg_count) {
        error();
        return;
    }

    if (redirect) {
        if (!strcmp(args[0], "exit") || !strcmp(args[0], "cd") || !strcmp(args[0], "pwd")) {
            error();
            return;
        }
    }

    do_command(args, arg_count, red_command);
}

void preprocess(char* command) {
    if (is_redirect(command)) {
        char* orig = strdup(command);

        char* split_red = ">";
        char* new_ptr = command;
        char* new_command = strtok_r(new_ptr, split_red, &new_ptr);
        new_command = trim(new_command);

        char* red_command = strtok_r(new_ptr, split_red, &new_ptr);
        red_command = trim(red_command);

        if (new_command && red_command && is_redirect(orig) <= 1) {
            redirect = 1;
            process_command(new_command, red_command);
            redirect = 0;
        } else {
            error();
        }

    } else {
        process_command(command, NULL);
    }
}

int main(int argc, char *argv[]) 
{
    int line_size = 1500;
    int max_line = 512;
    char cmd_buff[line_size];
    FILE* file = fopen(argv[1], "r");
    if (file == NULL || argc > 2) {
        error();
    }

    char* split_commands = ";";
    while (fgets(cmd_buff, line_size, file)) {
        char* orig_command = strdup(cmd_buff);
        int len = strlen(cmd_buff);
        
        // remove tabs
        for (int i = 0; i < len; i++) {
            if (cmd_buff[i] == '\t') {
                cmd_buff[i] = ' ';
            }
        }

        // remove newline
        cmd_buff[len - 1] = '\0';
        len = strlen(cmd_buff);

        // ignore blank inputs
        if (non_empty_line(cmd_buff)) {
            myPrint(orig_command);

            if (len <= max_line) {
                char* command_ptr = cmd_buff;
                char* command = strtok_r(command_ptr, split_commands, &command_ptr);
                while (command) {
                    if (non_empty_line(command)) {
                        preprocess(command);   
                    }
                    command = strtok_r(command_ptr, split_commands, &command_ptr);
                }
            } else {
                error();
            }
        }
    }
    fclose(file);
    return 0;
}