//
// Created by varun on 10/14/23.
//

#include "runcommands.h"

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
#include "fork.h"

int pwdHelper() {
    // Allocate PATH_MAX of memory. PATH_MAX is defined in <linux/limits.h>
    char *buffer = malloc(PATH_MAX);
    getcwd(buffer, PATH_MAX);
    printf("%s\n", buffer);
    // Free the buffer, due to memory leak error.
    free(buffer);
    return 0;
}

int cdHelper(char *tokens[], int n_tokens, int startIndex) {
    if (n_tokens > 2) {
        fprintf(stderr, "%s", "cd: wrong number of arguments\n");
        return 1;
    }
    char *destination;
    // if no parameter, set destination as home directory
    if (n_tokens == 1) {
        // get the home directory from environment variables
        destination = getenv("HOME");
    }
        // if 1 parameter, set destination as given parameter
    else if (n_tokens == 2) {
        destination = tokens[startIndex + 1];
    }
    // call chdir system  call with destination as parameter
    int success = chdir(destination);
    if (success != 0) {
        fprintf(stderr, "%s\n", strerror(errno));
        return 1;
    } else {
        return 0;
    }
}

int exitHelper(char *tokens[], int n_tokens, int startIndex) {
    if (n_tokens == 2) {
        // one parameter with exit is fine
        // re-enable the ^C signal
        signal(SIGINT, SIG_DFL);
        // the new string array created can be freed at exit
        // It not leads to memory leaks
        int status = atoi(tokens[startIndex + 1]);
        exit(status);
    } else if (n_tokens == 1) {
        // no parameters with exit is fine
        // re-enable the ^C signal
        // the new string array created can be freed at exit
        // It not leads to memory leaks
        signal(SIGINT, SIG_DFL);
        exit(0);
    }
    // but more than 0 or 1 additional parameters is bad.
    fprintf(stderr, "%s", "exit: too many arguments\n");
    return 1;
}

int runCommandsHelper(char *tokens[], int n_tokens, int startIndex, int endIndex) {

    // new array size
    int new_n_tokens = endIndex - startIndex + 1;
    // create new sub array
    // char ** new_tokens = createSubstringArray(tokens, startIndex, endIndex, &new_n_tokens);

    // if the command is cd...
    if (strcmp("cd", tokens[startIndex]) == 0) {
        return cdHelper(tokens, new_n_tokens, startIndex);
    }
        // if the command is pwd...
    else if (strcmp("pwd", tokens[startIndex]) == 0) {
        return pwdHelper();
    }
        // if the command is exit
    else if (strcmp("exit", tokens[startIndex]) == 0) {
        return exitHelper(tokens, new_n_tokens, startIndex);
    }
        // external command try to run it using fork
    else {
        return forkHelper(tokens, new_n_tokens, startIndex, endIndex);
    }
}
