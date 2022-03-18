#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "lexer.h"

typedef struct Info {
  int row, col, SOL;
} Info;

typedef struct Indent {
  int depth, indents[256];
} Indent;

char nextChar(FILE *stream, Info *info) {
  char c = fgetc(stream);
  if (c == '\n') {
    info->col = 1;
    info->row++;
    info->SOL = 1;
  } else {
    info->col++;
    info->SOL = c == ' ' ? info->SOL : 0;
  }
  return c;
}

char peek(FILE *stream) {
  char c = fgetc(stream);
  ungetc(c, stream);
  return c;
}

char expectChar(FILE *stream, Info *info, char expected) {
  if (peek(stream) != expected) return 0;
  return nextChar(stream, info);
}

char *appendChar(char *s, char c) {
  int len = strlen(s);
  char *out = realloc(s, len + 2);
  out[len] = c;
  out[len+1] = '\0';
  return out;
}

char *setString(char *ptr, const char *str) {
  return strcpy(realloc(ptr, strlen(str) + 1), str);
}

int matchIndent(Indent *indent, int len, Symbol *type) {
  if (indent->indents[indent->depth] == len) return -1;
  if (indent->indents[indent->depth] < len) {
    indent->depth++;
    indent->indents[indent->depth] = len;
    *type = tok_indent;
    return 1;
  }
  int count = 0;
  *type = tok_undent;
  while (indent->indents[indent->depth] > len) {
    indent->depth--;
    count++;
  }
  return indent->indents[indent->depth] == len ? count : 0;
}

TokenList lexer(FILE *stream) {
  Info info = { 1, 1, 1 };
  char c, *value;
  Indent indentation = {0, {0}};
  Symbol type;
  TokenList tokens = {malloc(0), 0, 0};

  c = nextChar(stream, &info);
  while (c != EOF) {
    value = strcpy(malloc(1), "");
    int row = info.row, col = info.col, sol = info.SOL;
    
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
      do {
          value = appendChar(value, c);
          c = nextChar(stream, &info);
        } while ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
        type = !strcmp(value, "pred")
          ? tok_predicate
          : !strcmp(value, "const")
            ? tok_constant
            : tok_identifier;
    } else if (c >= '1' && c <= '9') {
      type = tok_number;
        do {
          value = appendChar(value, c);
          c = nextChar(stream, &info);
        } while (c >= '0' && c <= '9');
    } else {
      switch (c) {
        case '<':
          type = tok_biconditional;
          if (!expectChar(stream, &info, '-') || !expectChar(stream, &info, '>')) {
            fprintf(stderr, "Invalid character at %d:%d", info.row, info.col);
            exit(-1);
          };
          c = nextChar(stream, &info);
          break;

        case '-':
          c = nextChar(stream, &info);
          if (c == '>') {
            type = tok_conditional;
            value = setString(value, "->");
            c = nextChar(stream, &info);
          } else if (c == '-' && expectChar(stream, &info, '-')) {
            type = tok_proof;
            value = setString(value, "---");
            c = nextChar(stream, &info);
          } else {
            type = tok_elimination;
            value = setString(value, "-");
          }
          break;

        case '^':
          type = tok_reiteration;
          value = setString(value, "^");
          c = nextChar(stream, &info);
          break;

        case '=':
          type = tok_identity;
          value = setString(value, "=");
          c = nextChar(stream, &info);
          break;
        
        case '&':
          type = tok_conjunction;
          value = setString(value, "&");
          c = nextChar(stream, &info);
          break;
        
        case '|':
          type = tok_disjunction;
          value = setString(value, "|");
          c = nextChar(stream, &info);
          break;

        case '!':
          type = tok_negation;
          value = setString(value, "!");
          c = nextChar(stream, &info);
          break;

        case '@':
          type = tok_forall;
          value = setString(value, "@");
          c = nextChar(stream, &info);
          break;

        case '%':
          type = tok_exists;
          value = setString(value, "%");
          c = nextChar(stream, &info);
          break;

        case '$':
          type = tok_contradiction;
          value = setString(value, "$");
          c = nextChar(stream, &info);
          break;

        case ',':
          type = tok_separator;
          value = setString(value, ",");
          c = nextChar(stream, &info);
          break;

        case '(':
          type = tok_lparen;
          value = setString(value, "(");
          c = nextChar(stream, &info);
          break;

        case ')':
          type = tok_rparen;
          value = setString(value, ")");
          c = nextChar(stream, &info);
          break;

        case '\n':
          if (peek(stream) != ' ') {
            int pushed = matchIndent(&indentation, 0, &type);
            if (!pushed) {
              fprintf(stderr, "Indent mismatch at %d:%d\n", row, col);
              exit(-1);
            }
            for (int i = 0; i < pushed; i++) {
              tokens.count++;
              tokens.tokens = realloc(tokens.tokens, tokens.count * sizeof(Token));
              Token token = { type, 0, row, col };
              token.value = strcpy(malloc(strlen(value) + 1), value);
              tokens.tokens[tokens.count-1] = token;
            }
          }
        case ';':
          type = tok_break;
          value = setString(value, " ");
          value[0] = c;
          c = nextChar(stream, &info);
          break;

        case ' ':
          do {
            value = appendChar(value, c);
            c = nextChar(stream, &info);
          } while (c == ' ');
          if (sol) {
            int pushed = matchIndent(&indentation, strlen(value), &type);
            if (!pushed) {
              fprintf(stderr, "Indent mismatch at %d:%d\n", row, col);
              exit(-1);
            }
            for (int i = 0; i < pushed; i++) {
              tokens.count++;
              tokens.tokens = realloc(tokens.tokens, tokens.count * sizeof(Token));
              Token token = { type, 0, row, col };
              token.value = strcpy(malloc(strlen(value) + 1), value);
              tokens.tokens[tokens.count-1] = token;
            }
          }
          free(value);
          continue;

        default:
          fprintf(stderr, "`%c` is not a valid character at %d:%d\n", c, row, col);
          exit(-1);
      }
    }

    tokens.count++;
    tokens.tokens = realloc(tokens.tokens, tokens.count * sizeof(Token));
    Token token = { type, 0, row, col };
    token.value = strcpy(malloc(strlen(value) + 1), value);
    tokens.tokens[tokens.count-1] = token;
    free(value);
  }

  return tokens;
}