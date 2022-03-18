#include "lexer.h"
#ifndef PARSER_H
#define PARSER_H

typedef enum { expr_empty, expr_fitch, expr_declaration, expr_identifier, expr_predicate, expr_proof, expr_premises, expr_conclusions, expr_literal, expr_reference, expr_number, expr_introduction, expr_elimination, expr_reiteration, expr_biconditional, expr_conditional, expr_forall, expr_exists, expr_conjunction, expr_disjunction, expr_negation, expr_identity } Expression;

typedef struct Node {
  Expression type;
  char *value;
  struct Node *children;
  int childCount;
} Node;

Node parser(TokenList tokens);
void printNodeToJSON(Node node); 

#endif