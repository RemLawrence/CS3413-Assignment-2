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
    if(buffer[len-1] == '\n'){ // Remove Enter
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
        parse(cmd, argv, buffer, &isPipe, &pipeNumber);

        printf("isPipe %d\n", isPipe);
        printf("pipeNumber %d\n", pipeNumber);
        
        if (strcmp(argv[0], "exit") == 0) {  /* is it an "exit"?     */
            printf("Quit Shell\n");
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

/*
void getPipedCommand(char** argv, char** pipedCmd) {
    int index = 0;
    for(index = 0; (strcmp(argv[0], "|") != 0); index++){
        pipedCmd[index] = argv[0];
        argv = argv + 1;
        printf("index %d\n", index);
    }
    argv = argv + 1;
    pipedCmd[index + 1] = "\0";
}
*/

void execute_pipe_command(int pipeNumber, char** argv) {
    // At this point, the cmd are at least in this format: ls /usr/bin | grep a
    int pipes[pipeNumber][2];
    for(int i = 0; i < pipeNumber; i++) {
        if(pipe(pipes[i]) < 0) {
            printf("An error occured with opening the pipe.\n");
            exit(0);
        }
    }

    for(int i = 0; i < pipeNumber; i++) {
        if(i == 0) {
            // First pipe cmd
            int pid1 = fork();
            if(pid1 != 0) {
                // parent work
            }
            else { // pid == 0, child's work
                close(pipes[i][0]); // Close the unused piper end
                dup2(pipes[i][1], 1); // Replace stdout with pipew . Stdout is disabled

                char *pipedCmd[1024]; /* the command line argument      */
                int index = 0;
                for(index = 0; (strcmp(argv[0], "|") != 0); index++){
                    pipedCmd[index] = argv[0];
                    argv = argv + 1;
                    //printf("index %d\n", index);
                }
                argv = argv + 1;
                pipedCmd[index + 1] = "\0";

                printf("1 pipedCmd[0] %s\n", pipedCmd[0]);
                printf("1 pipedCmd[1] %s\n", pipedCmd[1]);
                printf("1 pipedCmd[2] %s\n", pipedCmd[2]);
                printf("1 argv[0] %s\n", argv[0]);

                int return_code = execvp(*pipedCmd, pipedCmd); // execute the command
                //write(pipe[1], stdout, 1);
                close(pipes[i][1]);
                if(return_code != 0 || argv[0] == NULL) {
                    printf("Your command is invalid. Please re-enter one.\n");
                    exit(0);
                }
            }
            close(pipes[i][0]);
            close(pipes[i][1]);
            waitpid(pid1, NULL, 0); // Just wait until children's done.
        }
        else {
            // then this process has the responsibility to read from & write to the pipe
            int pid2 = fork();
            if(pid2 != 0) {
                // parent work
                
            }   
            else { // pid == 0, child's work
                close(pipes[i-1][1]); // Close the unused last pipew end
                close(pipes[i][0]); // Close the unused this piper
                dup2(pipes[i-1][0], 0); // Replace stdin with last piper
                dup2(pipes[i][1], 1); // Replace stdout with this pipew
                    
                char *pipedCmd[1024]; /* the command line argument      */
                int index = 0;
                for(index = 0; (strcmp(argv[0], "|") != 0); index++){
                    pipedCmd[index] = argv[0];
                    argv = argv + 1;
                    //printf("index %d\n", index);
                }
                argv = argv + 1;
                pipedCmd[index + 1] = "\0";

                printf("2 pipedCmd[0] %s\n", pipedCmd[0]);
                printf("2 pipedCmd[1] %s\n", pipedCmd[1]);
                printf("2 pipedCmd[2] %s\n", pipedCmd[2]);
                printf("2 argv[0] %s\n", argv[0]);

                int return_code = execvp(*pipedCmd, pipedCmd); // execute the command
                close(pipes[i-1][0]);
                close(pipes[i][1]);
                if(return_code != 0) {
                    printf("Your command is invalid. Please re-enter one.\n");
                    exit(0);
                }
                //exit(0);
            }
            waitpid(pid2, NULL, 0);
            close(pipes[i-1][1]); // Close the unused last pipew end
            close(pipes[i][0]); // Close the unused this piper
            close(pipes[i-1][0]);
            close(pipes[i][1]);
        }

        if (i == pipeNumber -1) { // If this loop can do the Last pipe cmd
                // then finish the pipe and starting to write stdout
                // Could come from any of the pipew process || piper && pipew process
                int pid3 = fork();
                if(pid3 != 0) {
                    // parent work
                
                }   
                else { // pid == 0, child's work
                    close(pipes[i][1]); // Close the unused pipew end
                    dup2(pipes[i][0], 0); // Replace stdin with piper
                    
                    char *pipedCmd[1024]; /* the command line argument      */
                    int index = 0;
                    for(index = 0; (strcmp(argv[0], "|") != 0); index++) {
                        pipedCmd[index] = argv[0];
                        argv = argv + 1;
                        //printf("index %d\n", index);
                    }
                    argv = argv + 1;
                    pipedCmd[index + 1] = "\0";

                    // printf("3 pipedCmd[0] %s\n", pipedCmd[0]);
                    // printf("3 pipedCmd[1] %s\n", pipedCmd[1]);
                    // printf("3 pipedCmd[2] %s\n", pipedCmd[2]);
                    // printf("3 argv[0] %s\n", argv[0]);
                    
                    argv[0] = "grep";
                    argv[1] = "c";
                    argv[2] = NULL;
                    int return_code = execvp(*argv, argv); // execute the command
                    close(pipes[i][0]);
                    if(return_code != 0) {
                        printf("Your command is invalid. Please re-enter one.\n");
                        exit(0);
                    }
                    //exit(0);
                }
                waitpid(pid3, NULL, 0);
                close(pipes[i][0]);
                close(pipes[i][1]);
            } 
    }
    //return 0;
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