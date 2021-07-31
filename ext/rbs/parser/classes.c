#include "parser.h"

VALUE RBS_AST;

VALUE RBS_Namespace;
VALUE RBS_TypeName;

VALUE RBS_Types;
VALUE RBS_Types_Bases;
VALUE RBS_Types_Bases_Self;
VALUE RBS_Types_ClassInstance;
VALUE RBS_Types_Alias;
VALUE RBS_Types_Interface;
VALUE RBS_Types_Union;
VALUE RBS_Types_Intersection;
VALUE RBS_Types_ClassSingleton;
VALUE RBS_Types_Tuple;
VALUE RBS_Types_Optional;
VALUE RBS_Location;

VALUE rbs_self_type(VALUE location) {
  VALUE args = rb_hash_new();
  rb_hash_aset(args, ID2SYM(rb_intern("location")), location);

  return rb_funcallv_kw(
    RBS_Types_Bases_Self,
    rb_intern("new"),
    1,
    &args,
    RB_PASS_KEYWORDS
  );
}

VALUE rbs_namespace(VALUE path, VALUE absolute) {
  VALUE args = rb_hash_new();
  rb_hash_aset(args, ID2SYM(rb_intern("path")), path);
  rb_hash_aset(args, ID2SYM(rb_intern("absolute")), absolute);

  return rb_funcallv_kw(
    RBS_Namespace,
    rb_intern("new"),
    1,
    &args,
    RB_PASS_KEYWORDS
  );
}

VALUE rbs_type_name(VALUE namespace, VALUE name) {
  VALUE args = rb_hash_new();
  rb_hash_aset(args, ID2SYM(rb_intern("namespace")), namespace);
  rb_hash_aset(args, ID2SYM(rb_intern("name")), name);

  return rb_funcallv_kw(
    RBS_TypeName,
    rb_intern("new"),
    1,
    &args,
    RB_PASS_KEYWORDS
  );
}

VALUE rbs_class_instance(VALUE typename, VALUE type_args) {
  VALUE args = rb_hash_new();
  rb_hash_aset(args, ID2SYM(rb_intern("name")), typename);
  rb_hash_aset(args, ID2SYM(rb_intern("args")), type_args);
  rb_hash_aset(args, ID2SYM(rb_intern("location")), Qnil);

  return rb_funcallv_kw(
    RBS_Types_ClassInstance,
    rb_intern("new"),
    1,
    &args,
    RB_PASS_KEYWORDS
  );
}

VALUE rbs_class_singleton(VALUE typename) {
  VALUE args = rb_hash_new();
  rb_hash_aset(args, ID2SYM(rb_intern("name")), typename);
  rb_hash_aset(args, ID2SYM(rb_intern("location")), Qnil);

  return rb_funcallv_kw(
    RBS_Types_ClassSingleton,
    rb_intern("new"),
    1,
    &args,
    RB_PASS_KEYWORDS
  );
}

VALUE rbs_alias(VALUE typename) {
  VALUE args = rb_hash_new();
  rb_hash_aset(args, ID2SYM(rb_intern("name")), typename);
  rb_hash_aset(args, ID2SYM(rb_intern("location")), Qnil);

  return rb_funcallv_kw(
    RBS_Types_Alias,
    rb_intern("new"),
    1,
    &args,
    RB_PASS_KEYWORDS
  );
}

VALUE rbs_interface(VALUE typename, VALUE type_args) {
  VALUE args = rb_hash_new();
  rb_hash_aset(args, ID2SYM(rb_intern("name")), typename);
  rb_hash_aset(args, ID2SYM(rb_intern("args")), type_args);
  rb_hash_aset(args, ID2SYM(rb_intern("location")), Qnil);

  return rb_funcallv_kw(
    RBS_Types_Interface,
    rb_intern("new"),
    1,
    &args,
    RB_PASS_KEYWORDS
  );
}

VALUE rbs_union(VALUE types) {
  VALUE args = rb_hash_new();
  rb_hash_aset(args, ID2SYM(rb_intern("types")), types);
  rb_hash_aset(args, ID2SYM(rb_intern("location")), Qnil);

  return rb_funcallv_kw(
    RBS_Types_Union,
    rb_intern("new"),
    1,
    &args,
    RB_PASS_KEYWORDS
  );
}

VALUE rbs_intersection(VALUE types) {
  VALUE args = rb_hash_new();
  rb_hash_aset(args, ID2SYM(rb_intern("types")), types);
  rb_hash_aset(args, ID2SYM(rb_intern("location")), Qnil);

  return rb_funcallv_kw(
    RBS_Types_Intersection,
    rb_intern("new"),
    1,
    &args,
    RB_PASS_KEYWORDS
  );
}

VALUE rbs_tuple(VALUE types) {
  VALUE args = rb_hash_new();
  rb_hash_aset(args, ID2SYM(rb_intern("types")), types);
  rb_hash_aset(args, ID2SYM(rb_intern("location")), Qnil);

  return rb_funcallv_kw(
    RBS_Types_Tuple,
    rb_intern("new"),
    1,
    &args,
    RB_PASS_KEYWORDS
  );
}

VALUE rbs_optional(VALUE type) {
  VALUE args = rb_hash_new();
  rb_hash_aset(args, ID2SYM(rb_intern("type")), type);
  rb_hash_aset(args, ID2SYM(rb_intern("location")), Qnil);

  return rb_funcallv_kw(
    RBS_Types_Optional,
    rb_intern("new"),
    1,
    &args,
    RB_PASS_KEYWORDS
  );
}

VALUE rbs_location(VALUE buffer, int start_pos, int end_pos) {
  return rb_funcall(
    RBS_Location,
    rb_intern("new"),
    3,
    buffer,
    INT2FIX(start_pos),
    INT2FIX(end_pos)
  );
}

VALUE rbs_location_pp(VALUE buffer, const position *start_pos, const position *end_pos) {
  return rbs_location(buffer, start_pos->char_pos, end_pos->char_pos);
}

VALUE rbs_location_tok(VALUE buffer, const token *tok) {
  return rbs_location(buffer, tok->start.char_pos, tok->end.char_pos);
}
