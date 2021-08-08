#ifndef RBS__PARSER_H
#define RBS__PARSER_H

#include "ruby.h"
#include "lexer.h"
#include "location.h"

typedef struct id_table {
  size_t size;
  size_t count;
  ID *ids;
  struct id_table *next;
} id_table;

typedef struct {
  position start;
  position end;
  size_t line_size;
  size_t line_count;
  token *tokens;
} comment;

typedef struct {
  lexstate *lexstate;
  token current_token;
  token next_token;
  token next_token2;
  VALUE buffer;
  id_table *vars;
  comment *last_comment;
} parserstate;

VALUE parse_type(parserstate *state);
VALUE parse_method_type(parserstate *state);
VALUE parse_signature(parserstate *state);

void rbs_unescape_string(VALUE string);

id_table *parser_push_table(parserstate *state);
void parser_insert_id(parserstate *state, ID id);
bool parser_id_member(parserstate *state, ID id);
void parser_pop_table(parserstate *state);

void print_parser(parserstate *state);
void parser_advance(parserstate *state);
void parser_advance_assert(parserstate *state, enum TokenType type);

void insert_comment_line(parserstate *state, token token);
VALUE get_comment(parserstate *state, int subject_line);

#define INTERN_TOKEN(parserstate, tok) \
  rb_intern3(\
    peek_token(parserstate->lexstate, tok),\
    token_bytes(tok),\
    rb_enc_get(parserstate->lexstate->string)\
  )

#endif
