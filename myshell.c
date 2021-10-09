#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

#define BUFSIZE 1024
void execute_pipe_command(int, char**);

void parse(char *line, char **argv, char *buffer, bool *isPipe, int *pipeNumber)
{
    // child work
    int len = strlen(buffer); // length of entered command
    if(buffer[len-1] == '\n') { // Remove Enter
        buffer[len-1] = '\0';
    }
    char* tokenizedStr;
    int cmdIndex = 0;
    while ((tokenizedStr = strtok_r(line, " ", &line)) && cmdIndex <= 100){
        argv[cmdIndex] = tokenizedStr;
        cmdIndex++;

        if((strcmp(tokenizedStr, "|") == 0) && cmdIndex > 1){
            *isPipe = true; // Check if this cmd requires pipe
            *pipeNumber = *pipeNumber + 1;
        }
        //printf("%s\n", token);
    }
    argv[cmdIndex] = '\0';
}

void execute_command(char *cmd, char *buffer) {
    while(cmd != NULL) {
        char *argv[1024]; /* the command line argument      */
        bool changeDir = false; // If the command is cd xxx ?
        bool isPipe = false; // If the command needs pipe?
        int pipeNumber = 0;
        // Print a prompt and read a command from standard input
        printf("%s %% ", getcwd(NULL, 1024)); // Get the current dir.

        cmd = fgets(buffer, BUFSIZE, stdin);
        if(strcmp(cmd, "\n") == 0) {
            printf("Your command is invalid. Please re-enter one.\n");
        }
        else {
            parse(cmd, argv, buffer, &isPipe, &pipeNumber);
        }
        
        // printf("isPipe %d\n", isPipe);
        // printf("pipeNumber %d\n", pipeNumber);
        
        if (strcmp(argv[0], "exit") == 0) {  /* is it an "exit"?     */
            printf("Quit Shell\n");
            exit(0);
        }

        if (pipeNumber > 100) {  // if this command requires more than 100 pipes
            printf("This shell program only supports maximum 100 number of commands :)\n");
            exit(0);
        }

        if((strcmp(argv[0], "cd") == 0) && (argv[1] != NULL)) {
            changeDir = true;
            chdir(argv[1]);
        }

        if(isPipe) {
            execute_pipe_command(pipeNumber, argv);
        }
        else {
            int pid = fork();
            if(pid != 0) {
                // parent work
                wait(NULL); // Just wait until children's done.
            }
            else { // pid == 0, child's work
                if(!changeDir) {
                    int return_code = execvp(*argv, argv); // execute the command
                    if(return_code != 0 || argv[0] == NULL) {
                        printf("Your command is invalid. Please re-enter one.\n");
                        exit(0);
                    }
                }
            }
        }
    }
}

void execute_pipe_command(int pipeNumber, char** argv) {
    // At this point, the cmd are at least in this format: ls /usr/bin | grep a
    int pipes[pipeNumber][2];
    int pipeIndex = 0;
    for(pipeIndex = 0; pipeIndex < pipeNumber; pipeIndex++) {
        // Initialize/Create the pipe(s)
        if(pipe(pipes[pipeIndex]) < 0) {
            printf("An error occured with opening the pipe.\n");
            exit(0);
        }
    }

    int i = 0;
    for(i = 0; i < pipeNumber; i++) {
        printf("index: %d\n", i);
        char *pipedCmd[1024] = {0}; // Used for storing separate commands from the argv[]
        int index = 0;
        for(index = 0; (strcmp(argv[0], "|") != 0); index++) {
            // This should extract the command between '|'
            pipedCmd[index] = argv[0]; // Store the separate command into pipedCmd[]
            argv = argv + 1; // Increase the original cmd pointer
        }
        printf("argv[0] before: %s\n", argv[0]);
        argv = argv + 1; // Increase the pointer to skip '|'
        printf("argv[0] now: %s\n", argv[0]);
        //pipedCmd[index + 1] = "\0";
        printf("pipedCmd[0]: %s\n", pipedCmd[0]);


        if(i == 0) {
            /**********************Process only writes to pipe (first command)**********************/
            int pid1 = fork();
            if(pid1 != 0) {
                // parent work
                //waitpid(pid1, NULL, 0);
            }
            else { // pid == 0, child's work
                close(pipes[i][0]); // Close the unused piper end
                dup2(pipes[i][1], 1); // Replace stdout with pipew . Stdout is disabled

                printf("1 pipedCmd[0] %s\n", pipedCmd[0]);
                printf("1 pipedCmd[1] %s\n", pipedCmd[1]);
                printf("1 pipedCmd[2] %s\n", pipedCmd[2]);
                printf("1 argv[0] %s\n", argv[0]);

                int return_code = execvp(*pipedCmd, pipedCmd); // execute the command
                //write(pipe[1], stdout, 1);
                close(pipes[i][1]); // After using pipe's write end, close it (in child)
                if(return_code != 0 || argv[0] == NULL) {
                    printf("Your command is invalid. Please re-enter one.\n");
                    exit(0);
                }
            }
            close(pipes[i][1]); // Close pipe's write end in parent. This line is important!
            waitpid(pid1, NULL, 0); // Just wait until children's done.
        }
        else {
            /**********************Processes only read && write to pipe**********************/
            // then this process has the responsibility to read from & write to the pipe
            int pid2 = fork();
            if(pid2 != 0) {
                // parent work
                //waitpid(pid2, NULL, 0);
            }   
            else { // pid == 0, child's work
                close(pipes[i-1][1]); // Close the unused last pipew end
                close(pipes[i][0]); // Close the unused this piper
                dup2(pipes[i-1][0], 0); // Replace stdin with last piper
                dup2(pipes[i][1], 1); // Replace stdout with this pipew

                printf("2 pipedCmd[0] %s\n", pipedCmd[0]);
                printf("2 pipedCmd[1] %s\n", pipedCmd[1]);
                printf("2 pipedCmd[2] %s\n", pipedCmd[2]);
                printf("2 argv[0] %s\n", argv[0]);

                int return_code = execvp(*pipedCmd, pipedCmd); // execute the command
                close(pipes[i-1][0]); // Close last pipe's read end (in child)
                close(pipes[i][1]); // Close this pipe's write end (in child)
                if(return_code != 0) {
                    printf("Your command is invalid. Please re-enter one.\n");
                    exit(0);
                }
                //exit(0);
            }
            waitpid(pid2, NULL, 0);
            close(pipes[i-1][0]); // Close last pipe's read end (in parent)
            close(pipes[i][1]); // Close this pipe's write end (in parent)
        }
       

        if (i == pipeNumber - 1) {
            /**********************Process reads from pipe and write to stdout**********************/
            // then finish the pipe and starting to write stdout
            int pid3 = fork();
            if(pid3 != 0) {
                // parent work
                //waitpid(pid3, NULL, 0);
            }   
            else { // pid == 0, child's work
                close(pipes[i-1][1]); // Close the unused last pipew end
                close(pipes[i-1][0]); // Close the unused last piper end
                close(pipes[i][1]); // Close the unused pipew end
                dup2(pipes[i][0], 0); // Replace stdin with piper

                char* lastCmd[1024];
                int lastPipedCmdIndex = 0;
                for(lastPipedCmdIndex = 0; argv[0] != NULL; lastPipedCmdIndex++){
                    // Get the last command to execute
                    lastCmd[lastPipedCmdIndex] = argv[0];
                    argv = argv + 1;
                }
                argv = argv + 1;
                lastCmd[lastPipedCmdIndex + 1] = "\0";

                printf("3 lastCmd[0] %s\n", lastCmd[0]);
                printf("3 lastCmd[1] %s\n", lastCmd[1]);
                printf("3 lastCmd[2] %s\n", lastCmd[2]);
                //printf("3 argv[0] %s\n", argv[0]);
                
                int return_code = execvp(*lastCmd, lastCmd); // execute the command
                close(pipes[i][0]); // Close this pipe's read end (in child)
                if(return_code != 0) {
                    printf("Your command is invalid. Please re-enter one.\n");
                    exit(0);
                }
                //exit(0);
            }
            
            waitpid(pid3, NULL, 0);
            close(pipes[i][0]); // Close this pipe's read end (in parent)
            close(pipes[i][1]); // Close this pipe's write end (in parent)
        }
        //printf("**************LOOP ENDS****************\n");
    }
}

int main(void) {
    char cmd[1024];		// pointer to entered command
    
    char buffer[BUFSIZE];	// room for 80 chars plus \0

    execute_command(cmd, buffer);

    exit(0);
}


/*
void handler(int num) {
    // overwrite the signal's function
    write(STDOUT_FILENO, "I won't die\n", 13);
}

signal(SIGINT, handler) // I want my handler to run whenever a SIGINT is hit.
signal(SIGTERM, handler) // I want my handler to run whenever a SIGINT is hit.
signal(SIGKILL, handler) // SIGKILL is an order, not a request


void seghandler(int num) {
    // overwrite the signal's function
    write(STDOUT_FILENO, "SEG FAULT!\n", 13);
}

signal(SIGSEGV, seghandler) // I want my handler to run whenever a SIGINT is hit.

pipe()
*/

//To reproduce:
// ls -la | grep a | grep c | grep e
// ls | grep p

//OR

// ls /usr/bin | more | grep c
// but ls -la | more | grep c

// ls -la | grep c | more | grep a
