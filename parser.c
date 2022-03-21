#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"

void printNodeToJSON(Node node);

Token current(TokenList *tokens) {
  if (tokens->current >= tokens->count) return (Token){tok_none};
  return tokens->tokens[tokens->current];
}
Token previous(TokenList *tokens) {
  return tokens->tokens[tokens->current - 1];
}

Token next(TokenList *tokens) {
  tokens->current++;
  return current(tokens);
}

int assert(TokenList *tokens, Symbol expected) {
  return current(tokens).type == expected;
}

int accept(TokenList *tokens, Symbol expected) {
  if (assert(tokens, expected)) {
    next(tokens);
    return 1;
  }
  return 0;
}

int expect(TokenList *tokens, Symbol expected) {
  if (accept(tokens, expected))
    return 1;
  fprintf(stderr, "Wrong token type %d at token %s, token %d/%d, expected %d, previous %s\n", current(tokens).type, current(tokens).value, tokens->current, tokens->count, expected, previous(tokens).value);
  exit(-1);
}

void appendChild(Node *parent, Node child) {
  parent->childCount++;
  parent->children = realloc(parent->children, parent->childCount * sizeof(Node));
  parent->children[parent->childCount - 1] = child;
}


Node newNode(Expression expr, Token token) {
  return (Node){ expr, 0, malloc(0), 0, token.row, token.col, 0 };
}
Node tokenToNode(Expression expr, Token token) {
  Node this = newNode(expr, token);
  this.value = malloc(strlen(token.value) + 1);
  strcpy(this.value, token.value);
  return this;
}

Node declaration(TokenList *tokens) {
  Node this = tokenToNode(expr_declaration, current(tokens));
  Expression expr;
  if (accept(tokens, tok_constant)) {
    expr = expr_constant;
  } else if (accept(tokens, tok_function)) {
    expr = expr_function;
  } else {
    expect(tokens, tok_predicate);
    expr = expr_predicate;
  }
  expect(tokens, tok_identifier);
  appendChild(&this, tokenToNode(expr, previous(tokens)));
  while (accept(tokens, tok_separator)) {
    expect(tokens, tok_identifier);
    appendChild(&this, tokenToNode(expr, previous(tokens)));
  }
  return this;
}

Node expression(TokenList *tokens);

Node factor(TokenList *tokens) {
  expect(tokens, tok_identifier);
  Node this = tokenToNode(expr_identifier, previous(tokens));
  if (accept(tokens, tok_lparen)) {
    this.type = expr_function;
    appendChild(&this, factor(tokens));
    while (accept(tokens, tok_separator)) {
      appendChild(&this, factor(tokens));
    }
    expect(tokens, tok_rparen);
  }
  return this;
}

Node term(TokenList *tokens) {
  Node this;
  if (accept(tokens, tok_negation)) {
    this = tokenToNode(expr_negation, previous(tokens));
    appendChild(&this, term(tokens));
  } else if (accept(tokens, tok_lparen)) {
    this = expression(tokens);
    expect(tokens, tok_lparen);
  } else {
    this = factor(tokens);
    if (accept(tokens, tok_identity)) {
      Node left = this;
      this = tokenToNode(expr_identity, previous(tokens));
      appendChild(&this, left);
      appendChild(&this, factor(tokens));
    } else {
      this.type = expr_predicate;
    }
  }
  return this;
}

Node quantifier(TokenList *tokens) {
  if (accept(tokens, tok_negation)) {
    Node this = tokenToNode(expr_negation, previous(tokens));
    appendChild(&this, quantifier(tokens));
    return this;
  }
  Node this;
  if (accept(tokens, tok_forall)) {
    this = tokenToNode(expr_forall, previous(tokens));
  } else if (accept(tokens, tok_exists)) {
    this = tokenToNode(expr_exists, previous(tokens));
  } else {
    return term(tokens);
  }
  expect(tokens, tok_identifier);
  appendChild(&this, tokenToNode(expr_variable, previous(tokens)));
  appendChild(&this, quantifier(tokens));
  return this;
}

Node conditional(TokenList *tokens) {
  Node this = quantifier(tokens);
  if (accept(tokens, tok_biconditional)) {
    Node left = this;
    this = tokenToNode(expr_biconditional, previous(tokens));
    appendChild(&this, left);
    appendChild(&this, conditional(tokens));
  } else if (accept(tokens, tok_conditional)) {
    Node left = this;
    this = tokenToNode(expr_conditional, previous(tokens));
    appendChild(&this, left);
    appendChild(&this, conditional(tokens));
  }
  return this;
}

Node expression(TokenList *tokens) {
  Node this = conditional(tokens);
  if (accept(tokens, tok_conjunction)) {
    Node left = this;
    this = tokenToNode(expr_conjunction, previous(tokens));
    appendChild(&this, left);
    appendChild(&this, expression(tokens));
  } else if (accept(tokens, tok_disjunction)) {
    Node left = this;
    this = tokenToNode(expr_disjunction, previous(tokens));
    appendChild(&this, left);
    appendChild(&this, expression(tokens));
  }
  return this;
}

Node premise(TokenList *tokens) {
  if (assert(tokens, tok_break)) {
    return tokenToNode(expr_empty, current(tokens));
  }
  Node this = expression(tokens);
  return this;
}

Node concludable(TokenList *tokens) {
  if (
    accept(tokens, tok_forall) ||
    accept(tokens, tok_exists) ||
    accept(tokens, tok_conditional) ||
    accept(tokens, tok_biconditional) ||
    accept(tokens, tok_conjunction) ||
    accept(tokens, tok_disjunction) ||
    accept(tokens, tok_negation) ||
    accept(tokens, tok_identity) ||
    expect(tokens, tok_contradiction)
  ) return tokenToNode(expr_literal, previous(tokens));
}

Node reference(TokenList *tokens) {
  Node this = newNode(expr_reference, current(tokens));
  expect(tokens, tok_number);
  appendChild(&this, tokenToNode(expr_number, previous(tokens)));
  if (accept(tokens, tok_colon)) {
    expect(tokens, tok_number);
    appendChild(&this, tokenToNode(expr_number, previous(tokens)));
  }
  if (accept(tokens, tok_elimination)) {
    Node left = this;
    this = tokenToNode(expr_reference_range, previous(tokens));
    appendChild(&this, left);
    appendChild(&this, reference(tokens));
  }
  return this;
}

Node referenceList(TokenList *tokens) {
  Node this = tokenToNode(expr_reference_list, current(tokens));
  expect(tokens, tok_lparen);
  appendChild(&this, reference(tokens));
  while (accept(tokens, tok_separator)) {
    appendChild(&this, reference(tokens));
  }
  expect(tokens, tok_rparen);
  return this;
}

Node conclusion(TokenList *tokens) {
  if (assert(tokens, tok_break)) {
    return tokenToNode(expr_empty, current(tokens));
  }
  Node this;
  if (accept(tokens, tok_introduction)) {
    this = tokenToNode(expr_introduction, previous(tokens));
    appendChild(&this, concludable(tokens));
  } else if (accept(tokens, tok_elimination)) {
    this = tokenToNode(expr_elimination, previous(tokens));
    appendChild(&this, concludable(tokens));
  } else {
    expect(tokens, tok_reiteration);
    this = tokenToNode(expr_reiteration, previous(tokens));
  }
  appendChild(&this, referenceList(tokens));
  appendChild(&this, premise(tokens));
  return this;
}

Node proof(TokenList *tokens) {
  Node this = newNode(expr_proof, current(tokens));
  if (accept(tokens, tok_lsquare)) {
    Node var = tokenToNode(expr_declaration, previous(tokens));
    expect(tokens, tok_identifier);
    appendChild(&var, tokenToNode(expr_variable, previous(tokens)));
    expect(tokens, tok_rsquare);
    appendChild(&this, var);
  }
  
  Node premises = newNode(expr_premises, current(tokens));
  while (!assert(tokens, tok_proof)) {
    appendChild(&premises, premise(tokens));
    expect(tokens, tok_break);
  }
  expect(tokens, tok_proof);
  expect(tokens, tok_break);
  
  Node conclusions = newNode(expr_conclusions, current(tokens));
  while (!(assert(tokens, tok_undent) || assert(tokens, tok_none))) {
    if (accept(tokens, tok_indent)) {
      appendChild(&conclusions, proof(tokens));
      expect(tokens, tok_undent);
    } else {
      appendChild(&conclusions, conclusion(tokens));
      accept(tokens, tok_break) || expect(tokens, tok_none);
    }
  }
  appendChild(&this, premises);
  appendChild(&this, conclusions);
  return this;
}

Node fitch(TokenList *tokens) {
  Node this = newNode(expr_fitch, current(tokens));
  while (assert(tokens, tok_predicate) || assert(tokens, tok_constant)) {
    appendChild(&this, declaration(tokens));
    expect(tokens, tok_break);
  }
  appendChild(&this, proof(tokens));
  return this;
}

Node parser(TokenList tokens) {
  tokens.current = 0;
  Node node = fitch(&tokens);
  for (int i = 0; i < tokens.count; i++) {
    free(tokens.tokens[i].value);
  }
  return node;
}


void printNodeToJSON(Node node) {
  printf("{\"type\":%d,\"value\":", node.type);
  printJSONString(node.value);
  printf(",\"row\":%d,\"col\":%d,\"valid\":%s,\"children\":[", node.row, node.col, node.valid ? "true": "false");
  for (int i = 0; i < node.childCount; i++) {
    printNodeToJSON(node.children[i]);
    if (i + 1 < node.childCount) putchar(',');
  }
  printf("]}");
}

void printJSONString(char *string) {
  if (!string) return fputs("null", stdout);
  fputs("\"", stdout);
  int len = strlen(string);
  for (int i = 0; i < len; i++) {
    switch (string[i]) {
      case '\b':
        fputs("\\b", stdout);
        break;

      case '\f':
        fputs("\\f", stdout);
        break;

      case '\n':
        fputs("\\n", stdout);
        break;

      case '\r':
        fputs("\\r", stdout);
        break;

      case '\t':
        fputs("\\t", stdout);
        break;

      case '"':
        fputs("\\\"", stdout);
        break;

      case '\\':
        fputs("\\\\", stdout);
        break;
      
      default:
        putchar(string[i]);
    }
  }
  fputs("\"", stdout);
}