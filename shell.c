#include <stdbool.h>
#include <string.h>

#include <malloc.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "scanner.h"

/**
 * The function acceptToken checks whether the current token matches a target identifier,
 * and goes to the next token if this is the case.
 * @param lp List pointer to the start of the tokenlist.
 * @param ident target identifier
 * @return a bool denoting whether the current token matches the target identifier.
 */
bool acceptToken(List* lp, char* ident) {
  if (*lp != NULL && (*lp)->tt != Pipeline && strcmp((*lp)->t.identifier, ident) == 0) {
    *lp = (*lp)->next;
    return true;
  }
  return false;
}



/**
 * Checks whether the input string \param s is an operator.
 * @param s input string.
 * @return a bool denoting whether the current string is an operator.
 */
bool isOperator(char* s) {
  // NULL-terminated array makes it easy to expand this array later
  // without changing the code at other places.
  char* operators[] = {
          "&",
          "&&",
          "||",
          ";",
          "<",
          ">",
          "|",
          NULL
  };

  for (int i = 0; operators[i] != NULL; i++) {
    if (strcmp(s, operators[i]) == 0) return true;
  }
  return false;
}

/**
 * The function parseExecutable parses an executable.
 * @param lp List pointer to the start of the tokenlist.
 * @return a bool denoting whether the executable was parsed successfully.
 */
bool parseExecutable(List* lp) {

  // TODO: Determine whether to accept parsing an executable here.
  // 
  // It is not recommended to check for existence of the executable already
  // here, since then it'll be hard to continue parsing the rest of the input
  // line (command execution should continue after a "not found" command),
  // it'll also be harder to print the correct error message.
  // 
  // Instead, we recommend to just do a syntactical check here, which makes
  // more sense, and defer the binary existence check to the runtime part
  // you'll write later.
  return (*lp)->tt == Option;
}

/**
 * The function parseOptions parses options.
 * @param lp List pointer to the start of the tokenlist.
 * @return a bool denoting whether the options were parsed successfully.
 */
bool parseOptions(List* lp) {
  //TODO: store each (*lp)->t as an option, if any exist
  while (*lp != NULL && (*lp)->tt == Option) {
    (*lp) = (*lp)->next;
  }
  return true;
}

/**
 * The function parseRedirections parses a command according to the grammar:
 *
 * <command>        ::= <executable> <options>
 *
 * @param lp List pointer to the start of the tokenlist.
 * @return a bool denoting whether the command was parsed successfully.
 */
bool parseCommand(List* lp) {
  return parseExecutable(lp) && parseOptions(lp);
}

/**
 * The function parsePipeline parses a pipeline according to the grammar:
 *
 * <pipeline>           ::= <command> "|" <pipeline>
 *                       | <command>
 *
 * @param lp List pointer to the start of the tokenlist.
 * @return a bool denoting whether the pipeline was parsed successfully.
 */
bool parsePipeline(List* lp) {
  if (!parseCommand(lp)) {
    return false;
  }

  if (acceptToken(lp, "|")) {
    return parsePipeline(lp);
  }

  return true;
}

/**
 * The function parseBuiltIn parses a builtin.
 * @param lp List pointer to the start of the tokenlist.
 * @return a bool denoting whether the builtin was parsed successfully.
 */
bool parseBuiltIn(List* lp) {

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
    if (acceptToken(lp, builtIns[i])) {
      return true;
    }
  }

  return false;
}

/**
 * The function parseFileName parses a filename.
 * @param lp List pointer to the start of the tokenlist.
 * @return a bool denoting whether the filename was parsed successfully.
 */
bool parseFileName(List* lp) {
  //TODO: Process the file name appropriately

  if ((*lp != NULL) && (*lp)->tt == Option && !parseBuiltIn(lp)) {
    (*lp) = (*lp)->next;
    return true;
  }
  return false;
}

/**
 * The function parseRedirections parses redirections according to the grammar:
 *
 * <redirections>       ::= <pipeline> <redirections>
 *                       |  <builtin> <options>
 *
 * @param lp List pointer to the start of the tokenlist.
 * @return a bool denoting whether the redirections were parsed successfully.
 */
bool parseRedirections(List* lp) {
  if (isEmpty(*lp)) {
    return true;
  }

  if (acceptToken(lp, "<")) {
    if (!parseFileName(lp)) {
      printf("Error: invalid syntax!\n");
      return false;
    }

    if (acceptToken(lp, ">")) {
      int here = parseFileName(lp);
      if (!here) {
        printf("Error: invalid syntax!\n");
        return false;
      }
      return here;
    }
    else return true;
  }
  else if (acceptToken(lp, ">")) {
    if (!parseFileName(lp)) {
      printf("Error: invalid syntax!\n");
      return false;
    }
    if (acceptToken(lp, "<")) return parseFileName(lp);
    else return true;
  }

  return true;
}

/**
 * The function parseChain parses a chain according to the grammar:
 *
 * <chain>              ::= <pipeline> <redirections>
 *                       |  <builtin> <options>
 *
 * @param lp List pointer to the start of the tokenlist.
 * @return a bool denoting whether the chain was parsed successfully.
 */
bool parseChain(List* lp) {
  if (parseBuiltIn(lp)) {
    return parseOptions(lp);
  }
  if (parsePipeline(lp)) {
    return parseRedirections(lp);
  }
  return false;
}

/**
 * The function parseInputLine parses an inputline according to the grammar:
 *
 * <inputline>      ::= <chain> & <inputline>
 *                   | <chain> && <inputline>
 *                   | <chain> || <inputline>
 *                   | <chain> ; <inputline>
 *                   | <chain>
 *                   | <empty>
 *
 * @param lp List pointer to the start of the tokenlist.
 * @return a bool denoting whether the inputline was parsed successfully.
 */
bool parseInputLine(List* lp) {
  if (isEmpty(*lp)) {
    return true;
  }

  if (!parseChain(lp)) {
    return false;
  }

  if (acceptToken(lp, "&") || acceptToken(lp, "&&")) {
    return parseInputLine(lp);
  }
  else if (acceptToken(lp, "||")) {
    return parseInputLine(lp);
  }
  else if (acceptToken(lp, ";")) {
    return parseInputLine(lp);
  }

  return true;
}


