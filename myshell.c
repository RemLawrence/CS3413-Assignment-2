/* A Simple Shell for CS3413 Assignment 2
    Author: Micah Hanmin Wang
    2021-10-15
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <signal.h>

#define BUFSIZE 1024
int pid = -4396; // random pid for initialization (this is not a 'magic number', just a random integer for initialization)
int contpid = -4396; // pid. Used for tracking those paused processes for now. (this is not a 'magic number', just a random integer for initialization)
void execute_pipe_command(int, char**);

/**
 * Parse the Command into a string array
 **/
void parse(char *line, char **argv, char *buffer, bool *isPipe, int *pipeNumber)
{
    // child work
    int len = strlen(buffer); // length of entered command
    if(buffer[len-1] == '\n') { // Remove Enter
        buffer[len-1] = '\0';
    }
    char* tokenizedStr;
    int cmdIndex = 0;
    while ((tokenizedStr = strtok_r(line, " ", &line)) && cmdIndex <= 100) {
        argv[cmdIndex] = tokenizedStr;
        cmdIndex++;

        if((strcmp(tokenizedStr, "|") == 0) && cmdIndex > 1){
            *isPipe = true; // Check if this cmd requires pipe
            *pipeNumber = *pipeNumber + 1;
        }
    }
    argv[cmdIndex] = NULL;
}

/**
 * Signal handler for SIGTSTP.
 * Decide if there's a job that needs to be suspended
 **/
void sig_handler()
{
    // First check if the child is running
    if (kill(pid, 0) == 0 || kill(contpid, 0) == 0) {
        // There are jobs running
        printf("\nThe job is suspsnded. Type 'fg' to resume. (Or 'bg' if you want)\n"); 
    }
    else if (kill(pid, 0) == -1) {
        // No job currently running
        printf("\nThere currently is no job running! Shell is still alive. Type a command below!\n"); // If child isn not running, print this message
    }
    signal(SIGTSTP, sig_handler); // re-subscribe
}

/**
 * Used to read and execute the command using execvp
 * Also deals with the SIGTSTP signal with internal command: fg and bg
 **/
void execute_command(char *cmd, char *buffer) {
    // Catch Ctrl+Z
    if (signal(SIGTSTP, sig_handler) == SIG_ERR) {
        perror("\nCan't catch SIGTSTP\n");
    }

    while(cmd != NULL) {
        char *argv[1024]; /* the command line argument      */
        bool changeDir = false; // If the command is cd xxx ?
        bool isPipe = false; // If the command needs pipe?
        int pipeNumber = 0; // Number of pipes needed
        bool isfg = false; // Detect if the user entered fg
        int status; // Used for tracking the process's status
        int paused_status; // Used for tracking the paused process's status
        
        // Print a prompt and read a command from standard input
        if(!isfg) {
            printf("%s %% ", getcwd(NULL, 1024)); // Get the current dir.
        }
        cmd = fgets(buffer, BUFSIZE, stdin);

        if(strcmp(cmd, "\n") == 0) {
            // If the user has hit just an enter, catch it to prevent segfault
            printf("Your command is invalid. Please re-enter one.\n");
        }
        else {
            // If the user didn't just hit an enter, then parse the command normally.
            parse(cmd, argv, buffer, &isPipe, &pipeNumber);
        }

        if((strcmp(argv[0], "cd") == 0) && (argv[1] != NULL)) {
            // Detect if the user wants to change dir
            changeDir = true;
            if (chdir(argv[1]) != 0) {
                perror("chdir() failed");
                exit(0);
            }
        }

        if (strcmp(argv[0], "exit") == 0) {  /* is it an "exit"?     */
            printf("Quit Shell\n");
            exit(0); // Exit from the parent (shell)
        }

        if (pipeNumber > 100) {  // if this command requires more than 100 pipes
            printf("This shell program only supports maximum 100 number of commands :)\n");
            isPipe = false;
        }

        pid = fork();
        if(pid < 0) {
            printf("Fork failed");
            exit(0);
        }
        else if(pid > 0) {
            // parent work
            if(strcmp(cmd, "fg") == 0 && contpid != -4396) {
                isfg = true; // isfg flag set to true -- command execution and shell printout are blocked

                if(kill(contpid, SIGCONT) == -1) {
                    perror("Continue process failed");
                } // Continue the tracked process
                
                int ifexit = waitpid(contpid, &paused_status, WUNTRACED); // and let parent wait for it
                if (ifexit == -1){
                    printf("waitpid failde!");
                }
                if(ifexit == contpid) {
                    //Check if waitpid has finished. If finished, then restore the fg flag to false.
                    isfg = false;
                }
            }
            if(strcmp(cmd, "bg") == 0) {
                if(kill(contpid, SIGCONT) == -1) {
                    perror("Continue process failed");
                } // Continue the tracked process without locking the shell resources
            }

            do {
                if(waitpid(pid, &status, WUNTRACED) == -1) {
                    perror("waitpid failed!");
                }

                if (WIFSTOPPED(status) == 1)
                {
                    //Child has been suspended.
                    contpid = pid; // Record the suspended process id (contpid)
                    break; // break out of the loop
                }
                
            } while(!WIFEXITED(status));
        }
        else {
            // pid == 0, child's work
            if(kill(contpid, 0) == -1) {
                // If there is no paused/unfinised process, then execute a new command is allowed.
                if(isPipe) {
                    // Execute command with pipe(s)
                    execute_pipe_command(pipeNumber, argv);
                    exit(0);
                }
                else {
                    if(!changeDir && !isfg) {
                        // Execute the command normally
                        int return_code = execvp(*argv, argv); // execute the command
                        if(return_code != 0 || argv[0] == NULL) {
                            printf("Your command is invalid. Please re-enter one.\n");
                            exit(0);
                        }
                    }
                    exit(0);
                }
            }
            else {
                if(strcmp(cmd, "fg") == 0 || strcmp(cmd, "bg") == 0){
                    // Only internal commands allowed when a process is running
                }
                else {
                    // If the paused/continued process hasn't exited, then don't allow any new commands to be executed.  
                    printf("Not allowed to start new command while you have a job active.\n"); 
                }  
                exit(0); // This is important. It allows to exit out of this children.
            }
        }
    }
}

/**
 * Execute commands with pipes
 **/
void execute_pipe_command(int pipeNumber, char** argv) {
    // At this point, the cmd are at least in this format: ls /usr/bin | grep a
    int pipes[pipeNumber][2];
    int pipeIndex = 0;
    for(pipeIndex = 0; pipeIndex < pipeNumber; pipeIndex++) {
        // Initialize/Create the pipe(s)
        if(pipe(pipes[pipeIndex]) < 0) {
            perror("An error occured with opening the pipe.\n");
            exit(0);
        }
    }

    int i = 0;
    for(i = 0; i < pipeNumber; i++) {
        char *pipedCmd[1024] = {0}; // Used for storing separate commands from the argv[]
        int index = 0;
        for(index = 0; (strcmp(argv[0], "|") != 0); index++) {
            // This should extract the command between '|'
            pipedCmd[index] = argv[0]; // Store the separate command into pipedCmd[]
            argv = argv + 1; // Increase the original cmd pointer
        }
        argv = argv + 1; // Increase the pointer to skip '|'


        if(i == 0) {
            /**********************Process only writes to pipe (first command)**********************/
            int pid1 = fork();
            if(pid1 < 0) {
                printf("Fork failed");
                exit(0);
            }
            else if(pid1 > 0) {
                // parent work
                if(waitpid(pid1, NULL, 0) == -1) {
                    perror("waitpid1 failed!");
                }
            }
            else { // pid == 0, child's work
                if(close(pipes[i][0]) == -1) {
                    perror("Close the unused piper end failed!");
                } // Close the unused piper end
                if(dup2(pipes[i][1], 1) == -1) { // Replace stdout with pipew . Stdout is disabled
                    perror("Replace stdout with pipe's write end failed!");
                } 

                int return_code = execvp(*pipedCmd, pipedCmd); // execute the command
                //write(pipe[1], stdout, 1);
                if(close(pipes[i][1]) == -1) {
                    perror("Close the pipew end failed!");
                } // After using pipe's write end, close it (in child)
                if(return_code != 0 || argv[0] == NULL) {
                    printf("Your command is invalid. Please re-enter one.\n");
                    exit(0);
                }
            }
            if(close(pipes[i][1]) == -1) {
                perror("Close the pipew end (in parent) failed!");
            } // Close pipe's write end in parent. This line is important!
        }
        else {
            /**********************Processes only read && write to pipe**********************/
            // then this process has the responsibility to read from & write to the pipe
            int pid2 = fork();
            if(pid2 < 0) {
                printf("Fork failed");
                exit(0);
            }
            else if(pid2 > 0) {
                // parent work
                if(waitpid(pid2, NULL, 0) == -1) {
                    perror("waitpid2 failed!");
                }
            }
            else { // pid == 0, child's work
                if(close(pipes[i][0]) == -1) {
                    perror("Close the unused piper end failed!");
                } // Close the unused this piper
                if(dup2(pipes[i-1][0], 0) == -1) {
                    perror("Replace stdin with pipe's read end failed!");
                } // Replace stdin with last piper
                if(dup2(pipes[i][1], 1) == -1) {
                    perror("Replace stdout with pipe's write end failed!");
                } // Replace stdout with this pipew

                int return_code = execvp(*pipedCmd, pipedCmd); // execute the command
                if(close(pipes[i-1][0]) == -1) {
                    perror("Close the last piper end failed!");
                } // Close last pipe's read end (in child)
                if(close(pipes[i][1]) == -1) {
                    perror("Close the pipew end failed!");
                } // Close this pipe's write end (in child)
                if(return_code != 0) {
                    printf("Your command is invalid. Please re-enter one.\n");
                    exit(0);
                }
            }
            if(close(pipes[i-1][0]) == -1) {
                perror("Close last piper end (in parent) failed!");
            } // Close last pipe's read end (in parent)
            if(close(pipes[i][1]) == -1) {
                perror("Close this pipew end (in parent) failed!");
            } // Close this pipe's write end (in parent)
        }


        if (i == pipeNumber - 1) {
            /**********************Process reads from pipe and write to stdout (last command)**********************/
            // then finish the pipe and starting to write stdout
            int pid3 = fork();
            if(pid3 < 0) {
                printf("Fork failed");
                exit(0);
            }
            else if(pid3 > 0) {
                // parent work
                if(waitpid(pid3, NULL, 0) == -1) {
                    perror("waitpid3 failed!");
                }
            }
            else { // pid == 0, child's work
                if(dup2(pipes[i][0], 0) == -1) {
                    perror("Replace stdin with pipe's read end failed!");
                } // Replace stdin with piper

                char* lastCmd[1024];
                int lastPipedCmdIndex = 0;
                for(lastPipedCmdIndex = 0; argv[0] != NULL; lastPipedCmdIndex++){
                    // Get the last command to execute
                    lastCmd[lastPipedCmdIndex] = argv[0];
                    argv = argv + 1;
                }
                argv = argv + 1;
                lastCmd[lastPipedCmdIndex + 1] = "\0";

                int return_code = execvp(*lastCmd, lastCmd); // execute the command
                if(close(pipes[i][0]) == -1) {
                    perror("Close the piper end failed!");
                } // Close this pipe's read end (in child)
                if(return_code != 0) {
                    printf("Your command is invalid. Please re-enter one.\n");
                    exit(0);
                }
            }
            if(close(pipes[i][0]) == -1) {
                perror("Close the piper end (in parent) failed!");
            } // Close this pipe's read end (in parent)
        }
    }
}

int main(void) {
    
    char cmd[1024];		// pointer to entered command

    char buffer[BUFSIZE];	// room for 80 chars plus \0

    execute_command(cmd, buffer);
}
