//
// Created by varun on 10/14/23.
//

#include "fork.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <linux/limits.h>
#include "redirection.h"

int forkHelper(char *tokens[], int n_tokens, int startIndex, int endIndex) {
    int child_pid = fork();
    int status = 0;
    if (child_pid == -1) {
        fprintf(stderr, "%s\n", "fork failed!!!");
        status = 1;
    } else if (child_pid == 0) {
        // re-enable the ^C signal
        signal(SIGINT, SIG_DFL);

        int redirectionValue = redirectHelper(tokens, startIndex, endIndex);
        if (redirectionValue > 0) {
            redirectionCommandHelper(tokens, n_tokens, startIndex, endIndex, redirectionValue);
        } else {
            execvp(tokens[startIndex], tokens + startIndex);
        }

        fprintf(stderr, "%s: %s\n", tokens[startIndex], strerror(errno));
        exit(EXIT_FAILURE);
    } else {
        int child_status;
        do {
            waitpid(child_pid, &child_status, WUNTRACED);
        } while (!WIFEXITED(child_status) && !WIFSIGNALED(child_status));
        // disable the ^C signal
        signal(SIGINT, SIG_IGN);
        status = WEXITSTATUS(child_status);
    }
    return status;
}
