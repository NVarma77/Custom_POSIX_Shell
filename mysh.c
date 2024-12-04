#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>     
#include <sys/wait.h>   
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <signal.h>

#define MAX_INPUT 1024
#define MAX_TOKENS 128
#define PATHS_SIZE 4
#define PATH_MAX 4096
#define FULL_PATH_MAX 8092
//Paths to search for executables
char *search_paths[PATHS_SIZE] = {"/usr/local/bin", "/usr/bin", "/bin", NULL};

int last_exit_status = 0;

void print_prompt() 
{
    printf("mysh> ");
    fflush(stdout);
}

void print_goodbye() 
{
    printf("mysh: exiting\n");
}

void print_welcome() 
{
    printf("Welcome to my shell!\n");
}

//Function prototypes
void handle_signal(int sig) 
{
    //Handle signals if necessary
    //For example, ignore Ctrl+C in the shell
    if (sig == SIGINT) 
    {
        printf("\n");
        //Re-display prompt if interactive
        if (isatty(STDIN_FILENO)) 
        {
            print_prompt();
            fflush(stdout);
        }
    }
}

int is_builtin(char *command) 
{
    return strcmp(command, "cd") == 0 || strcmp(command, "pwd") == 0 || strcmp(command, "which") == 0 || strcmp(command, "exit") == 0;
}

void expand_wildcards(char **args) 
{
    int i = 0;
    char *new_args[MAX_TOKENS];
    int new_argc = 0;
    char full_paths[MAX_TOKENS][PATH_MAX];

    while (args[i] != NULL) 
    {
        char *arg = args[i];
        if (strchr(arg, '*') != NULL) 
        {
            //Wildcard detected
            char dir_path[PATH_MAX];
            char pattern[PATH_MAX];
            char *slash = strrchr(arg, '/');
            if (slash != NULL) 
            {
                //There is a directory path
                strncpy(dir_path, arg, slash - arg);
                dir_path[slash - arg] = '\0';
                strcpy(pattern, slash + 1);
            } 
            else 
            {
                strcpy(dir_path, ".");
                strcpy(pattern, arg);
            }

            DIR *dir = opendir(dir_path);
            if (dir == NULL) 
            {
                // Directory cannot be opened
                new_args[new_argc++] = arg;
                i++;
                continue;
            }

            struct dirent *entry;
            int matched = 0;
            while ((entry = readdir(dir)) != NULL) 
            {
                if (entry->d_name[0] == '.' && pattern[0] != '.') 
                {
                    // Skip hidden files unless pattern starts with '.'
                    continue;
                }

                // Simple pattern matching: prefix*suffix
                char *asterisk = strchr(pattern, '*');
                if (asterisk != NULL) 
                {
                    char prefix[PATH_MAX] = "";
                    char suffix[PATH_MAX] = "";
                    strncpy(prefix, pattern, asterisk - pattern);
                    prefix[asterisk - pattern] = '\0';
                    strcpy(suffix, asterisk + 1);

                    if (strncmp(entry->d_name, prefix, strlen(prefix)) == 0 && strlen(entry->d_name) >= strlen(prefix) + strlen(suffix) && strcmp(entry->d_name + strlen(entry->d_name) - strlen(suffix), suffix) == 0) 
                    {
                        //Match found
                        char full_path[FULL_PATH_MAX];
                        if (slash != NULL)
                        {
                            snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
                        } 
                        else 
                        {
                            strcpy(full_path, entry->d_name);
                        }
                        strcpy(full_paths[new_argc], full_path);
                        new_args[new_argc] = full_paths[new_argc];
                        new_argc++;
                        matched = 1;
                    }
                }
            }
            closedir(dir);

            if (!matched) 
            {
                // No match found, include the original pattern
                new_args[new_argc++] = arg;
            }
        } 
        else 
        {
            //No wildcard, copy argument
            new_args[new_argc++] = arg;
        }
        i++;
    }
    new_args[new_argc] = NULL;

    //Replace original args with expanded args
    for (i = 0; i < new_argc; i++) 
    {
        args[i] = new_args[i];
    }
    args[new_argc] = NULL;
}

void execute_builtin(char **args, int interactive) 
{
    if (strcmp(args[0], "cd") == 0) 
    {
        if (args[1] == NULL || args[2] != NULL) 
        {
            fprintf(stderr, "cd: Wrong number of arguments\n");
            last_exit_status = 1;
        } 
        else 
        {
            if (chdir(args[1]) != 0) 
            {
                perror("cd");
                last_exit_status = 1;
            } 
            else 
            {
                last_exit_status = 0;
            }
        }
    } 
    else if (strcmp(args[0], "pwd") == 0) 
    {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) 
        {
            printf("%s\n", cwd);
            last_exit_status = 0;
        }
        else 
        {
            perror("pwd");
            last_exit_status = 1;
        }
    } 
    else if (strcmp(args[0], "which") == 0) 
    {
        if (args[1] == NULL || args[2] != NULL) 
        {
            last_exit_status = 1;
        } 
        else 
        {
            char *cmd = args[1];
            char path[PATH_MAX];
            if (is_builtin(cmd)) 
            {
                last_exit_status = 1;
            } 
            else 
            {
                int i = 0;
                while (search_paths[i]) 
                {
                    snprintf(path, sizeof(path), "%s/%s", search_paths[i], cmd);
                    if (access(path, X_OK) == 0) 
                    {
                        printf("%s\n", path);
                        last_exit_status = 0;
                        return;
                    }
                    i++;
                }
                last_exit_status = 1;
            }
        }
    } 
    else if (strcmp(args[0], "exit") == 0) 
    {
        //Print arguments
        for (int i = 1; args[i] != NULL; i++) 
        {
            printf("%s ", args[i]);
        }
        
        if (interactive) 
        {
            print_goodbye();
        }
        exit(EXIT_SUCCESS);
    }
}


void execute_command(char **args, char *input_file, char *output_file, int interactive) 
{
    pid_t pid = fork();
    if (pid == -1) 
    {
        perror("fork");
        last_exit_status = 1;
        return;
    } 
    else if (pid == 0) 
    {
        //Child process

        //Input redirection
        if (input_file) 
        {
            int fd_in = open(input_file, O_RDONLY);
            if (fd_in == -1) 
            {
                perror("open input_file");
                exit(EXIT_FAILURE);
            }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }

        // Output redirection
        if (output_file) 
        {
            int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0640);
            if (fd_out == -1)
            {
                perror("open output_file");
                exit(EXIT_FAILURE);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }

        // Execute command
        char *cmd = args[0];
        char path[PATH_MAX];
        if (strchr(cmd, '/')) 
        {
            // Pathname provided
            execv(cmd, args);
            perror("execv");
            exit(EXIT_FAILURE);
        } 
        else 
        {
            // Search in predefined paths
            int i = 0;
            while (search_paths[i]) 
            {
                snprintf(path, sizeof(path), "%s/%s", search_paths[i], cmd);
                execv(path, args);
                i++;
            }
            fprintf(stderr, "%s: command not found\n", cmd);
            exit(EXIT_FAILURE);
        }
    } 
    else 
    {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) 
        {
            last_exit_status = WEXITSTATUS(status);
            if (interactive && last_exit_status != 0) 
            {
                printf("mysh: Command failed with code %d\n", last_exit_status);
            }
        } 
        else if (WIFSIGNALED(status)) 
        {
            if (interactive) 
            {
                printf("mysh: Terminated by signal %d\n", WTERMSIG(status));
            }
            last_exit_status = 1;
        }
    }
}

void execute_pipeline(char **args1, char *input_file, char **args2, char *output_file, int interactive) 
{
    int pipefd[2];
    if (pipe(pipefd) == -1) 
    {
        perror("pipe");
        last_exit_status = 1;
        return;
    }

    pid_t pid1 = fork();
    if (pid1 == -1) 
    {
        perror("fork");
        last_exit_status = 1;
        return;
    }

    if (pid1 == 0) 
    {
        //First child process
        if (input_file) 
        {
            int fd_in = open(input_file, O_RDONLY);
            if (fd_in == -1) 
            {
                perror("open input_file");
                exit(EXIT_FAILURE);
            }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        //Execute first command
        char *cmd = args1[0];
        char path[PATH_MAX];
        if (strchr(cmd, '/')) 
        {
            execv(cmd, args1);
            perror("execv");
            exit(EXIT_FAILURE);
        } 
        else 
        {
            int i = 0;
            while (search_paths[i]) 
            {
                snprintf(path, sizeof(path), "%s/%s", search_paths[i], cmd);
                execv(path, args1);
                i++;
            }
            fprintf(stderr, "%s: command not found\n", cmd);
            exit(EXIT_FAILURE);
        }
    }

    pid_t pid2 = fork();
    if (pid2 == -1) 
    {
        perror("fork");
        last_exit_status = 1;
        return;
    }

    if (pid2 == 0) 
    {
        //Second child
        if (output_file) 
        {
            int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0640);
            if (fd_out == -1) 
            {
                perror("open output_file");
                exit(EXIT_FAILURE);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        //Execute second command
        char *cmd = args2[0];
        char path[PATH_MAX];
        if (strchr(cmd, '/')) 
        {
            execv(cmd, args2);
            perror("execv");
            exit(EXIT_FAILURE);
        } 
        else 
        {
            int i = 0;
            while (search_paths[i]) 
            {
                snprintf(path, sizeof(path), "%s/%s", search_paths[i], cmd);
                execv(path, args2);
                i++;
            }
            fprintf(stderr, "%s: command not found\n", cmd);
            exit(EXIT_FAILURE);
        }
    }

    //close pipe i/o
    close(pipefd[0]);
    close(pipefd[1]);

    //Wait for both children
    int status;
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);

    if (WIFEXITED(status)) 
    {
        last_exit_status = WEXITSTATUS(status);
        if (interactive && last_exit_status != 0) 
        {
            printf("mysh: Command failed with code %d\n", last_exit_status);
        }
    } 
    else if (WIFSIGNALED(status)) 
    {
        if (interactive) 
        {
            printf("mysh: Terminated by signal %d\n", WTERMSIG(status));
        }
        last_exit_status = 1;
    }
}


void parse_and_execute(char *command_line, int interactive) 
{
    if (command_line[0] == '\0') 
    {
        return;
    }

    //tokenize
    char *tokens[MAX_TOKENS];
    int num_tokens = 0;
    char *token = strtok(command_line, " \t");
    while (token != NULL && num_tokens < MAX_TOKENS - 1)
    {
        tokens[num_tokens++] = token;
        token = strtok(NULL, " \t");
    }
    tokens[num_tokens] = NULL;

    if (num_tokens == 0) 
    {
        return;
    }

    //built-in commands
    if (is_builtin(tokens[0])) 
    {
        execute_builtin(tokens, interactive);
        return;
    }

    // Check for pipes
    int pipe_pos = -1;
    for (int i = 0; i < num_tokens; i++) 
    {
        if (strcmp(tokens[i], "|") == 0) 
        {
            pipe_pos = i;
            if(pipe_pos == num_tokens - 1)
            {
                //pipe is last token edgecase
                perror("Pipe cannot be last token in command stream");
                exit(EXIT_FAILURE);
            }
            if(pipe_pos == 0)
            {
                //pipe is first token edge case
                perror("Pipe cannot be first token in command stream");
                exit(EXIT_FAILURE);
            }
            break;
        }
    }

    if (pipe_pos != -1) 
    {
        //split command line into two by pipeline
        tokens[pipe_pos] = NULL;
        char **args1 = &tokens[0];
        char **args2 = &tokens[pipe_pos + 1];

        //redirection handling
        char *input_file = NULL;
        char *output_file = NULL;

        //input redirection for the first command
        //output redirection goes through pipe so n/a
        for (int i = 0; args1[i] != NULL; i++) 
        {
            if (strcmp(args1[i], "<") == 0) 
            {
                args1[i] = NULL;
                input_file = args1[i + 1];
                break;
            }
        }

        //output redirection for the second command
        for (int i = 0; args2[i] != NULL; i++) 
        {
            if (strcmp(args2[i], ">") == 0) {
                args2[i] = NULL;
                output_file = args2[i + 1];
                break;
            }
        }

        //wildcard seperation for each half of pipe
        expand_wildcards(args1);
        expand_wildcards(args2);

        //final execution
        execute_pipeline(args1, input_file, args2, output_file, interactive);
    } 
    else 
    {
        //general case - no pipe
        char **args = tokens;
        char *input_file = NULL;
        char *output_file = NULL;

        //Handle redirection both ways 
        for (int i = 0; args[i] != NULL; i++) 
        {
            if (strcmp(args[i], "<") == 0) 
            {
                args[i] = NULL;
                input_file = args[i + 1];
                i++;
            } else if (strcmp(args[i], ">") == 0) 
            {
                args[i] = NULL;
                output_file = args[i + 1];
                i++;
            }
        }

        //Expand wildcards
        expand_wildcards(args);

        //final execution
        execute_command(args, input_file, output_file, interactive);
    }
}


int main(int argc, char *argv[]) 
{
    if (argc > 2) {
        perror("Too many arguments");
        return EXIT_FAILURE;
    }

    if (argc == 2) 
    {
        int batchFD = open(argv[1], O_RDONLY);
        dup2(batchFD, STDIN_FILENO);
        close(batchFD);
        
    }

    // Determine interactive mode based on the input stream
    int interactive = isatty(STDIN_FILENO);
    if (interactive) 
    {
        print_welcome();
    }

    // Handle SIGINT (Ctrl+C)
    signal(SIGINT, handle_signal);

    char buffer[MAX_INPUT];
    int bytes_read;
    int iterate = 0;

    while (1) 
    {
        if (interactive) 
        {
            print_prompt();
        }

        //initialize buffer and iterate for use
        iterate = 0;
        memset(buffer, 0, MAX_INPUT);
        while (1) 
        {
            bytes_read = read(STDIN_FILENO, &buffer[iterate], 1);
            if (bytes_read == -1) 
            {
                if (errno == EINTR) 
                {
                    continue; //Interrupted by signal; reread 
                } 
                else 
                {
                    perror("read");
                    return EXIT_FAILURE;
                }
            } 
            else if (bytes_read == 0) 
            {
                //EOF condition
                if (iterate == 0) 
                {
                    // No input, exit
                    if (interactive) 
                    {
                        print_goodbye();
                    }
                    return EXIT_SUCCESS;
                } 
                else 
                {
                    //set last to null
                    buffer[iterate] = '\0';
                    break;
                }
            } 
            else 
            {
                //general case read chars
                if (buffer[iterate] == '\n') 
                {
                    //if enter then set to null
                    buffer[iterate] = '\0';
                    break;
                }
                iterate++;
                //increase iterate value
                if (iterate >= MAX_INPUT - 1) 
                {
                    fprintf(stderr, "Input too long\n");
                    return EXIT_FAILURE;
                }
            }
        }

        //Process the command line
        parse_and_execute(buffer, interactive);
    }

    return EXIT_SUCCESS;
}

