#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <assert.h> // Include the correct signal header

#include "pipeLine.h"
#include "scanner.h"
#include "shell.h"

#define EXT_PROMPT 0

pid_t* pids;
int numOfJobs;
int status;
int isBackground;


/**
 * @brief Skips the operations in the token list
*/
void skipOperations(List* tokenList, int isFailed) {
  if (*tokenList != NULL && (*tokenList)->tt == Operator && strcmp((*tokenList)->t.identifier, "&") == 0) {
    return;
  }
  if (!isFailed) {
    while (*tokenList != NULL && (*tokenList)->tt == Operator && strcmp((*tokenList)->t.identifier, "||") == 0) {
      do {
        (*tokenList) = (*tokenList)->next;
      } while (*tokenList != NULL && ((*tokenList)->tt == Pipeline || (*tokenList)->tt == BuiltIn));
    }
  }
  else {
    while (*tokenList != NULL && (*tokenList)->tt == Operator && strcmp((*tokenList)->t.identifier, "&&") == 0) {
      do {
        *tokenList = (*tokenList)->next;
      } while (*tokenList != NULL && ((*tokenList)->tt == Pipeline || (*tokenList)->tt == BuiltIn));
    }
  }
}

/**
 * Handles the built-in functionalities
 * @param tokenList
 * @param status
 * @param isFailed
 * @param inputLine
 * @param copy
 * @param pids
 * @param numOfJobs
 */
void handleBuiltIns(List* tokenList, int* status, int* isFailed, char* inputLine, List copy, pid_t* pids, int numOfJobs) {
  if((*tokenList) != NULL && (*tokenList)->tt == BuiltIn
     && (*tokenList)->t.identifier != NULL){
    if (strcmp((*tokenList)->t.identifier, "exit") == 0) {
      if (numOfJobs != 0) {
        int flag = 0;
        for (int i = 0; i < numOfJobs; ++i) {
          if (pids[i] != -1) {
            flag = 1;
            break;
          }
        }
        if (flag) {
          printf("Error: there are still background processes running!\n");
          return;
        }
      }
      freeTokenList(copy);
      free(inputLine);
      free(pids);
      exit(EXIT_SUCCESS);
    } else if (strcmp((*tokenList)->t.identifier, "cd") == 0) {
      (*tokenList) = (*tokenList)->next;
      if ((*tokenList) != NULL && (*tokenList)->t.identifier != NULL) {
        if (chdir((*tokenList)->t.identifier) == 0) {
          *status = 0;
          *isFailed = *status;
          (*tokenList) = (*tokenList)->next;
        } else {
          printf("Error: cd directory not found!\n");
          *status = 2;
          *isFailed = *status;
          (*tokenList) = (*tokenList)->next;

        }
      } else {
        printf("Error: cd requires folder to navigate to!\n");
        *status = 2;
        *isFailed = *status;
      }
    } else if (strcmp((*tokenList)->t.identifier, "status") == 0) {
      printf("The most recent exit code is: %d\n", *status);
      (*tokenList) = (*tokenList)->next;
      *isFailed = 0;
    } else if (strcmp((*tokenList)->t.identifier, "jobs") == 0) {
      if (numOfJobs == 0) {
        printf("No background processes!\n");
        return;
      }
      for (int i = numOfJobs - 1; i >= 0; i--) {
        if (pids[i] != -1) {
          printf("Process running with index %d\n", i + 1);
        } else if (i == numOfJobs - 1) {
          printf("No background processes!\n");
        } else {
          break;
        }
      }
    } else if (strcmp((*tokenList)->t.identifier, "kill") == 0) {
      (*tokenList) = (*tokenList)->next;
      if ((*tokenList) == NULL) {
        printf("Error: command requires an index!\n");
        *status = 2;
        *isFailed = *status;
      } else {
        char *check;
        int process = strtol((*tokenList)->t.identifier, &check, 0);
        if (process == 0 || strcmp(check, "\0") != 0) {
          printf("Error: invalid index provided!\n");
          *status = 2;
          *isFailed = *status;
        } else if (process <= numOfJobs) {
          if (pids[process - 1] != -1) {
            if ((*tokenList)->next != NULL && (*tokenList)->next->tt == Option) {
              (*tokenList) = (*tokenList)->next;
              int signal = strtol((*tokenList)->t.identifier, &check, 0);
              if (signal > 0 && strcmp(check, "\0") == 0) {
                kill(pids[process - 1], signal);
              } else {
                printf("Error: invalid signal provided!\n");
                *status = 2;
                *isFailed = *status;
              }
            } else {
              kill(pids[process - 1], SIGTERM);
            }
          }
        } else {
          printf("Error: this index is not a background process!\n");
          *status = 2;
          *isFailed = *status;
        }
      }
    }
  }
}

/**
 * custom child handler, signals back if a child has terminated
 * puts -1 into pids[i], where i is the number of the job
 * @param sig
 * @param info
 * @param context
 */
void sigChildHandler(int sig, siginfo_t* info, void* context) {
  pid_t pid = info->si_pid;
  for (int i = 0; i < numOfJobs; i++) {
    if (pids[i] == pid) {
      int robin = 0;
      int pidStatus = waitpid(pid, &robin, WUNTRACED | WNOHANG);
      if (pidStatus > 0) {
        if (WIFSIGNALED(robin)) {
          status = 128 + WTERMSIG(robin);
        }
        pids[i] = -1;
      }
    }
  }
}

/**
 * custom interrupt handler, imitates exit behaviour
 * @param sig
 * @param info
 * @param context
 */
void sigIntHandler(int sig, siginfo_t* info, void* context) {
  if (numOfJobs != 0) {
    int flag = 0;
    for (int i = 0; i < numOfJobs; ++i) {
      if (pids[i] != -1) {
        flag = 1;
        break;
      }
    }
    if (flag) {
      printf("Error: there are still background processes running!\n");
      return;
    }
  }
  free(pids);
  exit(EXIT_SUCCESS);
}

/**
 * adds the pid of the new job to the pid array
 * @param pids
 * @param newJob
 * @param numJobs
 * @return
 */
pid_t* addNewJob(pid_t* pids, pid_t newJob, int numJobs) {
  pids = realloc(pids, (numJobs + 1) * sizeof(pid_t));
  assert(pids != NULL);
  pids[numJobs] = newJob;
  return pids;
}

int main(int argc, char* argv[]) {
  char* inputLine;
  List tokenList;
  int isFailed = 0;
  status = 0, isBackground = 0, numOfJobs = 0;
  pids = malloc(0);
  struct sigaction sa_child;
  memset(&sa_child, 0, sizeof(sa_child));
  sa_child.sa_sigaction = sigChildHandler;
  sa_child.sa_flags = SA_SIGINFO | SA_RESTART;
  if (sigaction(SIGCHLD, &sa_child, NULL) == -1) {
    perror("sigaction");
    exit(2);
  }


  struct sigaction sa_int;
  memset(&sa_int, 0, sizeof(sa_int));
  sa_int.sa_sigaction = sigIntHandler;
  sa_int.sa_flags = SA_SIGINFO | SA_RESTART;
  if (sigaction(SIGINT, &sa_int, NULL) == -1) {
    perror("sigaction");
    exit(2);
  }


  setbuf(stdin, NULL);
  setbuf(stdout, NULL);
  while (true) {

    // Part of the bonus, displays current directory
#if EXT_PROMPT
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
      printf("%s> ", cwd);
    }
    else {
      printf("getcwd() error");
    }
#endif
    List copy;
    inputLine = readInputLine();

    if (inputLine == NULL) {
      free(inputLine);
      exit(0);
    }
    tokenList = getTokenList(inputLine);
    copy = tokenList;

    bool parsedSuccessfully = parseInputLine(&copy);
    // printf("parsedSuccessfully: %d\n", parsedSuccessfully);
    if (copy == NULL && parsedSuccessfully) {
      copy = tokenList;
      tokenList = buildPipeline(tokenList);
      freeTokenList(copy);
      copy = tokenList;


      int first = 1;
      do {
        if (!first) {
          tokenList = tokenList->next;
        }
        else {
          first = 0;
        }
        // built-ins
        if (tokenList != NULL && tokenList->tt == BuiltIn) {
          handleBuiltIns(&tokenList, &status, &isFailed, inputLine, copy, pids, numOfJobs);
        }
        else {
          if (tokenList != NULL) {
            int* pipeFDs = calloc((tokenList->t.pipeline.numCommands + 1) * 2, sizeof(int));
            for (int i = 0; i < tokenList->t.pipeline.numCommands + 1; ++i) {
              if (pipe(pipeFDs + 2 * i) < 0) {
                printf("Pipes not initialized\n");
                exit(-1);
              }
            }
            if (tokenList->next != NULL && tokenList->next->tt == Operator && strcmp(tokenList->next->t.identifier, "&") == 0) {
              isBackground = 1;
            }
            pid_t* temp = executePipeline(tokenList->t.pipeline, pipeFDs, &status, isBackground);

            if (temp != NULL) {
              for (int i = 0; i < tokenList->t.pipeline.numCommands; i++) {
                pids = addNewJob(pids, temp[i], numOfJobs);
                numOfJobs++;
              }
            }
            free(temp);
            isBackground = 0;
            free(pipeFDs);
            isFailed = status;
            tokenList = tokenList->next;
          }

        }
        skipOperations(&tokenList, isFailed);

      } while (tokenList != NULL);

      freeTokenList(copy);
    }
    if (!parsedSuccessfully) {
      freeTokenList(tokenList);
    }
    free(inputLine);
  }
  return 0;
}