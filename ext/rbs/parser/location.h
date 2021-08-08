#ifndef RBS_LOCATION_H
#define RBS_LOCATION_H

#include "ruby.h"
#include "lexer.h"

typedef struct rbs_loc_list {
  ID name;
  range rg;
  struct rbs_loc_list *next;
} rbs_loc_list;

typedef struct {
  VALUE buffer;
  range rg;
  rbs_loc_list *requireds;
  rbs_loc_list *optionals;
} rbs_loc;

rbs_loc_list *rbs_loc_list_add(rbs_loc_list *list, ID name, range r);
rbs_loc_list *rbs_loc_list_dup(rbs_loc_list *list);
bool rbs_loc_list_find(const rbs_loc_list *list, ID name, range *rg);
void rbs_loc_list_free(rbs_loc_list *list);
size_t rbs_loc_list_size(const rbs_loc_list *list);

position rbs_loc_position(int char_pos);
position rbs_loc_position3(int char_pos, int line, int column);

VALUE rbs_new_location(VALUE buffer, range rg);
rbs_loc *check_location(VALUE location);
void rbs_loc_init0(rbs_loc *loc);
void rbs_loc_init(rbs_loc *loc, VALUE buffer, range rg);
void rbs_loc_add_required_child(rbs_loc *loc, ID name, range r);
void rbs_loc_add_optional_child(rbs_loc *loc, ID name, range r);
void rbs_loc_free(rbs_loc *loc);

void init_location();

#endif
