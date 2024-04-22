#ifndef SCANNER_H
#define SCANNER_H

#define INITIAL_STRING_SIZE 10
typedef struct ListNode* List;

typedef struct PipeLine {
    List *commands;
    int numCommands;
    char * input;
    char * output;
}PipeLine;

typedef enum TokenType {
    Pipeline,
    Operator,
    Option,
    BuiltIn
} TokenType;

typedef union Token {
    char* identifier;
    PipeLine pipeline;
} Token;


typedef struct ListNode {
    TokenType tt;
    Token t;
    List next;
} ListNode;


char* readInputLine();

List getTokenList(char* s);

bool isEmpty(List l);

void printList(List l);

void freeTokenList(List l);

#endif
