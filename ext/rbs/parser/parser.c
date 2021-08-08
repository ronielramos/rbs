#include "rbs_parser.h"
#include "ruby/internal/symbol.h"

#define KEYWORD_CASES case kBOOL:\
  case kBOT: \
  case kCLASS: \
  case kFALSE: \
  case kINSTANCE: \
  case kINTERFACE: \
  case kNIL: \
  case kSELF: \
  case kSINGLETON: \
  case kTOP: \
  case kTRUE: \
  case kVOID: \
  case kTYPE: \
  case kUNCHECKED: \
  case kIN: \
  case kOUT: \
  case kEND:

typedef struct {
  VALUE required_positionals;
  VALUE optional_positionals;
  VALUE rest_positionals;
  VALUE trailing_positionals;
  VALUE required_keywords;
  VALUE optional_keywords;
  VALUE rest_keywords;
} method_params;

static VALUE parse_optional(parserstate *state);
static VALUE parse_simple(parserstate *state);

static VALUE string_of_loc(parserstate *state, position start, position end) {
  return rb_enc_str_new(
    RSTRING_PTR(state->lexstate->string) + start.byte_pos,
      end.byte_pos - start.byte_pos,
      rb_enc_get(state->lexstate->string)
  );
}

static VALUE string_of_bytepos(parserstate *state, int byte_start, int byte_end) {
  return rb_enc_str_new(
    RSTRING_PTR(state->lexstate->string) + byte_start,
      byte_end - byte_start,
      rb_enc_get(state->lexstate->string)
  );
}

static void __attribute__((noreturn)) raise_syntax_error_e(parserstate *state, token tok, const char *expected) {
  rb_raise(
    rb_eRuntimeError,
    "Syntax error at line %d char %d, expected %s, but got %s",
    tok.range.start.line,
    tok.range.start.column,
    expected,
    token_type_str(tok.type)
  );
}

typedef enum {
  CLASS_NAME = 1,
  INTERFACE_NAME = 2,
  ALIAS_NAME = 4
} TypeNameKind;

/*
  type_name ::= {`::`} (tUIDENT `::`)* <tXIDENT>
              | {(tUIDENT `::`)*} <tXIDENT>
              | {<tXIDENT>}
*/
VALUE parse_type_name(parserstate *state, TypeNameKind kind) {
  VALUE absolute = Qfalse;
  VALUE path = rb_ary_new();
  VALUE namespace;

  if (state->current_token.type == pCOLON2) {
    absolute = Qtrue;
    parser_advance(state);
  }

  if (state->current_token.type == tUIDENT) {
    // may be prefix
    while (state->next_token.type == pCOLON2) {
      rb_ary_push(path, ID2SYM(INTERN_TOKEN(state, state->current_token)));

      parser_advance(state);
      parser_advance(state);
    }
  }
  namespace = rbs_namespace(path, absolute);

  switch (state->current_token.type) {
    case tLIDENT:
      if (kind & ALIAS_NAME) break;
    case tULIDENT:
      if (kind & INTERFACE_NAME) break;
    case tUIDENT:
      if (kind & CLASS_NAME) break;
    default: {
      VALUE ids = rb_ary_new();
      if (kind & ALIAS_NAME) {
        rb_ary_push(ids, rb_str_new_literal("alias name"));
      }
      if (kind & INTERFACE_NAME) {
        rb_ary_push(ids, rb_str_new_literal("interface name"));
      }
      if (kind & CLASS_NAME) {
        rb_ary_push(ids, rb_str_new_literal("class/module/constant name"));
      }

      raise_syntax_error_e(
        state,
        state->current_token,
        RSTRING_PTR(rb_funcall(ids, rb_intern("join"), 0))
      );
    }
  }

  return rbs_type_name(namespace, ID2SYM(INTERN_TOKEN(state, state->current_token)));
}

/*
  type_list ::= {}<> eol
              | {} type `,` ... <`,`> eol
              | {} type `,` ... `,` <type> eol
*/
static VALUE parse_type_list(parserstate *state, enum TokenType eol, VALUE types) {
  while (state->next_token.type != eol) {
    rb_ary_push(types, parse_type(state));

    if (state->next_token.type == pCOMMA) {
      parser_advance(state);
    } else {
      break;
    }
  }

  return types;
}

static bool is_keyword_token(enum TokenType type) {
  switch (type)
  {
  case tLIDENT:
  case tUIDENT:
  case tULIDENT:
  case kSINGLETON:
  case kSELF:
  case kINSTANCE:
  case kVOID:
    return true;
  default:
    return false;
  }
}

/*
  function_param ::= {} <type>
                   | {} type <param>
*/
static VALUE parse_function_param(parserstate *state) {
  range type_range;

  type_range.start = state->next_token.range.start;
  VALUE type = parse_type(state);
  type_range.end = state->current_token.range.end;

  if (state->next_token.type == pCOMMA || state->next_token.type == pRPAREN) {
    range param_range = type_range;

    VALUE location = rbs_new_location(state->buffer, param_range);
    rbs_loc *loc = check_location(location);
    rbs_loc_add_optional_child(loc, rb_intern("name"), NULL_RANGE);

    return rbs_function_param(type, Qnil, location);
  } else {
    range name_range = state->next_token.range;
    range param_range;

    parser_advance(state);
    param_range.start = type_range.start;
    param_range.end = name_range.end;

    VALUE name = ID2SYM(INTERN_TOKEN(state, state->current_token));
    VALUE location = rbs_new_location(state->buffer, param_range);
    rbs_loc *loc = check_location(location);
    rbs_loc_add_optional_child(loc, rb_intern("name"), name_range);

    return rbs_function_param(type, name, location);
  }
}

/*
  required_keyword ::= {} keyword `:` function_param <`,`>
                     | {} keyword `:` <function_param>
*/
static void parse_required_keyword(parserstate *state, method_params *params) {
  parser_advance(state);
  VALUE keyword = ID2SYM(INTERN_TOKEN(state, state->current_token));
  parser_advance_assert(state, pCOLON);
  VALUE param = parse_function_param(state);
  rb_hash_aset(params->required_keywords, keyword, param);

  if (state->next_token.type == pCOMMA) {
    parser_advance(state);
  }

  return;
}

/*
  optional_keyword ::= {`?`} keyword `:` function_param <`,`>
                     | {`?`} keyword `:` <function_param>
*/
static void parse_optional_keyword(parserstate *state, method_params *params) {
  parser_advance(state);
  VALUE keyword = ID2SYM(INTERN_TOKEN(state, state->current_token));
  parser_advance_assert(state, pCOLON);
  VALUE param = parse_function_param(state);
  rb_hash_aset(params->optional_keywords, keyword, param);

  if (state->next_token.type == pCOMMA) {
    parser_advance(state);
  }

  return;
}

/*
  keywords ::= {} `)`
             | {} `?` optional_keyword <keywords>
             | {} `**` function_param <keywords>
             | {} required_keyword <keywords>
*/
static void parse_keywords(parserstate *state, method_params *params) {
  while (true) {
    switch (state->next_token.type) {
    case pRPAREN:
      return;
    case pQUESTION:
      parser_advance(state);
      parse_optional_keyword(state, params);
      break;
    case pSTAR2:
      parser_advance(state);
      params->rest_keywords = parse_function_param(state);
      return;
    default:
      parse_required_keyword(state, params);
    }
  }
}

/*
  trailing_params ::= {<>} `)`
                    | {} `?` optional_keyword <keywords>
                    | {} <keywords>
                    | {} function_param `,` <trailing_params>
                    | {} function_param <trailing_params>         (FIXME)

*/
static void parse_trailing_params(parserstate *state, method_params *params) {
  while (true) {
    if (state->next_token.type == pRPAREN) {
      return;
    }

    if (state->next_token.type == pQUESTION) {
      parser_advance(state);
      parse_optional_keyword(state, params);
      parse_keywords(state, params);
      return;
    }

    if (state->next_token2.type == pSTAR2) {
      parse_keywords(state, params);
      return;
    }

    if (is_keyword_token(state->next_token.type)) {
      if (state->next_token2.type == pCOLON) {
        parse_keywords(state, params);
        return;
      }
    }

    VALUE param = parse_function_param(state);
    rb_ary_push(params->trailing_positionals, param);

    if (state->next_token.type == pCOMMA) {
      parser_advance(state);
    }
  }
}

/**
 * ... `?` Foo bar `,` ...
 *    >               >
 *
 * */
/*
  optional_params ::= {<>} `)`
                    | {} `*` <function_param> `)`
                    | {} `*` function_param `,` <trailing_params>
                    | {} `?` function_param `,` <optional_params>
                    | {} `?` function_param <optional_params>      (FIXME)
                    | {} <keywords>
*/
static void parse_optional_params(parserstate *state, method_params *params) {
  while (true) {
    if (state->next_token.type == pRPAREN) {
      return;
    }

    if (state->next_token.type == pSTAR) {
      parser_advance(state);
      params->rest_positionals = parse_function_param(state);

      if (state->next_token.type == pRPAREN) {
        return;
      } else {
        parser_advance_assert(state, pCOMMA);
        parse_trailing_params(state, params);
        return;
      }
    }

    if (state->next_token.type == pQUESTION) {
      parser_advance(state);

      if (is_keyword_token(state->next_token.type)) {
        if (state->next_token2.type == pCOLON) {
          parse_optional_keyword(state, params);
          parse_keywords(state, params);
          return;
        }
      }

      VALUE param = parse_function_param(state);
      rb_ary_push(params->optional_positionals, param);

      if (state->next_token.type == pCOMMA) {
        parser_advance(state);
      }
    } else {
      parse_keywords(state, params);
      return;
    }
  }
}

/*
  required_params ::= {<>} `)`
                    | {} `*` <function_param> `)`
                    | {} `*` function_params `,` <trailing_params>
                    | {} `**` <keywords>
                    | {} `?` <optional_params>
                    | {} function_param `,` <required_params>
                    | {} function_param <required_params>      (FIXME)
*/
static void parse_required_params(parserstate *state, method_params *params) {
  while (true) {
    if (state->next_token.type == pRPAREN) {
      return;
    }

    if (state->next_token.type == pSTAR) {
      parser_advance(state);
      params->rest_positionals = parse_function_param(state);

      if (state->next_token.type == pRPAREN) {
        return;
      } else {
        parser_advance_assert(state, pCOMMA);
        parse_trailing_params(state, params);
        return;
      }
    }

    if (state->next_token.type == pSTAR2) {
      parse_keywords(state, params);
      return;
    }

    if (state->next_token.type == pQUESTION) {
      parse_optional_params(state, params);
      return;
    }

    VALUE param = parse_function_param(state);
    rb_ary_push(params->required_positionals, param);

    if (state->next_token.type == pCOMMA) {
      parser_advance(state);
    }
  }
}

/*
  params ::= {} <required_params>
*/
static void parse_params(parserstate *state, method_params *params) {
  parse_required_params(state, params);
}

/*
  optinal ::= {} <simple_type>
            | {} simple_type <`?`>
*/
static VALUE parse_optional(parserstate *state) {
  VALUE type = parse_simple(state);

  if (state->next_token.type == pQUESTION) {
    parser_advance(state);
    return rbs_optional(type);
  } else {
    return type;
  }
}

static void initialize_method_params(method_params *params){
  params->required_positionals = rb_ary_new();
  params->optional_positionals = rb_ary_new();
  params->rest_positionals = Qnil;
  params->trailing_positionals = rb_ary_new();
  params->required_keywords = rb_hash_new();
  params->optional_keywords = rb_hash_new();
  params->rest_keywords = Qnil;
}

/*
  function ::= {} `(` params `)` `{` `(` params `)` `->` optional `}` `->` <optional>
             | {} `(` params `)` `->` <optional>
             | {} `{` `(` params `)` `->` optional `}` `->` <optional>
             | {} `{` `->` optional `}` `->` <optional>
             | {} `->` <optional>
*/
static void parse_function(parserstate *state, VALUE *function, VALUE *block) {
  method_params params;
  initialize_method_params(&params);

  if (state->next_token.type == pLPAREN) {
    parser_advance(state);
    parse_params(state, &params);
    parser_advance_assert(state, pRPAREN);
  }

  VALUE required = Qtrue;
  if (state->next_token.type == pQUESTION && state->next_token.type == pLBRACE) {
    // Optional block
    required = Qfalse;
    parser_advance(state);
  }
  if (state->next_token.type == pLBRACE) {
    parser_advance(state);

    method_params block_params;
    initialize_method_params(&block_params);

    if (state->next_token.type == pLPAREN) {
      parser_advance(state);
      parse_params(state, &block_params);
      parser_advance_assert(state, pRPAREN);
    }

    parser_advance_assert(state, pARROW);
    VALUE block_return_type = parse_optional(state);

    *block = rbs_block(
      rbs_function(
        block_params.required_positionals,
        block_params.optional_positionals,
        block_params.rest_positionals,
        block_params.trailing_positionals,
        block_params.required_keywords,
        block_params.optional_keywords,
        block_params.rest_keywords,
        block_return_type
      ),
      required
    );

    parser_advance_assert(state, pRBRACE);
  }

  parser_advance_assert(state, pARROW);
  VALUE type = parse_optional(state);

  *function = rbs_function(
    params.required_positionals,
    params.optional_positionals,
    params.rest_positionals,
    params.trailing_positionals,
    params.required_keywords,
    params.optional_keywords,
    params.rest_keywords,
    type
  );
}

/*
  proc_type ::= {`^`} <function>
*/
static VALUE parse_proc_type(parserstate *state) {
  position start = state->current_token.range.start;
  VALUE function = Qnil;
  VALUE block = Qnil;
  parse_function(state, &function, &block);
  position end = state->current_token.range.end;
  VALUE loc = rbs_location_pp(state->buffer, &start, &end);

  return rbs_proc(function, block, loc);
}

/**
 * ... `{` ... `}` ...
 *        >   >
 * */
/*
  record_attributes ::= {`{`} record_attribute... <record_attribute> `}`

  record_attribute ::= {} keyword_token `:` <type>
                     | {} literal_type `=>` <type>
*/
VALUE parse_record_attributes(parserstate *state) {
  VALUE hash = rb_hash_new();

  while (true) {
    VALUE key;
    VALUE type;

    if (is_keyword_token(state->next_token.type) && state->next_token2.type == pCOLON) {
      // { foo: type } syntax
      parser_advance(state);
      key = ID2SYM(INTERN_TOKEN(state, state->current_token));
      parser_advance_assert(state, pCOLON);
    } else {
      // { key => type } syntax
      switch (state->next_token.type)
      {
      case tSYMBOL:
      case tSQSTRING:
      case tDQSTRING:
      case tINTEGER:
      case kTRUE:
      case kFALSE:
        key = rb_funcall(parse_type(state), rb_intern("literal"), 0);
        break;
      default:
        rb_raise(rb_eRuntimeError, "Unexpected");
      }

      parser_advance_assert(state, pFATARROW);
    }

    type = parse_type(state);

    rb_hash_aset(hash, key, type);

    if (state->next_token.type == pCOMMA) {
      parser_advance(state);
      if (state->next_token.type == pRBRACE) {
        break;
      }
    } else {
      break;
    }
  }

  return hash;
}

/*
  symbol ::= {<tSYMBOL>}
*/
static VALUE parse_symbol(parserstate *state) {
  VALUE string = state->lexstate->string;
  rb_encoding *enc = rb_enc_get(string);

  int offset_bytes = rb_enc_codelen(':', enc);
  int bytes = token_bytes(state->current_token) - offset_bytes;

  unsigned int first_char = rb_enc_mbc_to_codepoint(
    RSTRING_PTR(string) + state->current_token.range.start.byte_pos + offset_bytes,
    RSTRING_END(string),
    enc
  );

  VALUE literal;

  if (first_char == '"' || first_char == '\'') {
    int bs = rb_enc_codelen(first_char, enc);
    offset_bytes += bs;
    bytes -= 2 * bs;

    char *buffer = peek_token(state->lexstate, state->current_token);
    VALUE string = rb_enc_str_new(buffer+offset_bytes, bytes, enc);

    if (first_char == '\"') {
      rbs_unescape_string(string);
    }
    literal = rb_funcall(string, rb_intern("to_sym"), 0);
  } else {
    char *buffer = peek_token(state->lexstate, state->current_token);
    literal = ID2SYM(rb_intern3(buffer+offset_bytes, bytes, enc));
  }

  return rbs_literal(
    literal,
    rbs_location_current_token(state)
  );
}

/*
  simple ::= {} `(`) type <`)`>
           | {} <base type>
           | {} <type_name>
           | {} class_instance `[` type_list <`]`>
           | {} `singleton` `(` type_name <`)`>
           | {} `[` type_list <`]`>
           | {} `{` record_attributes <`}`>
           | {} `^` <function>
*/
static VALUE parse_simple(parserstate *state) {
  parser_advance(state);

  switch (state->current_token.type) {
  case pLPAREN: {
    VALUE type = parse_type(state);
    parser_advance_assert(state, pRPAREN);
    return type;
  }
  case kBOOL:
    return rbs_base_type(RBS_Types_Bases_Bool, rbs_location_current_token(state));
  case kBOT:
    return rbs_base_type(RBS_Types_Bases_Bottom, rbs_location_current_token(state));
  case kCLASS:
    return rbs_base_type(RBS_Types_Bases_Class, rbs_location_current_token(state));
  case kINSTANCE:
    return rbs_base_type(RBS_Types_Bases_Instance, rbs_location_current_token(state));
  case kNIL:
    return rbs_base_type(RBS_Types_Bases_Nil, rbs_location_current_token(state));
  case kSELF:
    return rbs_base_type(RBS_Types_Bases_Self, rbs_location_current_token(state));
  case kTOP:
    return rbs_base_type(RBS_Types_Bases_Top, rbs_location_current_token(state));
  case kVOID:
    return rbs_base_type(RBS_Types_Bases_Void, rbs_location_current_token(state));
  case tINTEGER: {
    VALUE literal = rb_funcall(
      string_of_loc(state, state->current_token.range.start, state->current_token.range.end),
      rb_intern("to_i"),
      0
    );
    return rbs_literal(
      literal,
      rbs_location_current_token(state)
    );
  }
  case kTRUE:
    return rbs_literal(Qtrue, rbs_location_current_token(state));
  case kFALSE:
    return rbs_literal(Qfalse, rbs_location_current_token(state));
  case tSQSTRING: {
    VALUE literal = string_of_bytepos(state, state->current_token.range.start.byte_pos + 1, state->current_token.range.end.byte_pos - 1);
    return rbs_literal(
      literal,
      rbs_location_current_token(state)
    );
  }
  case tDQSTRING: {
    VALUE literal = string_of_loc(state, state->current_token.range.start, state->current_token.range.end);
    rbs_unescape_string(literal);
    return rbs_literal(
      literal,
      rbs_location_current_token(state)
    );
  }
  case tSYMBOL: {
    return parse_symbol(state);
  }
  case tUIDENT: {
    ID name = INTERN_TOKEN(state, state->current_token);
    if (parser_id_member(state, name)) {
      return rbs_variable(ID2SYM(name), rbs_location_current_token(state));
    }
    // fallthrough for type name
  }
  case tULIDENT:
    // fallthrough
  case pCOLON2: {
    range name_range;
    range args_range;
    range type_range;

    name_range.start = state->current_token.range.start;
    VALUE typename = parse_type_name(state, INTERFACE_NAME | CLASS_NAME | ALIAS_NAME);
    name_range.end = state->current_token.range.end;
    VALUE types = rb_ary_new();

    TypeNameKind kind;
    if (state->current_token.type == tUIDENT) {
      kind = CLASS_NAME;
    } else if (state->current_token.type == tULIDENT) {
      kind = INTERFACE_NAME;
    } else if (state->current_token.type == tLIDENT) {
      kind = ALIAS_NAME;
    } else {
      rb_raise(rb_eRuntimeError, "Unknown type name");
    }

    if (state->next_token.type == pLBRACKET) {
      parser_advance(state);
      args_range.start = state->current_token.range.start;
      rb_ary_push(types, parse_type(state));
      if (state->next_token.type == pCOMMA) parser_advance(state);
      parse_type_list(state, pRBRACKET, types);
      parser_advance_assert(state, pRBRACKET);
      args_range.end = state->current_token.range.end;
    } else {
      args_range = NULL_RANGE;
    }

    type_range.start = name_range.start;
    type_range.end = nonnull_pos_or(args_range.end, name_range.end);

    VALUE location = rbs_new_location(state->buffer, type_range);
    rbs_loc *loc = check_location(location);
    rbs_loc_add_required_child(loc, rb_intern("name"), name_range);
    rbs_loc_add_optional_child(loc, rb_intern("args"), args_range);

    if (kind == CLASS_NAME) {
      return rbs_class_instance(typename, types, location);
    } else if (kind == INTERFACE_NAME) {
      return rbs_interface(typename, types, location);
    } else if (kind == ALIAS_NAME) {
      return rbs_alias(typename, location);
    } else {
      return Qnil;
    }
  }
  case tLIDENT: {
    VALUE location = rbs_location_current_token(state);
    VALUE typename = parse_type_name(state, ALIAS_NAME);
    return rbs_alias(typename, location);
  }
  case kSINGLETON: {
    range name_range;
    range type_range;

    type_range.start = state->current_token.range.start;
    parser_advance_assert(state, pLPAREN);
    parser_advance(state);

    name_range.start = state->current_token.range.start;
    VALUE typename = parse_type_name(state, CLASS_NAME);
    name_range.end = state->current_token.range.end;

    parser_advance_assert(state, pRPAREN);
    type_range.end = state->current_token.range.end;

    VALUE location = rbs_new_location(state->buffer, type_range);
    rbs_loc *loc = check_location(location);
    rbs_loc_add_required_child(loc, rb_intern("name"), name_range);

    return rbs_class_singleton(typename, location);
  }
  case pLBRACKET: {
    VALUE types = parse_type_list(state, pRBRACKET, rb_ary_new());
    parser_advance_assert(state, pRBRACKET);
    return rbs_tuple(types);
  }
  case pLBRACE: {
    position start = state->current_token.range.start;
    VALUE fields = parse_record_attributes(state);
    parser_advance_assert(state, pRBRACE);
    position end = state->current_token.range.end;
    VALUE location = rbs_location_pp(state->buffer, &start, &end);
    return rbs_record(fields, location);
  }
  case pHAT: {
    return parse_proc_type(state);
  }
  default:
    rb_raise(
      rb_eRuntimeError,
      "Parse error in parse_simple: %s",
      token_type_str(state->current_token.type)
    );
  }
}

/*
  intersection ::= {} optional `&` ... '&' <optional>
                 | {} <optional>
*/
static VALUE parse_intersection(parserstate *state) {
  VALUE type = parse_optional(state);
  VALUE intersection_types = rb_ary_new();

  rb_ary_push(intersection_types, type);
  while (state->next_token.type == pAMP) {
    parser_advance(state);
    rb_ary_push(intersection_types, parse_optional(state));
  }

  if (rb_array_len(intersection_types) > 1) {
    type = rbs_intersection(intersection_types);
  }

  return type;
}

/*
  union ::= {} intersection '|' ... '|' <intersection>
          | {} <intersection>
*/
VALUE parse_type(parserstate *state) {
  VALUE type = parse_intersection(state);
  VALUE union_types = rb_ary_new();

  rb_ary_push(union_types, type);
  while (state->next_token.type == pBAR) {
    parser_advance(state);
    rb_ary_push(union_types, parse_intersection(state));
  }

  if (rb_array_len(union_types) > 1) {
    type = rbs_union(union_types);
  }

  return type;
}

/*
  method_type ::= {} `[` type_vars `]` <function>
                | {} <function>
*/
VALUE parse_method_type(parserstate *state) {
  VALUE function = Qnil;
  VALUE block = Qnil;
  id_table *table = parser_push_table(state);

  position start = state->next_token.range.start;

  if (state->next_token.type == pLBRACKET) {
    parser_advance(state);

    while (true) {
      parser_advance_assert(state, tUIDENT);
      ID name = INTERN_TOKEN(state, state->current_token);
      parser_insert_id(state, name);

      if (state->next_token.type == pCOMMA) {
        parser_advance(state);
        if (state->next_token.type == pRBRACKET) {
          break;
        }
      } else {
        break;
      }
    }

    parser_advance_assert(state, pRBRACKET);
  }

  parse_function(state, &function, &block);

  position end = state->current_token.range.end;

  VALUE type_params = rb_ary_new();
  for (size_t i = 0; i < table->count; i++) {
    rb_ary_push(type_params, ID2SYM(table->ids[i]));
  }

  parser_pop_table(state);

  return rbs_method_type(
    type_params,
    function,
    block,
    rbs_location_pp(state->buffer, &start, &end)
  );
}

/*
  global_decl ::= {tGIDENT} `:` <type>
*/
VALUE parse_global_decl(parserstate *state) {
  position start = state->current_token.range.start;
  VALUE typename = ID2SYM(INTERN_TOKEN(state, state->current_token));

  parser_advance_assert(state, pCOLON);

  VALUE type = parse_type(state);
  position end = state->current_token.range.end;

  return rbs_ast_decl_global(
    typename,
    type,
    rbs_location_pp(state->buffer, &start, &end),
    get_comment(state, start.line)
  );
}

/*
  const_decl ::= {const_name} `:` <type>
*/
VALUE parse_const_decl(parserstate *state) {
  position start = state->current_token.range.start;
  VALUE typename = parse_type_name(state, CLASS_NAME);

  parser_advance_assert(state, pCOLON);

  VALUE type = parse_type(state);
  position end = state->current_token.range.end;

  return rbs_ast_decl_constant(
    typename,
    type,
    rbs_location_pp(state->buffer, &start, &end),
    get_comment(state, start.line)
  );
}

/*
  type_decl ::= {kTYPE} alias_name `=` <type>
*/
VALUE parse_type_decl(parserstate *state, position comment_pos, VALUE annotations) {
  position start = state->current_token.range.start;
  comment_pos = nonnull_pos_or(comment_pos, start);

  parser_advance(state);
  VALUE typename = parse_type_name(state, ALIAS_NAME);

  parser_advance_assert(state, pEQ);

  VALUE type = parse_type(state);
  position end = state->current_token.range.end;

  return rbs_ast_decl_alias(
    typename,
    type,
    annotations,
    rbs_location_pp(state->buffer, &start, &end),
    get_comment(state, comment_pos.line)
  );
}

/*
  annotation ::= {<tANNOTATION>}
*/
VALUE parse_annotation(parserstate *state) {
  VALUE content = rb_funcall(state->buffer, rb_intern("content"), 0);
  rb_encoding *enc = rb_enc_get(content);

  char *p = peek_token(state->lexstate, state->current_token);
  int bytes = state->current_token.range.end.byte_pos - state->current_token.range.start.byte_pos;

  VALUE string = rb_enc_str_new(p, bytes, enc);
  VALUE location = rbs_location_current_token(state);

  return rbs_ast_annotation(string, location);
}

/*
  module_type_params ::= {} `[` module_type_param `,` ... <`]`>
                       | {<>}

  module_type_param ::= kUNCHECKED? (kIN|kOUT|) tUIDENT
*/
VALUE parse_module_type_params(parserstate *state) {
  VALUE params = rbs_ast_decl_module_type_params();

  if (state->next_token.type == pLBRACKET) {
    parser_advance(state);

    while (true) {
      VALUE name;
      VALUE unchecked = Qfalse;
      VALUE variance = ID2SYM(rb_intern("invariant"));
      position start = NullPosition;

      if (state->next_token.type == kUNCHECKED) {
        unchecked = Qtrue;
        parser_advance(state);
        start = state->current_token.range.start;
      }

      if (state->next_token.type == kIN || state->next_token.type == kOUT) {
        switch (state->next_token.type) {
        case kIN:
          variance = ID2SYM(rb_intern("contravariant"));
          break;
        case kOUT:
          variance = ID2SYM(rb_intern("covariant"));
          break;
        default:
          rb_raise(rb_eRuntimeError, "Unexpected");
        }

        parser_advance(state);
        if (null_position_p(start)) {
          start = state->current_token.range.start;
        }
      }

      parser_advance_assert(state, tUIDENT);
      if (null_position_p(start)) {
        start = state->current_token.range.start;
      }

      ID id = INTERN_TOKEN(state, state->current_token);
      name = ID2SYM(id);

      parser_insert_id(state, id);

      VALUE loc = rbs_location_pp(state->buffer, &start, &state->current_token.range.end);
      VALUE param = rbs_ast_decl_module_type_params_param(name, variance, unchecked, loc);
      rb_funcall(params, rb_intern("add"), 1, param);

      if (state->next_token.type == pCOMMA) {
        parser_advance(state);
      }

      if (state->next_token.type == pRBRACKET) {
        break;
      }
    }

    parser_advance_assert(state, pRBRACKET);
  }

  return params;
}

/*
  annotations ::= {} annotation ... <annotation>
                | {<>}
*/
void parse_annotations(parserstate *state, VALUE annotations, position *annot_pos) {
  *annot_pos = NullPosition;

  while (true) {
    if (state->next_token.type == tANNOTATION) {
      parser_advance(state);

      if (null_position_p((*annot_pos))) {
        *annot_pos = state->current_token.range.start;
      }

      rb_ary_push(annotations, parse_annotation(state));
    } else {
      break;
    }
  }
}

/*
  method_name ::= {} <IDENT | keyword>
                | {} (IDENT | keyword)~<`?`>
*/
VALUE parse_method_name(parserstate *state, range *range) {
  parser_advance(state);

  switch (state->current_token.type)
  {
  case tUIDENT:
  case tLIDENT:
  case tULIDENT:
  KEYWORD_CASES
    if (state->next_token.type == pQUESTION && state->current_token.range.end.byte_pos == state->next_token.range.start.byte_pos) {
      range->start = state->current_token.range.start;
      range->end = state->next_token.range.end;
      parser_advance(state);

      ID id = rb_intern3(
        RSTRING_PTR(state->lexstate->string) + range->start.byte_pos,
        range->end.byte_pos - range->start.byte_pos,
        rb_enc_get(state->lexstate->string)
      );

      return ID2SYM(id);
    } else {
      *range = state->current_token.range;
      return ID2SYM(INTERN_TOKEN(state, state->current_token));
    }

  case tBANGIDENT:
  case tEQIDENT:
  case tQIDENT:
    *range = state->current_token.range;
    return ID2SYM(INTERN_TOKEN(state, state->current_token));

  default:
    raise_syntax_error_e(
      state,
      state->current_token,
      "method name"
    );
  }
}

typedef enum {
  INSTANCE_KIND,
  SINGLETON_KIND,
  INSTANCE_SINGLETON_KIND
} InstanceSingletonKind;

/*
  instance_singleton_kind ::= {<>}
                            | {} kSELF <`.`>
                            | {} kSELF~`?` <`.`>
*/
InstanceSingletonKind parse_instance_singleton_kind(parserstate *state, range *rg) {
  InstanceSingletonKind kind = INSTANCE_KIND;

  if (state->next_token.type == kSELF) {
    range self_range = state->next_token.range;
    parser_advance(state);

    if (state->next_token.type == pDOT) {
      parser_advance(state);
      kind = SINGLETON_KIND;
      rg->start = self_range.start;
      rg->end = state->current_token.range.end;
    } else if (
      state->next_token.type == pQUESTION
    && state->next_token.range.end.char_pos == state->next_token2.range.start.char_pos
    && state->next_token2.type == pDOT) {
      parser_advance(state);
      parser_advance(state);
      kind = INSTANCE_SINGLETON_KIND;
      rg->start = self_range.start;
      rg->end = state->current_token.range.end;
    } else {
      raise_syntax_error_e(state, state->next_token, "instance/singleton kind, `self.` or `self?.`");
    }
  } else {
    *rg = NULL_RANGE;
  }

  return kind;
}

/**
 * def_member ::= {kDEF} method_name `:` <method_types>
 *
 * method_types ::= {} <method_type>
 *                | {} <`...`>
 *                | {} method_type `|` <method_types>
 *
 * @param instance_only `true` to reject singleton method definition.
 * @param accept_overload `true` to accept overloading (...) definition.
 * */
VALUE parse_member_def(parserstate *state, bool instance_only, bool accept_overload, position comment_pos, VALUE annotations) {
  range member_range;
  range keyword_range;
  range name_range;
  range kind_range;
  range overload_range = NULL_RANGE;

  keyword_range = state->current_token.range;
  member_range.start = keyword_range.start;

  comment_pos = nonnull_pos_or(comment_pos, keyword_range.start);
  VALUE comment = get_comment(state, comment_pos.line);

  InstanceSingletonKind kind;
  if (instance_only) {
    kind_range = NULL_RANGE;
    kind = INSTANCE_KIND;
  } else {
    kind = parse_instance_singleton_kind(state, &kind_range);
  }

  VALUE name = parse_method_name(state, &name_range);
  VALUE method_types = rb_ary_new();
  VALUE overload = Qfalse;

  parser_advance_assert(state, pCOLON);

  bool loop = true;
  while (loop) {
    switch (state->next_token.type) {
    case pLPAREN:
    case pARROW:
    case pLBRACE:
    case pLBRACKET:
      rb_ary_push(method_types, parse_method_type(state));
      member_range.end = state->current_token.range.end;
      break;

    case pDOT3:
      if (accept_overload) {
        overload = Qtrue;
        parser_advance(state);
        loop = false;
        overload_range = state->current_token.range;
        break;
      } else {
        raise_syntax_error_e(
          state,
          state->next_token,
          "method definition without `...`"
        );
      }

    default:
      raise_syntax_error_e(
        state,
        state->next_token,
        "method type or `...`"
      );
    }

    if (state->next_token.type == pBAR) {
      parser_advance(state);
    } else {
      loop = false;
    }
  }

  VALUE k;
  switch (kind) {
  case INSTANCE_KIND:
    k = ID2SYM(rb_intern("instance"));
    break;
  case SINGLETON_KIND:
    k = ID2SYM(rb_intern("singleton"));
    break;
  case INSTANCE_SINGLETON_KIND:
    k = ID2SYM(rb_intern("singleton_instance"));
    break;
  }

  VALUE location = rbs_new_location(state->buffer, member_range);
  rbs_loc *loc = check_location(location);
  rbs_loc_add_required_child(loc, rb_intern("keyword"), keyword_range);
  rbs_loc_add_required_child(loc, rb_intern("name"), name_range);
  rbs_loc_add_optional_child(loc, rb_intern("kind"), kind_range);
  rbs_loc_add_optional_child(loc, rb_intern("overload"), overload_range);

  return rbs_ast_members_method_definition(
    name,
    k,
    method_types,
    annotations,
    location,
    comment,
    overload
  );
}

/**
 * class_instance_name ::= {} <class_name>
 *                       | {} class_name `[` type args <`]`>
 *
 * @param kind
 * */
void class_instance_name(parserstate *state, TypeNameKind kind, VALUE *name, VALUE args, range *name_range, range *args_range) {
  parser_advance(state);

  *name_range = state->current_token.range;
  *name = parse_type_name(state, kind);

  if (state->next_token.type == pLBRACKET) {
    parser_advance(state);
    args_range->start = state->current_token.range.start;
    parse_type_list(state, pRBRACKET, args);
    parser_advance_assert(state, pRBRACKET);
    args_range->end = state->current_token.range.end;
  } else {
    *args_range = NULL_RANGE;
  }
}

/**
 *  mixin_member ::= {kINCLUDE} <class_instance_name>
 *                 | {kPREPEND} <class_instance_name>
 *                 | {kEXTEND} <class_instance_name>
 *
 * @param from_interface `true` when the member is in an interface.
 * */
VALUE parse_mixin_member(parserstate *state, bool from_interface, position comment_pos, VALUE annotations) {
  range member_range;
  range name_range;
  range keyword_range;
  range args_range = NULL_RANGE;

  member_range.start = state->current_token.range.start;
  comment_pos = nonnull_pos_or(comment_pos, member_range.start);

  keyword_range = state->current_token.range;

  VALUE klass = Qnil;
  switch (state->current_token.type)
  {
  case kINCLUDE:
    klass = RBS_AST_Members_Include;
    break;
  case kEXTEND:
    klass = RBS_AST_Members_Extend;
    break;
  case kPREPEND:
    klass = RBS_AST_Members_Prepend;
    break;
  default:
    rb_raise(rb_eRuntimeError, "Unexpected");
  }

  if (from_interface) {
    if (state->current_token.type != kINCLUDE) {
      raise_syntax_error_e(
        state,
        state->current_token,
        "include"
      );
    }
  }

  VALUE name;
  VALUE args = rb_ary_new();
  class_instance_name(
    state,
    from_interface ? INTERFACE_NAME : (INTERFACE_NAME | CLASS_NAME),
    &name, args, &name_range, &args_range
  );

  member_range.end = state->current_token.range.end;

  VALUE location = rbs_new_location(state->buffer, member_range);
  rbs_loc *loc = check_location(location);
  rbs_loc_add_required_child(loc, rb_intern("name"), name_range);
  rbs_loc_add_required_child(loc, rb_intern("keyword"), keyword_range);
  rbs_loc_add_optional_child(loc, rb_intern("args"), args_range);

  return rbs_ast_members_mixin(
    klass,
    name,
    args,
    annotations,
    location,
    get_comment(state, comment_pos.line)
  );
}

/**
 * @code
 * alias_member ::= {kALIAS} method_name <method_name>
 *                | {kALIAS} kSELF `.` method_name kSELF `.` <method_name>
 * @endcode
 *
 * @param[in] instance_only `true` to reject `self.` alias.
 * */
VALUE parse_alias_member(parserstate *state, bool instance_only, position comment_pos, VALUE annotations) {
  range member_range;
  range keyword_range, new_name_range, old_name_range;
  range new_kind_range, old_kind_range;

  member_range.start = state->current_token.range.start;
  keyword_range = state->current_token.range;

  comment_pos = nonnull_pos_or(comment_pos, member_range.start);
  VALUE comment = get_comment(state, comment_pos.line);

  VALUE new_name;
  VALUE old_name;
  VALUE kind;

  if (!instance_only && state->next_token.type == kSELF) {
    kind = ID2SYM(rb_intern("singleton"));

    new_kind_range.start = state->next_token.range.start;
    new_kind_range.end = state->next_token2.range.end;
    parser_advance_assert(state, kSELF);
    parser_advance_assert(state, pDOT);
    new_name = parse_method_name(state, &new_name_range);

    old_kind_range.start = state->next_token.range.start;
    old_kind_range.end = state->next_token2.range.end;
    parser_advance_assert(state, kSELF);
    parser_advance_assert(state, pDOT);
    old_name = parse_method_name(state, &old_name_range);
  } else {
    kind = ID2SYM(rb_intern("instance"));
    new_name = parse_method_name(state, &new_name_range);
    old_name = parse_method_name(state, &old_name_range);

    new_kind_range = NULL_RANGE;
    old_kind_range = NULL_RANGE;
  }

  member_range.end = state->current_token.range.end;
  VALUE location = rbs_new_location(state->buffer, member_range);
  rbs_loc *loc = check_location(location);
  rbs_loc_add_required_child(loc, rb_intern("keyword"), keyword_range);
  rbs_loc_add_required_child(loc, rb_intern("new_name"), new_name_range);
  rbs_loc_add_required_child(loc, rb_intern("old_name"), old_name_range);
  rbs_loc_add_optional_child(loc, rb_intern("new_kind"), new_kind_range);
  rbs_loc_add_optional_child(loc, rb_intern("old_kind"), old_kind_range);

  return rbs_ast_members_alias(
    new_name,
    old_name,
    kind,
    annotations,
    location,
    comment
  );
}

/*
  interface_members ::= {} ...<interface_member> kEND

  interface_member ::= def_member
*/
VALUE parse_interface_members(parserstate *state) {
  VALUE members = rb_ary_new();

  while (state->next_token.type != kEND) {
    VALUE annotations = rb_ary_new();
    position annot_pos = NullPosition;

    parse_annotations(state, annotations, &annot_pos);

    parser_advance(state);

    VALUE member;
    switch (state->current_token.type) {
    case kDEF:
      member = parse_member_def(state, true, true, annot_pos, annotations);
      break;

    case kINCLUDE:
    case kEXTEND:
    case kPREPEND:
      member = parse_mixin_member(state, true, annot_pos, annotations);
      break;

    case kALIAS:
      member = parse_alias_member(state, true, annot_pos, annotations);
      break;

    default:
      raise_syntax_error_e(
        state,
        state->current_token,
        "interface member"
      );
    }

    rb_ary_push(members, member);
  }

  return members;
}

/*
  interface_decl ::= {`interface`} interface_name module_type_params interface_members <kEND>
*/
VALUE parse_interface_decl(parserstate *state, position comment_pos, VALUE annotations) {
  position start = state->current_token.range.start;
  comment_pos = nonnull_pos_or(comment_pos, start);

  parser_push_table(state);
  parser_advance(state);

  VALUE name = parse_type_name(state, INTERFACE_NAME);
  VALUE params = parse_module_type_params(state);
  VALUE members = parse_interface_members(state);

  parser_advance_assert(state, kEND);

  position end = state->current_token.range.end;
  parser_pop_table(state);

  return rbs_ast_decl_interface(
    name,
    params,
    members,
    annotations,
    rbs_location_pp(state->buffer, &start, &end),
    get_comment(state, comment_pos.line)
  );
}

VALUE parse_decl(parserstate *state) {
  VALUE annotations = rb_ary_new();
  position annot_pos = NullPosition;

  parse_annotations(state, annotations, &annot_pos);

  parser_advance(state);
  switch (state->current_token.type)
  {
  case tUIDENT:
  case pCOLON2:
    return parse_const_decl(state);
  case tGIDENT:
    return parse_global_decl(state);
  case kTYPE:
    return parse_type_decl(state, annot_pos, annotations);
  case kINTERFACE:
    return parse_interface_decl(state, annot_pos, annotations);
  default:
    raise_syntax_error_e(
      state,
      state->current_token,
      "declaration"
    );
  }
}

VALUE parse_signature(parserstate *state) {
  VALUE decls = rb_ary_new();

  while (state->next_token.type != pEOF) {
    rb_ary_push(decls, parse_decl(state));
  }

  return decls;
}
