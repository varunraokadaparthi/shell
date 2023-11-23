//
// Created by varun on 10/14/23.
//

#include "redirection.h"

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

#include "parser.h"
#include "runcommands.h"

void freeStringArray(char **array, int rows) {
    for (int i = 0; i < rows; i++) {
        free(array[i]);
    }
    free(array);
}

int redirectHelper(char *tokens[], int startIndex, int endIndex) {
    for (int i = startIndex; i <= endIndex; i++) {
        if (strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], "<") == 0) {
            return i;
        }
    }
    // no redirect found
    return -1;
}

int redirectionCommandHelper(char *tokens[], int n_tokens, int startIndex, int endIndex, int redirectionValue) {
    int _status;

    if (strcmp(tokens[redirectionValue], ">") == 0) {

        // replace "<" or ">" with NULL so that execpvp() will consider only until "<" or ">"
        tokens[redirectionValue] = NULL;
        // execute first part
        // save that output to the second part file
        int file_fd = open(tokens[redirectionValue + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (file_fd == -1) {
            fprintf(stderr, "failed to create file descriptor for %s\n!!!", tokens[startIndex]);
            exit(EXIT_FAILURE);
        }

        dup2(file_fd, STDOUT_FILENO);
        close(file_fd);
        _status = runCommandsHelper(tokens, n_tokens, startIndex, redirectionValue - 1);
        if (_status == 0) {
            exit(EXIT_SUCCESS);
        }
        fprintf(stderr, "%s: %s\n", tokens[startIndex], strerror(errno));
        exit(EXIT_FAILURE);

    } else {

        FILE *fp = fopen(tokens[redirectionValue + 1], "r");
        if (fp == NULL) {
            fprintf(stderr, "%s: %s\n", tokens[redirectionValue + 1], strerror(errno));
            exit(EXIT_FAILURE); /* see: man 3 exit */
        }

        char line[1024], linebuf[1024];
        const int max_tokens = 32;
        char *in_tokens[max_tokens];

        if (!fgets(line, sizeof(line), fp)) {
            fprintf(stderr, "%s: %s\n", tokens[redirectionValue + 1], strerror(errno));
            exit(EXIT_FAILURE);
        }

        /* read a line, tokenize it, and print it out
         */
        int n_in_tokens = parse(line, max_tokens, in_tokens, linebuf, sizeof(linebuf));

        int total_tokens = redirectionValue - startIndex + n_in_tokens + 1;
//        char *new_tokens[] = (char *)malloc(total_tokens * sizeof(char *));
        char *new_tokens[total_tokens];
        int j = 0;
        for (int k = startIndex; k < redirectionValue; k++) {
            new_tokens[j] = tokens[k];
            j++;
        }
        for (int k = 0; k < n_in_tokens; k++) {
            new_tokens[j] = in_tokens[k];
            j++;
        }
        new_tokens[j] = NULL;
        tokens[redirectionValue] = NULL;
        _status = runCommandsHelper(new_tokens, total_tokens - 1, 0, total_tokens - 2);
        int file_fd = open(tokens[redirectionValue + 1], O_RDONLY);
        dup2(file_fd, STDIN_FILENO);

        if (_status == 0) {
            exit(EXIT_SUCCESS);
        }
        fprintf(stderr, "%s: %s\n", tokens[startIndex], strerror(errno));
        exit(EXIT_FAILURE);
    }
}
