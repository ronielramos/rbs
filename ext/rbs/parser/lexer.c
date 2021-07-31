#include "parser.h"

#define ONE_CHAR_PATTERN(c, t) case c: tok = next_token(state, t, start); break
#define peek(state) rb_enc_mbc_to_codepoint(RSTRING_PTR(state->string) + state->current.byte_pos, RSTRING_END(state->string), rb_enc_get(state->string))

token NullToken = { NullType };
position NullPosition = { 0 };

int token_chars(token tok) {
  return tok.end.char_pos - tok.start.char_pos;
}

int token_bytes(token tok) {
  return tok.end.byte_pos - tok.start.byte_pos;
}

token next_token(lexstate *state, enum TokenType type, position start) {
  token t;

  t.type = type;
  t.start = start;
  t.end = state->current;

  return t;
}

void skipbyte(lexstate *state, unsigned int c, int len) {
  state->current.char_pos += 1;
  state->current.byte_pos += len;

  if (c == '\n') {
    state->current.line += 1;
    state->current.column = 0;
  } else {
    state->current.column += 1;
  }
}

void skip(lexstate *state, unsigned int c) {
  int len = rb_enc_codelen(c, rb_enc_get(state->string));
  skipbyte(state, c, len);
}

static token lex_colon(lexstate *state, position start) {
  unsigned int c = peek(state);

  if (c == ':') {
    skipbyte(state, c, 1);
    return next_token(state, pCOLON2, start);
  } else {
    return next_token(state, pCOLON, start);
  }
}

static token lex_hyphen(lexstate* state, position start) {
  unsigned int c = peek(state);

  if (c == '>') {
    skipbyte(state, c, 1);
    return next_token(state, pARROW, start);
  } else {
    return next_token(state, pMINUS, start);
  }
}

static token lex_dot(lexstate *state, position start, int count) {
  unsigned int c = peek(state);

  if (c == '.' && count == 2) {
    skipbyte(state, c, 1);
    return next_token(state, pDOT3, start);
  } else if (c == '.' && count == 1) {
    skipbyte(state, c, 1);
    return lex_dot(state, start, 2);
  } else {
    return next_token(state, pDOT, start);
  }
}

static token lex_eq(lexstate *state, position start, int count) {
  unsigned int c = peek(state);

  switch (count) {
  case 1:
    switch (c) {
    case '=':
      skipbyte(state, c, 1);
      return lex_eq(state, start, 2);
      break;
    case '>':
      skipbyte(state, c, 1);
      return next_token(state, pFATARROW, start);
    default:
      return NullToken;
    }

    break;

  case 2:
    switch (c) {
    case '=':
      skipbyte(state, c, 1);
      return next_token(state, pEQ3, start);
    default:
      return next_token(state, pEQ2, start);
    }

    break;
  }

  return NullToken;
}

static token lex_underscore(lexstate *state, position start) {
  unsigned int c;

  c = peek(state);

  if (('A' <= c && c <= 'Z') || c == '_') {
    skipbyte(state, c, 1);

    while (true) {
      c = peek(state);

      if (rb_isalnum(c) || c == '_') {
        // ok
        skipbyte(state, c, 1);
      } else {
        break;
      }
    }

    return next_token(state, tULIDENT, start);
  } else {
    return next_token(state, tULIDENT, start);
  }
}

static token lex_global(lexstate *state, position start) {
  unsigned int c;

  c = peek(state);

  if (rb_isalpha(c) || c == '_') {
    skipbyte(state, c, 1);

    while (true) {
      c = peek(state);
      if (rb_isalnum(c) || c == '_') {
        skipbyte(state, c, 1);
      } else {
        return next_token(state, tGIDENT, start);
      }
    }
  }

  return NullToken;
}

void pp(VALUE object) {
  VALUE inspect = rb_funcall(object, rb_intern("inspect"), 0);
  printf("pp >> %s\n", RSTRING_PTR(inspect));
}

static token lex_number(lexstate *state, position start) {
}

static token lex_ident(lexstate *state, position start, enum TokenType default_type) {
  unsigned int c;
  token tok;

  while (true) {
    c = peek(state);
    if (rb_isalnum(c) || c == '_') {
      skip(state, c);
    } else if (c == '!') {
      skipbyte(state, c, 1);
      tok = next_token(state, tBANGIDENT, start);
      break;
    } else if (c == '=') {
      skipbyte(state, c, 1);
      tok = next_token(state, tEQIDENT, start);
      break;
    } else {
      tok = next_token(state, default_type, start);
      break;
    }
  }

  if (tok.type == tLIDENT) {
    VALUE string = rb_enc_str_new(
      RSTRING_PTR(state->string) + tok.start.byte_pos,
      tok.end.byte_pos - tok.start.byte_pos,
      rb_enc_get(state->string)
    );

    VALUE type = rb_hash_aref(rbsparser_Keywords, string);
    if (FIXNUM_P(type)) {
      tok.type = FIX2INT(type);
    }
  }

  return tok;
}

token rbsparser_next_token(lexstate *state) {
  token tok = NullToken;
  position start = NullPosition;

  unsigned int c;
  bool skipping = true;

  while (skipping) {
    start = state->current;
    c = peek(state);
    skip(state, c);

    switch (c) {
    case ' ':
    case '\t':
    case '\n':
      // nop
      break;
    default:
      skipping = false;
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
    ONE_CHAR_PATTERN('?', pQUESTION);
    case ':':
      tok = lex_colon(state, start);
      break;
    case '-':
      tok = lex_hyphen(state, start);
      break;
    case '.':
      tok = lex_dot(state, start, 1);
      break;
    case '=':
      tok = lex_eq(state, start, 1);
      break;
    case '_':
      tok = lex_underscore(state, start);
      break;
    case '$':
      tok = lex_global(state, start);
      break;
    default:
      if (rb_isalpha(c) && rb_isupper(c)) {
        tok = lex_ident(state, start, tUIDENT);
      }
      if (rb_isalpha(c) && rb_islower(c)) {
        tok = lex_ident(state, start, tLIDENT);
      }
  }

  return tok;
}

char *peek_token(lexstate *state, token tok) {
  return RSTRING_PTR(state->string) + tok.start.byte_pos;
}
