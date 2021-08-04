#include "parser.h"

#define ONE_CHAR_PATTERN(c, t) case c: tok = next_token(state, t, start); break

/**
 * Returns one character at current.
 *
 * ... A B C ...
 *    ^           current => A
 * */
#define peek(state) rb_enc_mbc_to_codepoint(RSTRING_PTR(state->string) + state->current.byte_pos, RSTRING_END(state->string), rb_enc_get(state->string))

token NullToken = { NullType };
position NullPosition = { 0 };

unsigned int peekn(lexstate *state, unsigned int chars[], size_t length) {
  int byteoffset = 0;

  rb_encoding *encoding = rb_enc_get(state->string);
  char *start = RSTRING_PTR(state->string) + state->current.byte_pos;
  char *end = RSTRING_END(state->string);

  for (size_t i = 0; i < length; i++)
  {
    chars[i] = rb_enc_mbc_to_codepoint(start + byteoffset, end, encoding);
    byteoffset += rb_enc_codelen(chars[i], rb_enc_get(state->string));
  }

  return byteoffset;
}

int token_chars(token tok) {
  return tok.end.char_pos - tok.start.char_pos;
}

int token_bytes(token tok) {
  return tok.end.byte_pos - tok.start.byte_pos;
}

/**
 * ... token ...
 *    ^             start
 *          ^       current
 *
 * */
token next_token(lexstate *state, enum TokenType type, position start) {
  token t;

  t.type = type;
  t.start = start;
  t.end = state->current;
  state->first_token_of_line = false;

  return t;
}

/**
 * ... c1c2c3 ...
 *    ^              current
 *           ^       current + len
 *           ^       After skip
 *
 * */
void skipbyte(lexstate *state, unsigned int c, int len) {
  state->current.char_pos += 1;
  state->current.byte_pos += len;

  if (c == '\n') {
    state->current.line += 1;
    state->current.column = 0;
    state->first_token_of_line = true;
  } else {
    state->current.column += 1;
  }
}

void skip(lexstate *state, unsigned int c) {
  int len = rb_enc_codelen(c, rb_enc_get(state->string));
  skipbyte(state, c, len);
}

/*
   ... 0 1 ...
        ^           current
          ^         current (return)
*/
static token lex_number(lexstate *state, position start) {
  unsigned int c;

  while (true) {
    c = peek(state);

    if (rb_isdigit(c) || c == '_') {
      skipbyte(state, c, 1);
    } else {
      break;
    }
  }

  return next_token(state, tINTEGER, start);
}

/*
  ... - > ...
     ^              start
       ^            current
         ^          current (after)

  ... - 1 ...
     ^              start
       ^            current
       ^            current (lex_number)

  ... - x ...
     ^              start
       ^            current
       ^            current (after)
*/
static token lex_hyphen(lexstate* state, position start) {
  unsigned int c = peek(state);

  if (c == '>') {
    skipbyte(state, c, 1);
    return next_token(state, pARROW, start);
  } else if (rb_isdigit(c)) {
    skipbyte(state, c, 1);
    return lex_number(state, start);
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
      skip(state, c);
      return lex_eq(state, start, 2);
      break;
    case '>':
      skipbyte(state, c, 1);
      return next_token(state, pFATARROW, start);
    default:
      return next_token(state, pEQ, start);
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

static token lex_comment(lexstate *state, enum TokenType type) {
  unsigned int c;

  c = peek(state);
  if (c == ' ') {
    skip(state, c);
  }

  position start = state->current;

  while (true) {
    c = peek(state);

    if (c == '\n' || c == '\0') {
      break;
    } else {
      skip(state, c);
    }
  }

  token tok = next_token(state, type, start);

  skip(state, c);

  return tok;
}

/*
   ... " ... " ...
      ^               start
        ^             current
              ^       current (after)
*/
static token lex_dqstring(lexstate *state, position start) {
  unsigned int c;

  c = peek(state);

  if (c != '"') {
    while (true) {
      c = peek(state);
      skip(state, c);

      if (c == '\\') {
        if (peek(state) == '"') {
          skip(state, c);
          c = peek(state);
        }
      } else if (c == '"') {
        break;
      }
    }
  }

  return next_token(state, tDQSTRING, start);
}

/*
   ... @ foo ...
      ^             start
        ^           current
            ^       current (return)

   ... @ @ foo ...
      ^               start
        ^             current
              ^       current (return)
*/
static token lex_ivar(lexstate *state, position start) {
  unsigned int c;

  enum TokenType type = tAIDENT;

  c = peek(state);

  if (c == '@') {
    type = tA2IDENT;
    skip(state, c);
  }

  while (rb_isalnum(c) || c == '_') {
    skip(state, c);
    c = peek(state);
  }

  return next_token(state, type, start);
}

/*
   ... ' ... ' ...
      ^               start
        ^             current
              ^       current (after)
*/
static token lex_sqstring(lexstate *state, position start) {
  unsigned int c;

  c = peek(state);

  if (c != '\'') {
    while (true) {
      c = peek(state);
      skip(state, c);

      if (c == '\\') {
        if (peek(state) == '\'') {
          skip(state, c);
          c = peek(state);
        }
      } else if (c == '\'') {
        break;
      }
    }
  }

  return next_token(state, tSQSTRING, start);
}

#define EQPOINTS2(c0, c1, s) (c0 == s[0] && c1 == s[1])
#define EQPOINTS3(c0, c1, c2, s) (c0 == s[0] && c1 == s[1] && c2 == s[2])

/*
   ... : @ ...
      ^          start
        ^        current
          ^      current (return)
*/
static token lex_colon_symbol(lexstate *state, position start) {
  unsigned int c[3];
  peekn(state, c, 3);

  switch (c[0]) {
    case '|':
    case '&':
    case '/':
    case '%':
    case '~':
    case '`':
    case '^':
      skip(state, c[0]);
      return next_token(state, tSYMBOL, start);
    case '=':
      if (EQPOINTS2(c[0], c[1], "=~")) {
        // :=~
        skip(state, c[0]);
        skip(state, c[1]);
        return next_token(state, tSYMBOL, start);
      } else if (EQPOINTS3(c[0], c[1], c[2], "===")) {
        // :===
        skip(state, c[0]);
        skip(state, c[1]);
        skip(state, c[2]);
        return next_token(state, tSYMBOL, start);
      } else if (EQPOINTS2(c[0], c[1], "==")) {
        // :==
        skip(state, c[0]);
        skip(state, c[1]);
        return next_token(state, tSYMBOL, start);
      }
      break;
    case '<':
      printf("%c %c %c\n", c[0], c[1], c[2]);
      if (EQPOINTS3(c[0], c[1], c[2], "<=>")) {
        skip(state, c[0]);
        skip(state, c[1]);
        skip(state, c[2]);
      } else if (EQPOINTS2(c[0], c[1], "<=") || EQPOINTS2(c[0], c[1], "<<")) {
        skip(state, c[0]);
        skip(state, c[1]);
      } else {
        skip(state, c[0]);
      }
      return next_token(state, tSYMBOL, start);
    case '>':
      if (EQPOINTS2(c[0], c[1], ">=") || EQPOINTS2(c[0], c[1], ">>")) {
        skip(state, c[0]);
        skip(state, c[1]);
      } else {
        skip(state, c[0]);
      }
      return next_token(state, tSYMBOL, start);
    case '-':
    case '+':
      if (EQPOINTS2(c[0], c[1], "+@") || EQPOINTS2(c[0], c[1], "-@")) {
        skip(state, c[0]);
        skip(state, c[1]);
      } else {
        skip(state, c[0]);
      }
      return next_token(state, tSYMBOL, start);
    case '*':
      if (EQPOINTS2(c[0], c[1], "**")) {
        skip(state, c[0]);
        skip(state, c[1]);
      } else {
        skip(state, c[0]);
      }
      return next_token(state, tSYMBOL, start);
    case '[':
      if (EQPOINTS3(c[0], c[1], c[2], "[]=")) {
        skip(state, c[0]);
        skip(state, c[1]);
        skip(state, c[2]);
      } else if (EQPOINTS2(c[0], c[1], "[]")) {
        skip(state, c[0]);
        skip(state, c[1]);
      } else {
        break;
      }
      return next_token(state, tSYMBOL, start);
    case '!':
      if (EQPOINTS2(c[0], c[1], "!=") || EQPOINTS2(c[0], c[1], "!~")) {
        skip(state, c[0]);
        skip(state, c[1]);
      } else {
        break;
      }
      return next_token(state, tSYMBOL, start);
    case '@':
      skip(state, '@');
      lex_ivar(state, start);
      return next_token(state, tSYMBOL, start);
    case '$':
      skip(state, '$');
      lex_global(state, start);
      return next_token(state, tSYMBOL, start);
    case '\'':
      skip(state, '\'');
      lex_sqstring(state, start);
      return next_token(state, tSYMBOL, start);
    case '"':
      skip(state, '"');
      lex_dqstring(state, start);
      return next_token(state, tSYMBOL, start);
    default:
      if (rb_isalpha(c[0]) || c[0] == '_') {
        lex_ident(state, start, NullType);

        if (peek(state) == '?') {
          skip(state, '?');
        }

        return next_token(state, tSYMBOL, start);
      }
  }

  return next_token(state, pCOLON, start);
}

/*
   ... : : ...
      ^          start
        ^        current
          ^      current (return)

   ... :   ...
      ^          start
        ^        current (lex_colon_symbol)
*/
static token lex_colon(lexstate *state, position start) {
  unsigned int c = peek(state);

  if (c == ':') {
    skipbyte(state, c, 1);
    return next_token(state, pCOLON2, start);
  } else {
    return lex_colon_symbol(state, start);
  }
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
    case '\0':
      return next_token(state, pEOF, start);
    default:
      skipping = false;
      break;
    }
  }


  /* ... c d ..                */
  /*      ^     state->current */
  /*    ^       start          */
  switch (c) {
    case '\0': tok = next_token(state, pEOF, start);
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
    case '#':
      if (state->first_token_of_line) {
        tok = lex_comment(state, tLINECOMMENT);
      } else {
        tok = lex_comment(state, tCOMMENT);
      }
      break;
    case ':':
      tok = lex_colon(state, start);
      break;
    case '-':
      tok = lex_hyphen(state, start);
      break;
    case '+':
      if (rb_isdigit(peek(state))) {
        return lex_number(state, start);
      } else {
        return next_token(state, pPLUS, start);
      }
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
    case '@':
      tok = lex_ivar(state, start);
    case '"':
      tok = lex_dqstring(state, start);
      break;
    case '\'':
      tok = lex_sqstring(state, start);
      break;
    default:
      if (rb_isalpha(c) && rb_isupper(c)) {
        tok = lex_ident(state, start, tUIDENT);
      }
      if (rb_isalpha(c) && rb_islower(c)) {
        tok = lex_ident(state, start, tLIDENT);
      }
      if (rb_isdigit(c)) {
        tok = lex_number(state, start);
      }
  }

  return tok;
}

char *peek_token(lexstate *state, token tok) {
  return RSTRING_PTR(state->string) + tok.start.byte_pos;
}
