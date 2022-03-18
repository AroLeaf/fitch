#include <stdlib.h>
#include <stdio.h>
#include <argp.h>
#include <string.h>
#include <unistd.h>

#include "lexer.h"
#include "parser.h"

struct arguments {
  char *args[1]; // input file
  int json;  // --json flag
};

static struct argp_option options[] = {
  { "json", 'j', 0, 0, "Output json" },
  {0}
};

static error_t parse_opt (int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;

  switch (key) {
    case 'j':
      arguments->json = 1;
      break;
    case ARGP_KEY_ARG:
      if (state->arg_num >= 2) {
        argp_usage(state);
      }
      arguments->args[state->arg_num] = arg;
      break;
    case ARGP_KEY_END:
      if(!state->arg_num && isatty(fileno(stdin))) {
        argp_usage(state);
      }
      break;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static char args_doc[] = "input";

static char doc[] = "A program to parse and validate fitch proofs written in a custom, easy-to-type format.";

static struct argp argp = {options, parse_opt, args_doc, doc};

int main(int argc, char *argv[]) {
  struct arguments arguments;
  
  arguments.args[0] = "";
  arguments.json = 0;

  argp_parse (&argp, argc, argv, 0, 0, &arguments);


  FILE *instream;
  
  if (isatty(fileno(stdin))) {
    instream = fopen(arguments.args[0], "r");
  } else {
    instream = stdin;
  }

  if (!instream) {
    fprintf(stderr, "Error! file `%s` doesn't exist.\n", arguments.args[0]);
    exit(-1);
  }

  TokenList tokens = lexer(instream);
  Node tree = parser(tokens);

  printNodeToJSON(tree);
  putchar('\n');
  return 0;
}