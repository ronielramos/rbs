#include "parser.h"

#define ONE_CHAR_PATTERN(c, t) case c: tok.type = t; tok.start = start; tok.end = state->current; break
#define peek(state) rb_enc_mbc_to_codepoint(RSTRING_PTR(state->string) + state->current.byte_pos, RSTRING_END(state->string), rb_enc_get(state->string))

static token NullToken = { NullType };

void consume_token(lexstate *state, token *token, enum TokenType type, position start) {
  state->last_token = *token;
  token->type = type;
  token->start = start;
  token->end = state->current;
}

unsigned int advance(lexstate *state, token token) {
  unsigned int c = peek(state);
  int len = rb_enc_codelen(c, rb_enc_get(state->string));

  state->last_token = token;

  state->current.char_pos += 1;
  state->current.byte_pos += len;

  if (c == '\n') {
    state->current.line += 1;
    state->current.column = 0;
  } else {
    state->current.column += 1;
  }

  return c;
}

static token lex_colon(lexstate *state, token current_token, position start) {
  unsigned int c = peek(state);

  if (c == ':') {
    advance(state, current_token);
    consume_token(state, &current_token, pCOLON2, start);
  } else {
    consume_token(state, &current_token, pCOLON, start);
  }

  return current_token;
}

static token lex_hyphen(lexstate* state, token current_token, position start) {
  unsigned int c = peek(state);

  if (c == '>') {
    advance(state, current_token);
    consume_token(state, &current_token, pARROW, start);
  } else {
    consume_token(state, &current_token, pMINUS, start);
  }

  return current_token;
}

static token lex_dot(lexstate *state, token current_token, position start, int count) {
  unsigned int c = peek(state);

  if (c == '.' && count == 2) {
    advance(state, current_token);
    consume_token(state, &current_token, pDOT3, start);
  } else if (c == '.' && count == 1) {
    advance(state, current_token);
    return lex_dot(state, current_token, start, 2);
  } else {
    consume_token(state, &current_token, pDOT, start);
  }

  return current_token;
}

static token lex_eq(lexstate *state, token current_token, position start, int count) {
  unsigned int c = peek(state);

  printf("[lex_eq:%d] [peek] `%c` (%d)\n", count, c, c);

  switch (count) {
  case 1:
    switch (c) {
    case '=':
      advance(state, current_token);
      return lex_eq(state, current_token, start, 2);
      break;
    case '>':
      advance(state, current_token);
      consume_token(state, &current_token, pFATARROW, start);
      break;
    default:
      return NullToken;
    }

    break;

  case 2:
    switch (c) {
    case '=':
      advance(state, current_token);
      consume_token(state, &current_token, pEQ3, start);
      break;
    default:
      consume_token(state, &current_token, pEQ2, start);
      break;
    }

    break;
  }

  return current_token;
}

token rbsparser_nextToken(lexstate *state, token current_token) {
  token tok = { NullType };

  position start = { 0 };

  unsigned int c;
  bool skip = true;

  while (skip) {
    start = state->current;
    c = advance(state, current_token);

    switch (c) {
    case ' ':
    case '\t':
    case '\n':
      // nop
      break;
    default:
      skip = false;
      break;
    }
  }

  switch (c) {
    ONE_CHAR_PATTERN('(', pLPAREN);
    ONE_CHAR_PATTERN(')', pRPAREN);
    ONE_CHAR_PATTERN('[', pLBRACKET);
    ONE_CHAR_PATTERN(']', pRBRACKET);
    ONE_CHAR_PATTERN('{', pLBRACE);
    ONE_CHAR_PATTERN('}', pRBRACE);
    ONE_CHAR_PATTERN('^', pHAT);
    ONE_CHAR_PATTERN(',', pCOMMA);
    ONE_CHAR_PATTERN('|', pBAR);
    ONE_CHAR_PATTERN('&', pAMP);
    case ':':
      tok = lex_colon(state, current_token, start);
      break;
    case '-':
      tok = lex_hyphen(state, current_token, start);
      break;
    case '.':
      tok = lex_dot(state, current_token, start, 1);
      break;
    case '=':
      tok = lex_eq(state, current_token, start, 1);
      break;
  }

  return tok;
}
