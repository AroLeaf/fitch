#ifndef LEXER_H
#define LEXER_H

typedef enum { tok_none, tok_constant, tok_predicate, tok_identifier, tok_number, tok_separator, tok_identity, tok_conjunction, tok_disjunction, tok_negation, tok_elimination, tok_introduction, tok_reiteration, tok_conditional, tok_biconditional, tok_contradiction, tok_forall, tok_exists, tok_proof, tok_lparen, tok_rparen, tok_indent, tok_undent, tok_break } Symbol;

typedef struct Token {
  Symbol type;
  char *value;
  int row, col;
} Token;

typedef struct TokenList {
  Token *tokens;
  int count, current;
} TokenList;

TokenList lexer(FILE *instream);

#endif