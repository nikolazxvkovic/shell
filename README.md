# Operating Systems Shell

## Introduction

This contains a custom advanced shell as part of the Operating Sytems course at University of Groningen. It incorporates features such as background process execution,
signal handling, and built-in commands for enhanced user control.

## Instructions

In order to use the shell clone the repository and run the following commands

```c
make clean shell
./shell
```

Multiple Built-Ins can be used like 'cd', 'ls', 'status', 'exit' and 'kill', 'jobs' for background process operations

## Bonus

This contains bonus code for Assignment 1 of the Operating Systems course. We implemented the prompt before each input line and the `cd` command in the shell.

## Testing

In order to test the parts of the bonus that we completed you will have to change the `#define EXT_PROMPT 0` to be `#define EXT_PROMPT 1`. Then run a `make` in the terminal and input `./shell`

## Prompt

The prompt before each input line can be done by getting the current working directory path. Which is the directory in which the user is currently located. In order to implement this functionality, we can use the `getcwd()` function from the `unistd.h` library. Here is an how we did that:

```c
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s> ", cwd);
    }
    else {
        perror("getcwd() error");
    }
```

## CD

The change directory command is a built-in command that we implemented in order to be able to change directories. Here is how we did that:

```c
    //checks if the command is cd
    if (strcmp(tokenList->t, "cd") == 0) {
        tokenList = tokenList->next;
        if (tokenList != NULL) {
          int directory = chdir(tokenList->t);
          if (directory == -1) {
            printf("Error: directory not found!\n");
          }
        }
        else {
          printf("Error: no directory specified!\n");
        }
        tokenList = tokenList->next;
        isFailed = 0;
    }
```
