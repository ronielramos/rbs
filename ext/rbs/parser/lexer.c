#include "parser.h"



token rbsparser_nextToken(lexstate *state) {
  token tok = { NullType };

  unsigned int c = rb_enc_codepoint(
    RSTRING_PTR(state->string),
    RSTRING_END(state->string),
    rb_enc_get(state->string)
  );

  switch (c) {
    case '(':
      tok.type = pLPAREN;
      break;
    case ')':
      tok.type = pRPAREN;
      break;
  }

  return tok;
}
