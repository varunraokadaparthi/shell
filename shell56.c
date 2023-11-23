/*
 * file:        shell56.c
 * description: skeleton code for simple shell
 *
 */

/* <> means don't check the local directory */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

/* "" means check the local directory */
#include "parser.h"

/* you'll need these includes later: */
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <linux/limits.h>
#include "redirection.h"
#include "runcommands.h"
#include "fork.h"

void freeSubstringArray(int **array, int rows) {
    for (int i = 0; i < rows; i++) {
        free(array[i]);
    }
    free(array);
}

int pipeHelper(char *tokens[], int num_commands, int **commandsIndex) {

    // Save the STDIN and STDOUT fds
    int save_std_in = dup(STDIN_FILENO);
    int save_std_out = dup(STDOUT_FILENO);

    int status = 0;

    int pipefds[num_commands - 1][2];
    pid_t child_pids[num_commands];
    pid_t child_pid;
    for (int i = 0; i < num_commands; i++) {
        if (i < num_commands - 1) {
            if (pipe(pipefds[i]) == -1) {
                fprintf(stderr, "%s\n", "pipe failed!!!");
                return 1;
            }
        }
        child_pid = fork();
        if (child_pid < 0) {
            fprintf(stderr, "%s\n", "fork failed!!!");
            return 1;
        }
            // Child process
        else if (child_pid == 0) {

            // re-enable the ^C signal
            signal(SIGINT, SIG_DFL);

            int startIndex = commandsIndex[i][0];
            int endIndex = commandsIndex[i][1];

            if (i > 0) {
                // If not the first command, redirect input from the previous pipe
                dup2(pipefds[i - 1][0], STDIN_FILENO);
                close(pipefds[i - 1][0]);
                close(pipefds[i - 1][1]);
            }

            if (i < num_commands - 1) {
                // If not the last command, redirect output to the current pipe
                dup2(pipefds[i][1], STDOUT_FILENO);
                close(pipefds[i][0]);
                close(pipefds[i][1]);
            }


            int redirectionValue = redirectHelper(tokens, startIndex, endIndex);
            int n_tokens = redirectionValue - startIndex + 1;
            int _status;
            if (redirectionValue >= 0) {
                redirectionCommandHelper(tokens, n_tokens, startIndex, endIndex, redirectionValue);
            } else {
                _status = runCommandsHelper(tokens, n_tokens, startIndex, endIndex);

                if (_status == 0) {
                    exit(EXIT_SUCCESS);
                }
                fprintf(stderr, "%s: %s\n", tokens[startIndex], strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
            // Parent process
        else {
            // close the unused file descriptors
            if (i > 0) {
                close(pipefds[i - 1][0]);
                close(pipefds[i - 1][1]);
            }
            child_pids[i] = child_pid;
            int child_status;
            do {
                waitpid(child_pids[i], &child_status, WUNTRACED);
            } while (!WIFEXITED(child_status) && !WIFSIGNALED(child_status));
            status = WEXITSTATUS(child_status);
        }
    }

    // restore STDIN and STDOUT fds
    dup2(save_std_in, STDIN_FILENO);
    dup2(save_std_out, STDOUT_FILENO);
    // disable the ^C signal
    signal(SIGINT, SIG_IGN);

    // TODO: update return value
    return status;
}

int **create2DArray(int rows, int cols) {
    int **array2D = (int **) malloc(rows * sizeof(int *));
    for (int i = 0; i < rows; i++) {
        array2D[i] = (int *) malloc(cols * sizeof(int));
    }
    return array2D;
}

int fillCommandIndexes(int **commandIndexes, int num_commands, char *tokens[], int n_tokens) {
    // command 1 starts at index 0
//    int j = 0;
//    commandIndexes[j][0] = 0;
//    for (int i = 0; i < n_tokens; i++) {
//        if (tokens[i] == NULL) {
//            // TODO: handle pipe at index 0
//            commandIndexes[j][1] = i - 1;
//            j++;
//            if (i < n_tokens - 1) {
//                commandIndexes[j][0] = i + 1;
//            }
//        }
//        if (i == n_tokens - 1) {
//            commandIndexes[j][1] = i;
//        }
//    }

    int startIndex = 0;
    int i = 0;
    int endIndex = -1;

    int count = 0;
    while (startIndex < n_tokens && i < n_tokens) {

        if (i == 0 && tokens[i] == NULL) {
            i++;
            startIndex = i;
            continue;
        }
        while (tokens[i] == NULL && tokens[i - 1] == NULL) {
            i++;
            startIndex = i;
        }
        if (i == n_tokens - 1 && tokens[i] == NULL) {
            endIndex = i;
            commandIndexes[count][0] = startIndex;
            commandIndexes[count][1] = endIndex;
            count++;
            i++;
        } else if (tokens[i] == NULL && tokens[i - 1] != NULL) {
            endIndex = i - 1;
            commandIndexes[count][0] = startIndex;
            commandIndexes[count][1] = endIndex;
            count++;
            i++;
            startIndex = i;
        } else if (i == n_tokens - 1 && tokens[i] != NULL){
            endIndex = i;
            commandIndexes[count][0] = startIndex;
            commandIndexes[count][1] = endIndex;
            count++;
            i++;
        } else {
            i++;
        }
    }
    return count;
}

int main(int argc, char **argv) {
    bool interactive = isatty(STDIN_FILENO); /* see: man 3 isatty */
    FILE *fp = stdin;

    if (argc == 2) {
        interactive = false;
        fp = fopen(argv[1], "r");
        if (fp == NULL) {
            fprintf(stderr, "%s: %s\n", argv[1], strerror(errno));
            exit(EXIT_FAILURE); /* see: man 3 exit */
        }
    }
    if (argc > 2) {
        fprintf(stderr, "%s: too many arguments\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char line[1024], linebuf[1024];
    const int max_tokens = 32;
    char *tokens[max_tokens];

    /* loop:
     *   if interactive: print prompt
     *   read line, break if end of file
     *   tokenize it
     *   print it out <-- your logic goes here
     */
    while (true) {
        if (interactive) {
            // disable the ^C signal
            signal(SIGINT, SIG_IGN);
            /* print prompt. flush stdout, since normally the tty driver doesn't
             * do this until it sees '\n'
             */
            printf("$ ");
            fflush(stdout);
        }

        /* see: man 3 fgets (fgets returns NULL on end of file)
         */
        if (!fgets(line, sizeof(line), fp))
            break;

        /* read a line, tokenize it, and print it out
         */
        int n_tokens = parse(line, max_tokens, tokens, linebuf, sizeof(linebuf));

        // keep track of the status for later (step 4 I think)
        int status;

        char qbuf[16];

        sprintf(qbuf, "%d", status);

        int num_pipes = 1;
        for (int i = 0; i < n_tokens; i++) {
            if (strcmp("$?", tokens[i]) == 0) {
                tokens[i] = qbuf;
            }
            if (strcmp("|", tokens[i]) == 0) {
                tokens[i] = NULL;
                num_pipes++;
            }
        }
        int **commandIndexes = create2DArray(num_pipes, 2);
        int num_commands = fillCommandIndexes(commandIndexes, num_pipes, tokens, n_tokens);
        if (num_commands == 1) {
            status = runCommandsHelper(tokens, n_tokens, commandIndexes[0][0], commandIndexes[0][1]);
        } else if (num_commands > 1) {
            // rows = noOfChildProcesses and cols = 2(startIndex, endIndex) for each command
            status = pipeHelper(tokens, num_commands, commandIndexes);
        }
        freeSubstringArray(commandIndexes, num_pipes);
    }

    if (interactive)            /* make things pretty */
        printf("\n");           /* try deleting this and then quit with ^D */
}