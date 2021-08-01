#include "parser.h"

VALUE RBS_AST;

VALUE RBS_Namespace;
VALUE RBS_TypeName;

VALUE RBS_Types;
VALUE RBS_Types_Bases;
VALUE RBS_Types_Bases_Any;
VALUE RBS_Types_Bases_Bool;
VALUE RBS_Types_Bases_Bottom;
VALUE RBS_Types_Bases_Class;
VALUE RBS_Types_Bases_Instance;
VALUE RBS_Types_Bases_Nil;
VALUE RBS_Types_Bases_Self;
VALUE RBS_Types_Bases_Top;
VALUE RBS_Types_Bases_Void;
VALUE RBS_Types_ClassInstance;
VALUE RBS_Types_Alias;
VALUE RBS_Types_Interface;
VALUE RBS_Types_Union;
VALUE RBS_Types_Intersection;
VALUE RBS_Types_ClassSingleton;
VALUE RBS_Types_Tuple;
VALUE RBS_Types_Optional;
VALUE RBS_Location;
VALUE RBS_Types_Function;
VALUE RBS_Types_Function_Param;
VALUE RBS_Types_Block;
VALUE RBS_Types_Proc;
VALUE RBS_Types_Literal;

VALUE rbs_base_type(VALUE klass, VALUE location) {
  VALUE args = rb_hash_new();
  rb_hash_aset(args, ID2SYM(rb_intern("location")), location);

  return rb_funcallv_kw(
    klass,
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

VALUE rbs_location_current_token(parserstate *state) {
  return rbs_location(state->buffer, state->current_token.start.char_pos, state->current_token.end.char_pos);
}

VALUE rbs_block(VALUE type, VALUE required) {
  VALUE args = rb_hash_new();
  rb_hash_aset(args, ID2SYM(rb_intern("type")), type);
  rb_hash_aset(args, ID2SYM(rb_intern("required")), required);

  return rb_funcallv_kw(
    RBS_Types_Block,
    rb_intern("new"),
    1,
    &args,
    RB_PASS_KEYWORDS
  );
}

VALUE rbs_function_param(VALUE type, VALUE name, VALUE location) {
  VALUE args = rb_hash_new();
  rb_hash_aset(args, ID2SYM(rb_intern("type")), type);
  rb_hash_aset(args, ID2SYM(rb_intern("name")), name);
  rb_hash_aset(args, ID2SYM(rb_intern("location")), location);

  return rb_funcallv_kw(
    RBS_Types_Function_Param,
    rb_intern("new"),
    1,
    &args,
    RB_PASS_KEYWORDS
  );
}

VALUE rbs_function(
  VALUE required_positional_params,
  VALUE optional_positional_params,
  VALUE rest_positional_param,
  VALUE trailing_positional_params,
  VALUE required_keyword_params,
  VALUE optional_keyword_params,
  VALUE rest_keyword_param,
  VALUE return_type
) {
  VALUE args = rb_hash_new();
  rb_hash_aset(args, ID2SYM(rb_intern("required_positionals")), required_positional_params);
  rb_hash_aset(args, ID2SYM(rb_intern("optional_positionals")), optional_positional_params);
  rb_hash_aset(args, ID2SYM(rb_intern("rest_positionals")), rest_positional_param);
  rb_hash_aset(args, ID2SYM(rb_intern("trailing_positionals")), trailing_positional_params);
  rb_hash_aset(args, ID2SYM(rb_intern("required_keywords")), required_keyword_params);
  rb_hash_aset(args, ID2SYM(rb_intern("optional_keywords")), optional_keyword_params);
  rb_hash_aset(args, ID2SYM(rb_intern("rest_keywords")), rest_keyword_param);
  rb_hash_aset(args, ID2SYM(rb_intern("return_type")), return_type);

  return rb_funcallv_kw(
    RBS_Types_Function,
    rb_intern("new"),
    1,
    &args,
    RB_PASS_KEYWORDS
  );
}

VALUE rbs_proc(VALUE function, VALUE block, VALUE location) {
  VALUE args = rb_hash_new();
  rb_hash_aset(args, ID2SYM(rb_intern("type")), function);
  rb_hash_aset(args, ID2SYM(rb_intern("block")), block);
  rb_hash_aset(args, ID2SYM(rb_intern("location")), location);

  return rb_funcallv_kw(
    RBS_Types_Proc,
    rb_intern("new"),
    1,
    &args,
    RB_PASS_KEYWORDS
  );
}

VALUE rbs_void(VALUE location) {
  VALUE args = rb_hash_new();
  rb_hash_aset(args, ID2SYM(rb_intern("location")), location);

  return rb_funcallv_kw(
    RBS_Types_Bases_Void,
    rb_intern("new"),
    1,
    &args,
    RB_PASS_KEYWORDS
  );
}

VALUE rbs_literal(VALUE literal, VALUE location) {
  VALUE args = rb_hash_new();
  rb_hash_aset(args, ID2SYM(rb_intern("location")), location);
  rb_hash_aset(args, ID2SYM(rb_intern("literal")), literal);

  return rb_funcallv_kw(
    RBS_Types_Literal,
    rb_intern("new"),
    1,
    &args,
    RB_PASS_KEYWORDS
  );
}
