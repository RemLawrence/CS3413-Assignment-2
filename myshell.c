#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

#define BUFSIZE 1024
bool isPipe = false;

void parse(char *line, char **argv, char *buffer)
{
    // child work
    int len = strlen(buffer); // length of entered command
    if(buffer[len-1] == '\n'){ // Remove Enter
        buffer[len-1] = '\0';
    }
    char* tokenizedStr;
    int cmdIndex=0;
    while ((tokenizedStr = strtok_r(line, " ", &line)) && cmdIndex<=100){
        argv[cmdIndex] = tokenizedStr;
        cmdIndex++;

        if((strcmp(tokenizedStr, "|") == 0)){
            isPipe = true; // Check if this cmd requires pipe
        }
        //printf("%s\n", token);
    }
    argv[cmdIndex] = '\0';
    printf("%d\n", isPipe);
}

void execute_command(char *cmd, char **argv, char *buffer) {
    
    while(cmd != NULL) {
        // Print a prompt and read a command from standard input
        printf("%s %% ", getcwd(NULL, 1024)); // Get the current dir.

        cmd = fgets(buffer, BUFSIZE, stdin);
        //cmd = fgets(buffer, BUFSIZE, stdin);
        parse(cmd, argv, buffer);
        //printf("cmd %s\n", cmd);
        //printf("*argv %s\n", *argv);
        printf("argv[0] %s\n", argv[0]);
        printf("argv[1] %s\n", argv[1]);
        printf("argv[2] %s\n", argv[2]);
        
        if (strcmp(argv[0], "exit") == 0) {  /* is it an "exit"?     */
            printf("Quit Shell\n");
            exit(0);
        }

        bool changeDir = false;
        if((strcmp(argv[0], "cd") == 0) && (argv[1] != NULL)) {
            changeDir = true;
            chdir(argv[1]);
        }

        int pid = fork();
        if(pid != 0) {
            // parent work
            wait(NULL);
        }
        else {
            if(!changeDir) {
                int return_code = execvp(*argv, argv); // execute the command
                
                if(return_code != 0) {
                    printf("Your command is invalid. Please re-enter one.\n");
                    exit(0);
                }
            }
            
        }
    }
}

int main(void) {
    char cmd[1024];		// pointer to entered command
    char *argv[64];            /* the command line argument      */
    
    char buffer[BUFSIZE];	// room for 80 chars plus \0

    int fileDescriptor[2];

    execute_command(cmd, argv, buffer);

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