#include "parser.h"

id_table *parser_push_table(parserstate *state) {
  id_table *table;

  if (state->vars) {
    table = state->vars;
    while (table->next != NULL) {
      table = table->next;
    }

    table->next = calloc(1, sizeof(id_table));
    table = table->next;
  } else {
    table = calloc(1, sizeof(id_table));
    state->vars = table;
  }

  table->size = 10;
  table->ids = calloc(10, sizeof(ID));
  table->count = 0;

  return table;
}

void parser_insert_id(parserstate *state, ID id) {
  id_table *table;

  table = state->vars;

  while (table->next != NULL) {
    table = table->next;
  }

  if (table->size == table->count) {
    ID *copy = calloc(table->size * 2, sizeof(ID));
    table->size *= 2;
    memcpy(copy, table->ids, table->count * sizeof(ID));
    free(table->ids);
    table->ids = copy;
  }

  table->ids[table->count] = id;
  table->count += 1;

  return;
}

bool parser_id_member(parserstate *state, ID id) {
  id_table *table;

  table = state->vars;

  while (true) {
    if (table == NULL) break;

    for (size_t i = 0; i < table->count; i++) {
      if (table->ids[i] == id) {
        return true;
      }
    }

    table = table->next;
  }

  return false;
}

void parser_pop_table(parserstate *state) {
  id_table *table;
  id_table *parent;

  if (state->vars->next == NULL) {
    free(state->vars->ids);
    free(state->vars);
    state->vars = NULL;
  } else {
    parent = state->vars;
    table = state->vars->next;

    while (table->next != NULL) {
      parent = table;
      table = table->next;
    }

    free(table->ids);
    free(table);

    parent->next = NULL;
  }
}


void print_parser(parserstate *state) {
  pp(state->buffer);
  printf("  current_token = %s (%d...%d)\n", RBS_TOKENTYPE_NAMES[state->current_token.type], state->current_token.start.char_pos, state->current_token.end.char_pos);
  printf("     next_token = %s (%d...%d)\n", RBS_TOKENTYPE_NAMES[state->next_token.type], state->next_token.start.char_pos, state->next_token.end.char_pos);
  printf("    next_token2 = %s (%d...%d)\n", RBS_TOKENTYPE_NAMES[state->next_token2.type], state->next_token2.start.char_pos, state->next_token2.end.char_pos);
}

void parser_advance(parserstate *state) {
  state->current_token = state->next_token;
  state->next_token = state->next_token2;

  while (true) {
    if (state->next_token2.type == pEOF) {
      break;
    }

    state->next_token2 = rbsparser_next_token(state->lexstate);

    if (state->next_token2.type == tCOMMENT) {
      // skip
    } else if (state->next_token2.type == tLINECOMMENT) {
      insert_comment_line(state, state->next_token2);
    } else {
      break;
    }
  }
}

void parser_advance_assert(parserstate *state, enum TokenType type) {
  parser_advance(state);
  if (state->current_token.type != type) {
    print_token(state->current_token);
    rb_raise(
      rb_eRuntimeError,
      "Unexpected token: expected=%s, actual=%s",
      RBS_TOKENTYPE_NAMES[type],
      RBS_TOKENTYPE_NAMES[state->current_token.type]
    );
  }
}

void print_token(token tok) {
  printf(
    "%s char=%d...%d\n",
    RBS_TOKENTYPE_NAMES[tok.type],
    tok.start.char_pos,
    tok.end.char_pos
  );
}

void insert_comment_line(parserstate *state, token tok) {
  if (state->last_comment) {
    if (state->last_comment->end.line != tok.start.line - 1) {
      free(state->last_comment->tokens);
      free(state->last_comment);
      state->last_comment = NULL;
    }
  }

  if (!state->last_comment) {
    state->last_comment = calloc(1, sizeof(comment));
    state->last_comment->tokens = calloc(10, sizeof(token));
    state->last_comment->line_size = 10;
    state->last_comment->start = tok.start;
  }

  if (state->last_comment->line_count == state->last_comment->line_size) {
    token *p = state->last_comment->tokens;
    state->last_comment->tokens = calloc(state->last_comment->line_size + 10, sizeof(token));
    memcpy(state->last_comment->tokens, p, state->last_comment->line_size * sizeof(token));
    state->last_comment->line_size += 10;
    free(p);
  }

  state->last_comment->tokens[state->last_comment->line_count++] = tok;
  state->last_comment->end = tok.end;
}

VALUE get_comment(parserstate *state, int subject_line) {
  comment *comment = state->last_comment;

  if (!comment) return Qnil;

  printf("comment->end.line = %d\n", comment->end.line);
  if (comment->end.line != subject_line - 1) return Qnil;

  VALUE content = rb_funcall(state->buffer, rb_intern("content"), 0);
  VALUE string = rb_enc_str_new_cstr("", rb_enc_get(content));

  for (size_t i = 0; i < comment->line_count; i++) {
    token tok = comment->tokens[i];
    char *p = peek_token(state->lexstate, tok);
    rb_str_cat(string, p, tok.end.byte_pos - tok.start.byte_pos);
    rb_str_cat_cstr(string, "\n");
  }

  return rbs_ast_comment(
    string,
    rbs_location_pp(state->buffer, &comment->start, &comment->end)
  );
}
