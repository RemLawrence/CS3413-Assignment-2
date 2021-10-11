#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <signal.h>

#define BUFSIZE 1024
int pid = 4396; // random pid for initialization
void execute_pipe_command(int, char**);
void sig_handler(int);
int a = 0;
int b = 0;
int check = 0;

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
        //printf("%s\n", token);
    }
    argv[cmdIndex] = NULL;
}

void sig_handler(int signo)
{
    if (signo == SIGTSTP) {
        //sigset(SIGTSTP, SIG_DFL); // Set to default
        //kill(pid, SIGTSTP);
        // First check if the children are running
        if (kill(pid, 0) == 0) {
            // There are jobs running
            //b += 1;
            //printf("\nb %d\n", b);

            //printf("\nkill %d\n", kill(pid, 0));
            // It should suspend the child process, if the child is running.
            //sigset(SIGTSTP, SIG_IGN); //
            
            sigset(SIGTSTP, SIG_DFL);
            kill(pid, SIGTSTP);
            
            printf("\nThe job is suspsnded. Type 'fg' to resume.\n");
            //exit(0);
            //kill(pid, SIGCONT);
        }
        else if (kill(pid, 0) == -1) {
            // No job currently running
            //a += 1;
            //printf("\na %d\n", a);
            //sigset(SIGTSTP, SIG_IGN);
            //printf("\nkill %d\n", kill(pid, 0));
            //sigset(SIGTSTP, SIG_DFL);
            
            //sigset(SIGTSTP, SIG_DFL);
            printf(" There currently is no job running! Shell is still alive. Type a command below!\n"); // If child isn not running, print this message

        }
        signal(SIGTSTP, sig_handler); // re-subscribe
        //(0);
    }
}

void execute_command(char *cmd, char *buffer) {

    while(cmd != NULL) {
        char *argv[1024]; /* the command line argument      */
        bool changeDir = false; // If the command is cd xxx ?
        bool isPipe = false; // If the command needs pipe?
        int pipeNumber = 0;
        // Print a prompt and read a command from standard input
        printf("%s %% ", getcwd(NULL, 1024)); // Get the current dir.

        // Catch Ctrl+Z
        if (signal(SIGTSTP, sig_handler) == SIG_ERR) {
            printf("\nCan't catch SIGTSTP\n");
        }
        //printf("************************************************************************************************Sure\n");

        if(strcmp(cmd, "fg") == 0) {
            changeDir = true;
            printf("CONTINUE!\n");
            kill(check, SIGCONT); // NO USE. WHY?
        }

        cmd = fgets(buffer, BUFSIZE, stdin);
        if(strcmp(cmd, "\n") == 0) {
            // If the user has hit just a space
            printf("Your command is invalid. Please re-enter one.\n");
        }
        else {
            // If the user didn't just hit a space, then parse the command normally.
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
        }

        pid = fork();

        printf("pid %d\n", pid);

        if(pid != 0) {
            // parent work
            // kill(pid, SIGTSTP); // Send a SIGTSTP signal to all children from parent.
            // usleep(3000000);
            // kill(pid, SIGCONT); // Send a SIGCONT signal to all children from parent.

            printf("If the children are running: %d\n", check); // Children not running

            check = waitpid(pid, NULL, WUNTRACED); // Just wait until children's done.
            //printf("Papa parent is (done) waiting!\n");
        }
        else {
            // pid == 0, child's work
            if(isPipe) {
                //usleep(3000000);
                printf("(pipe child)If the children are running: %d\n", check); // Children not running
                execute_pipe_command(pipeNumber, argv);
                exit(0);
            }
            else if((strcmp(argv[0], "cd") == 0) && (argv[1] != NULL)) {
                changeDir = true;
                if (chdir(argv[1]) != 0) {
                    perror("chdir() failed");
                    exit(0);
                }
            }
            else {
                if((strcmp(argv[0], "cc") == 0)) {
                    for(int k = 0; k < 100; k++) {
                        usleep(1000000);
                        printf("%d\n", k);
                    }
                }
                else if(!changeDir) {
                    //usleep(3000000);
                    printf("(normal child)If the children are running: %d\n", check); // Children not running
                    
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
        //printf("index: %d\n", i);
        char *pipedCmd[1024] = {0}; // Used for storing separate commands from the argv[]
        int index = 0;
        for(index = 0; (strcmp(argv[0], "|") != 0); index++) {
            // This should extract the command between '|'
            pipedCmd[index] = argv[0]; // Store the separate command into pipedCmd[]
            argv = argv + 1; // Increase the original cmd pointer
        }
        //printf("argv[0] before: %s\n", argv[0]);
        argv = argv + 1; // Increase the pointer to skip '|'
        //printf("argv[0] now: %s\n", argv[0]);
        //pipedCmd[index + 1] = "\0";
        //printf("pipedCmd[0]: %s\n", pipedCmd[0]);


        if(i == 0) {
            /**********************Process only writes to pipe (first command)**********************/
            int pid1 = fork();
            printf("pid1 %d\n", pid1);
            //waitpid(pid1, NULL, 0);
            if(pid1 != 0) {
                // parent work
                waitpid(pid1, NULL, 0);
            }
            else { // pid == 0, child's work
                close(pipes[i][0]); // Close the unused piper end
                dup2(pipes[i][1], 1); // Replace stdout with pipew . Stdout is disabled

                // printf("1 pipedCmd[0] %s\n", pipedCmd[0]);
                // printf("1 pipedCmd[1] %s\n", pipedCmd[1]);
                // printf("1 pipedCmd[2] %s\n", pipedCmd[2]);
                // printf("1 argv[0] %s\n", argv[0]);

                int return_code = execvp(*pipedCmd, pipedCmd); // execute the command
                //write(pipe[1], stdout, 1);
                close(pipes[i][1]); // After using pipe's write end, close it (in child)
                if(return_code != 0 || argv[0] == NULL) {
                    printf("Your command is invalid. Please re-enter one.\n");
                    exit(0);
                }
            }
            close(pipes[i][1]); // Close pipe's write end in parent. This line is important!
        }
        else {
            /**********************Processes only read && write to pipe**********************/
            // then this process has the responsibility to read from & write to the pipe
            int pid2 = fork();
            printf("pid2 %d\n", pid2);
            //waitpid(pid2, NULL, 0);
            if(pid2 != 0) {
                // parent work
                waitpid(pid2, NULL, 0);
            }
            else { // pid == 0, child's work
                close(pipes[i-1][1]); // Close the unused last pipew end
                close(pipes[i][0]); // Close the unused this piper
                dup2(pipes[i-1][0], 0); // Replace stdin with last piper
                dup2(pipes[i][1], 1); // Replace stdout with this pipew

                // printf("2 pipedCmd[0] %s\n", pipedCmd[0]);
                // printf("2 pipedCmd[1] %s\n", pipedCmd[1]);
                // printf("2 pipedCmd[2] %s\n", pipedCmd[2]);
                // printf("2 argv[0] %s\n", argv[0]);

                int return_code = execvp(*pipedCmd, pipedCmd); // execute the command
                close(pipes[i-1][0]); // Close last pipe's read end (in child)
                close(pipes[i][1]); // Close this pipe's write end (in child)
                if(return_code != 0) {
                    printf("Your command is invalid. Please re-enter one.\n");
                    exit(0);
                }
                //exit(0);
            }
            close(pipes[i-1][0]); // Close last pipe's read end (in parent)
            close(pipes[i][1]); // Close this pipe's write end (in parent)
        }


        if (i == pipeNumber - 1) {
            /**********************Process reads from pipe and write to stdout (last command)**********************/
            // then finish the pipe and starting to write stdout
            int pid3 = fork();
            printf("pid3 %d\n", pid3);
            //waitpid(pid3, NULL, 0);
            if(pid3 != 0) {
                // parent work
                waitpid(pid3, NULL, 0);
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

                // printf("3 lastCmd[0] %s\n", lastCmd[0]);
                // printf("3 lastCmd[1] %s\n", lastCmd[1]);
                // printf("3 lastCmd[2] %s\n", lastCmd[2]);
                //printf("3 argv[0] %s\n", argv[0]);

                int return_code = execvp(*lastCmd, lastCmd); // execute the command
                close(pipes[i][0]); // Close this pipe's read end (in child)
                if(return_code != 0) {
                    printf("Your command is invalid. Please re-enter one.\n");
                    exit(0);
                }
                //exit(0);
            }
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



// void handler(int num) {
//     // overwrite the signal's function
//     write(STDOUT_FILENO, "I won't die\n", 13);
// }

// signal(SIGINT, handler) // I want my handler to run whenever a SIGINT is hit.
// signal(SIGTERM, handler) // I want my handler to run whenever a SIGINT is hit.
// signal(SIGKILL, handler) // SIGKILL is an order, not a request


// void seghandler(int num) {
//     // overwrite the signal's function
//     write(STDOUT_FILENO, "SEG FAULT!\n", 13);
// }

// signal(SIGSEGV, seghandler) // I want my handler to run whenever a SIGINT is hit.


//To reproduce:
// ls -la | grep a | grep c | grep e
// ls | grep p

//OR

// ls /usr/bin | more | grep c
// but ls -la | more | grep c

// ls -la | grep c | more | grep a | more | grep e | more | grep O
