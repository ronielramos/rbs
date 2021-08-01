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
