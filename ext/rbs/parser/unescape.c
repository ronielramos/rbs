#include "parser.h"

static VALUE REGEXP = NULL;
static VALUE HASH = NULL;

static const char *regexp_str = "\\\\[abefnrstv\"]";

/*
  Unescape escape sequences:
    '\\n' => "\n"
*/
void rbs_unescape_string(VALUE string) {
  if (REGEXP == NULL) {
    REGEXP = rb_reg_new(regexp_str, strlen(regexp_str), 0);
  }

  if (HASH == NULL) {
    HASH = rb_hash_new();
    rb_hash_aset(HASH, rb_str_new_literal("\\a"), rb_str_new_literal("\a"));
    rb_hash_aset(HASH, rb_str_new_literal("\\b"), rb_str_new_literal("\b"));
    rb_hash_aset(HASH, rb_str_new_literal("\\e"), rb_str_new_literal("\e"));
    rb_hash_aset(HASH, rb_str_new_literal("\\f"), rb_str_new_literal("\f"));
    rb_hash_aset(HASH, rb_str_new_literal("\\n"), rb_str_new_literal("\n"));
    rb_hash_aset(HASH, rb_str_new_literal("\\r"), rb_str_new_literal("\r"));
    rb_hash_aset(HASH, rb_str_new_literal("\\s"), rb_str_new_literal(" "));
    rb_hash_aset(HASH, rb_str_new_literal("\\t"), rb_str_new_literal("\t"));
    rb_hash_aset(HASH, rb_str_new_literal("\\v"), rb_str_new_literal("\v"));
    rb_hash_aset(HASH, rb_str_new_literal("\\\""), rb_str_new_literal("\""));
  }

  rb_funcall(string, rb_intern("gsub!"), 2, REGEXP, HASH);
}
