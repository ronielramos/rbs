#include "parser.h"

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

VALUE rbs_record(VALUE fields, VALUE location) {
  VALUE args = rb_hash_new();
  rb_hash_aset(args, ID2SYM(rb_intern("location")), location);
  rb_hash_aset(args, ID2SYM(rb_intern("fields")), fields);

  return rb_funcallv_kw(
    RBS_Types_Record,
    rb_intern("new"),
    1,
    &args,
    RB_PASS_KEYWORDS
  );
}

VALUE rbs_variable(VALUE name, VALUE location) {
  VALUE args = rb_hash_new();
  rb_hash_aset(args, ID2SYM(rb_intern("location")), location);
  rb_hash_aset(args, ID2SYM(rb_intern("name")), name);

  return rb_funcallv_kw(
    RBS_Types_Variable,
    rb_intern("new"),
    1,
    &args,
    RB_PASS_KEYWORDS
  );
}

VALUE rbs_method_type(VALUE type_params, VALUE type, VALUE block, VALUE location) {
  VALUE args = rb_hash_new();
  rb_hash_aset(args, ID2SYM(rb_intern("type_params")), type_params);
  rb_hash_aset(args, ID2SYM(rb_intern("type")), type);
  rb_hash_aset(args, ID2SYM(rb_intern("block")), block);
  rb_hash_aset(args, ID2SYM(rb_intern("location")), location);

  return rb_funcallv_kw(
    RBS_MethodType,
    rb_intern("new"),
    1,
    &args,
    RB_PASS_KEYWORDS
  );
}

VALUE rbs_ast_comment(VALUE string, VALUE location) {
  VALUE args = rb_hash_new();
  rb_hash_aset(args, ID2SYM(rb_intern("string")), string);
  rb_hash_aset(args, ID2SYM(rb_intern("location")), location);

  return rb_funcallv_kw(
    RBS_AST_Comment,
    rb_intern("new"),
    1,
    &args,
    RB_PASS_KEYWORDS
  );
}

VALUE rbs_ast_decl_constant(VALUE name, VALUE type, VALUE location, VALUE comment) {
  VALUE args = rb_hash_new();
  rb_hash_aset(args, ID2SYM(rb_intern("name")), name);
  rb_hash_aset(args, ID2SYM(rb_intern("type")), type);
  rb_hash_aset(args, ID2SYM(rb_intern("location")), location);
  rb_hash_aset(args, ID2SYM(rb_intern("comment")), comment);

  return rb_funcallv_kw(
    RBS_AST_Declarations_Constant,
    rb_intern("new"),
    1,
    &args,
    RB_PASS_KEYWORDS
  );
}

VALUE rbs_ast_decl_global(VALUE name, VALUE type, VALUE location, VALUE comment) {
  VALUE args = rb_hash_new();
  rb_hash_aset(args, ID2SYM(rb_intern("name")), name);
  rb_hash_aset(args, ID2SYM(rb_intern("type")), type);
  rb_hash_aset(args, ID2SYM(rb_intern("location")), location);
  rb_hash_aset(args, ID2SYM(rb_intern("comment")), comment);

  return rb_funcallv_kw(
    RBS_AST_Declarations_Global,
    rb_intern("new"),
    1,
    &args,
    RB_PASS_KEYWORDS
  );
}
