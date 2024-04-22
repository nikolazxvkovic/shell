//
// Created by barnabas on 19-3-24.
//

#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "pipeLine.h"


/**
 * @brief creates a NULL terminated array of strings from a tokenlist
 * @return char* a NULL terminated array that holds all arguments for a command
*/
char** getOptions(List tokenList) {
  int size = 8;
  int length = 1;
  char** args = malloc(size * sizeof(char*));
  while (tokenList != NULL) {
    if (size == length) {
      size *= 2;
      args = realloc(args, size * sizeof(char*));
    }
    args[length - 1] = tokenList->t.identifier;
    length++;
    tokenList = tokenList->next;
  }
  args[length - 1] = NULL;
  return args;
}


/**
 * The function parseBuiltIn parses a builtin.
 * @param lp List pointer to the start of the tokenlist.
 * @return a bool denoting whether the builtin was parsed successfully.
 */
bool isBuiltIn(List lp) {

  //
  //

  // NULL-terminated array makes it easy to expand this array later
  // without changing the code at other places.
  char* builtIns[] = {
      "exit",
      "status",
      "cd",
      "jobs",
      "kill",
      NULL
  };

  for (int i = 0; builtIns[i] != NULL; i++) {
    if (!strcmp(lp->t.identifier, builtIns[i])) {
      return true;
    }
  }

  return false;
}

/**
 * This function retuns a copy of the provided inputlist, up until the first operator node
 * while advancing the inputlist as well
 * @param lp the pointer to the inputlist
 * @return the copy of the (part) input list
 */
List copyList(List* lp, int isBuiltin) {
  int isFirst = 1;
  List copy = malloc(sizeof(ListNode)), returnVal, next = NULL;
  returnVal = copy;
  while ((*lp) != NULL && (*lp)->tt != Operator) {
    if (isFirst) {
      copy->tt = Option;
      if (isBuiltin) {
        copy->tt = BuiltIn;
      }
      copy->t.identifier = malloc(strlen((*lp)->t.identifier) + 1 * sizeof(char));
      strcpy(copy->t.identifier, (*lp)->t.identifier);
      isFirst = 0;
      copy->next = next;
      (*lp) = (*lp)->next;
    }
    else {
      next = malloc(sizeof(ListNode));
      next->tt = Option;
      if (isBuiltin) {
        copy->tt = BuiltIn;
      }
      next->t.identifier = malloc(strlen((*lp)->t.identifier) + 1 * sizeof(char));
      strcpy(next->t.identifier, (*lp)->t.identifier);
      next->next = NULL;
      copy->next = next;
      copy = next;
      (*lp) = (*lp)->next;
    }
  }

  return returnVal;
}

/**
 * This function creates a modified copy of the verified inputlist where each node has one of the
 * following types: BuiltIn, PipeLine or Operator
 * @param ls the input list
 * @return the reprocessed input
 */
PipeLine newPipeline(List ls) {
  PipeLine pipeLine;
  int size = 4;
  pipeLine.numCommands = 0;
  pipeLine.output = NULL;
  pipeLine.input = NULL;
  pipeLine.commands = calloc(size, sizeof(List));
  while (ls != NULL) {
    if (ls->tt == Operator) {
      if (!strcmp(ls->t.identifier, "<")) {
        ls = ls->next;
        pipeLine.input = malloc(strlen(ls->t.identifier) + 1 * sizeof(char));
        strcpy(pipeLine.input, ls->t.identifier);
        ls = ls->next;
      }
      else if (!strcmp(ls->t.identifier, ">")) {
        ls = ls->next;
        pipeLine.output = malloc(strlen(ls->t.identifier) + 1 * sizeof(char));
        strcpy(pipeLine.output, ls->t.identifier);
        ls = ls->next;
      }
      else if (!strcmp(ls->t.identifier, "|")) {
        ls = ls->next;
        pipeLine.commands[pipeLine.numCommands] = copyList(&ls, 0);
        pipeLine.numCommands++;
        if (pipeLine.numCommands == size) {
          size *= 2;
          pipeLine.commands = realloc(pipeLine.commands, size * sizeof(List));
        }
      }
      else {
        break;
      }
    }
    else {
      pipeLine.commands[pipeLine.numCommands] = copyList(&ls, 0);
      pipeLine.numCommands++;
      if (pipeLine.numCommands == size) {
        size *= 2;
        pipeLine.commands = realloc(pipeLine.commands, size * sizeof(List));
      }
    }
  }
  return pipeLine;
}


/**
 * Builds a tokenList that only holds tokens of type Operator, BuiltIn and Pipeline
 * @param lp
 * @return the new tokenList
 */
List buildPipeline(List lp) {
  List copy = lp, tmp1, tmp2;
  if (lp == NULL) {
    return NULL;
  }
  else if (lp->tt == Option && isBuiltIn(lp)) {
    tmp2 = copyList(&lp, 1);
    List node = tmp2;
    while (node->next != NULL) {
      node = node->next;
    }
    if (lp != NULL) {
      tmp1 = malloc(sizeof(ListNode));
      tmp1->tt = Operator;
      tmp1->t.identifier = malloc(strlen(lp->t.identifier) + 1 * sizeof(char));
      strcpy(tmp1->t.identifier, lp->t.identifier);
      node->next = tmp1;
      node->next->next = buildPipeline(lp->next);
      return tmp2;
    }
    return tmp2;
  }
  List node = calloc(1, sizeof(ListNode));
  assert(node != NULL);
  node->tt = Pipeline;
  node->t.pipeline = newPipeline(copy);

  //advance copy over the things already compressed into a pipeline
  while (copy->next != NULL && (copy->next->tt == Option ||
    !(strcmp(copy->next->t.identifier, "<") && strcmp(copy->next->t.identifier, ">") &&
      strcmp(copy->next->t.identifier, "|")))) {
    copy = copy->next;
  }
  copy = copy->next;
  if (copy != NULL) {
    tmp1 = malloc(sizeof(ListNode));
    tmp1->tt = Operator;
    tmp1->t.identifier = malloc(strlen(copy->t.identifier) + 1 * sizeof(char));
    strcpy(tmp1->t.identifier, copy->t.identifier);
    node->next = tmp1;
    node->next->next = buildPipeline(copy->next);
  }

  return node;
}



/**
 * This function should execute pipeline
 * @param pipeLine
 * @param current
 * @param pipeFDs
 * @param status
 */
pid_t* executePipeline(PipeLine pipeLine, int* pipeFDs, int* status, int isBackground) {

  int* pids = malloc(pipeLine.numCommands * sizeof(int));

  for (int i = 0; i < pipeLine.numCommands; i++) {
    char** arguments = getOptions(pipeLine.commands[i]);
    pids[i] = fork();

    if (pids[i] == 0) {

      if (pipeLine.input != NULL && pipeLine.output != NULL
        && strcmp(pipeLine.input, pipeLine.output) == 0) {
        printf("Error: input and output files cannot be equal!\n");
        exit(EXIT_FAILURE);
      }
      if (i != 0) {
        if (dup2(pipeFDs[(i - 1) * 2], 0) < 0) {
          perror("dup2 error");
          exit(EXIT_FAILURE);
        }
      }
      else {
        if (pipeLine.input != NULL) {
          int in = open(pipeLine.input, O_RDONLY);
          if (dup2(in, 0) < 0) {
            perror("dup2 error");
            close(in);
            exit(EXIT_FAILURE);
          }
          close(in);
        }
      }

      if (i != pipeLine.numCommands - 1) {
        if (dup2(pipeFDs[i * 2 + 1], 1) < 0) {
          perror("dup2 error");
          exit(EXIT_FAILURE);
        }
      }
      else if (pipeLine.output != NULL) {
        int out = open(pipeLine.output, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (out < 0) {
          perror("open error");
          exit(EXIT_FAILURE);
        }
        if (dup2(out, 1) < 0) {
          perror("dup2 error");
          close(out);
          exit(EXIT_FAILURE);
        }
        close(out);
      }
      for (int j = 0; j < pipeLine.numCommands * 2; j++) {
        close(pipeFDs[j]);
      }
      *status = execvp(arguments[0], arguments);
      if (*status == -1) {
        printf("Error: command not found!\n");
        exit(EXIT_COMMAND_NOT_FOUND);
      }
    }
    else if (pids[i] > 0) {
      if (i != 0) {
        close(pipeFDs[(i - 1) * 2]);
      }
      if (i != pipeLine.numCommands - 1) {
        close(pipeFDs[i * 2 + 1]);
      }
      if (*status == -1) {
        perror("waitpid error");
        exit(EXIT_FAILURE);
      }
    }
    else {
      perror("fork error");
      exit(EXIT_FAILURE);
    }

    free(arguments);
  }

  if (isBackground) {
    return pids;
  }
  for (int i = 0; i < pipeLine.numCommands; i++) {
    int childStatus;

    waitpid(pids[i], &childStatus, 0);

    if (WIFSIGNALED(childStatus)) {
      *status = 128 + WTERMSIG(childStatus);
    }
    else {
      *status = WEXITSTATUS(childStatus);
    }
  }
  free(pids);
  return NULL;
}
