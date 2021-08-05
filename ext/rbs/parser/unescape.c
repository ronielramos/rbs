#include "parser.h"

static VALUE REGEXP = 0;
static VALUE HASH = 0;

static const char *regexp_str = "\\\\[abefnrstv\"]";

static ID gsub = 0;

/*
  Unescape escape sequences:
    '\\n' => "\n"
*/
void rbs_unescape_string(VALUE string) {
  if (!REGEXP) {
    REGEXP = rb_reg_new(regexp_str, strlen(regexp_str), 0);
    rb_global_variable(&REGEXP);
  }

  if (!gsub) {
    gsub = rb_intern("gsub!");
  }

  if (!HASH) {
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
    rb_global_variable(&HASH);
  }

  rb_funcall(string, gsub, 2, REGEXP, HASH);
}
