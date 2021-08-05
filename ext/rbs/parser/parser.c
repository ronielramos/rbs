#include "parser.h"
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

#define INTERN_TOKEN(parserstate, tok) \
  rb_intern3(\
    peek_token(parserstate->lexstate, tok),\
    token_bytes(tok),\
    rb_enc_get(parserstate->lexstate->string)\
  )

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

static void __attribute__((noreturn)) raise_syntax_error() {
  rb_raise(rb_eRuntimeError, "Syntax error");
}

static void __attribute__((noreturn)) raise_syntax_error_e(parserstate *state, token tok, char *expected) {
  rb_raise(rb_eRuntimeError, "Syntax error at line %d char %d, expected %s, but got %s", tok.start.line, tok.start.column, expected, RBS_TOKENTYPE_NAMES[tok.type]);
}

VALUE parse_type_name(parserstate *state) {
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

  switch (state->current_token.type)
  {
  case tLIDENT:
  case tULIDENT:
  case tUIDENT:
    return rbs_type_name(namespace, ID2SYM(INTERN_TOKEN(state, state->current_token)));
    break;
  default:
    raise_syntax_error();
  }
}

VALUE parse_const_name(parserstate *state) {
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

  switch (state->current_token.type)
  {
  case tUIDENT:
    return rbs_type_name(namespace, ID2SYM(INTERN_TOKEN(state, state->current_token)));
    break;
  default:
    raise_syntax_error();
  }
}

/*
  ... name ...
          >
          >

  ... NAME `::` name ...
          >
                    >

  ... `::` NAME `::` name ...
          >
                         >

*/
VALUE parse_alias_name(parserstate *state) {
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

  switch (state->current_token.type)
  {
  case tLIDENT:
    return rbs_type_name(
      namespace,
      ID2SYM(INTERN_TOKEN(state, state->current_token))
    );
  default:
    raise_syntax_error_e(
      state,
      state->current_token,
      "type alias name (tLIDENT)"
    );
  }
}


/*
  ... `::` interface name ...
      ^^^^                           current (start)
                     ^^^^            current (end)

  ... UIDENT `::` UIDENT `::` lident ...
      ^^^^^^                                   current (start)
                              ^^^^^^           current (end)


*/
VALUE parse_interface_name(parserstate *state) {
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

  switch (state->current_token.type)
  {
  case tULIDENT:
    return rbs_type_name(
      namespace,
      ID2SYM(INTERN_TOKEN(state, state->current_token))
    );
  default:
    raise_syntax_error_e(
      state,
      state->current_token,
      "interface name (tULIDENT)"
    );
  }
}

static VALUE parse_type_list(parserstate *state, enum TokenType eol, VALUE types) {
  while (true) {
    if (state->next_token.type == eol) {
      break;
    }

    rb_ary_push(types, parse_type(state));

    if (state->next_token.type == pCOMMA) {
      parser_advance(state);
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

/**
 * ```
 * ... type name , ...
 *    >         >       before type => after name
 * ```
 *
 * ```
 *     ... type ) ...
 *        >    >            before type => after type
 * ```
 * */
static VALUE parse_function_param(parserstate *state) {
  position start = state->next_token.start;
  VALUE type = parse_type(state);

  if (state->next_token.type == pCOMMA || state->next_token.type == pRPAREN) {
    return rbs_function_param(
      type,
      Qnil,
      rb_funcall(type, rb_intern("location"), 0)
    );
  } else {
    parser_advance(state);
    VALUE name = ID2SYM(INTERN_TOKEN(state, state->current_token));
    VALUE location = rbs_location_pp(state->buffer, &start, &state->current_token.end);
    return rbs_function_param(type, name, location);
  }
}

/**
 * ```
 * ... keyword `:` type name , ...
 *    ^                              start (before keyword token)
 *                            ^      end   (after comma)
 * ```
 * */
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

/**
 * ... `?` keyword `:` type name , ...
 *        ^                              start (before keyword token)
 *                                ^      end   (after comma)
 * */
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

/**
 *   ... keyword `:` param, ... `)`
 *      >                      >
 *   ... `?` keyword `:` param, ... `)`
 *      >                          >
 *   ... `**` type param `)`
 *      >               >
 *   ...  `)`
 *      >>
 * */
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

/**
 * ... type name, ...
 *    >          >
 *
 * */
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

/**
 *
 * ... type name, type , ...
 *    >          >      >
 *
 *
 *
 * */
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

static void parse_params(parserstate *state, method_params *params) {
  parse_required_params(state, params);
}

/*
    simple_type
  | simple_type '?'
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

/**
 * ... `(` params `)` `->` Type ...
 *  ^^^                                 start
 *                         ^^^^         end
 *
 * */
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

/**
 *  ... `^` `(` ... type ...
 *         ^                    start
 *                      ^       end
 * */
static VALUE parse_proc_type(parserstate *state) {
  position start = state->current_token.start;
  VALUE function = Qnil;
  VALUE block = Qnil;
  parse_function(state, &function, &block);
  position end = state->current_token.end;
  VALUE loc = rbs_location_pp(state->buffer, &start, &end);

  return rbs_proc(function, block, loc);
}

/**
 * ... `{` ... `}` ...
 *        >   >
 * */
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

static VALUE parse_symbol(parserstate *state) {
  VALUE string = state->lexstate->string;
  rb_encoding *enc = rb_enc_get(string);

  int offset_bytes = rb_enc_codelen(':', enc);
  int bytes = token_bytes(state->current_token) - offset_bytes;

  unsigned int first_char = rb_enc_mbc_to_codepoint(
    RSTRING_PTR(string) + state->current_token.start.byte_pos + offset_bytes,
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
      string_of_loc(state, state->current_token.start, state->current_token.end),
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
    VALUE literal = string_of_bytepos(state, state->current_token.start.byte_pos + 1, state->current_token.end.byte_pos - 1);
    return rbs_literal(
      literal,
      rbs_location_current_token(state)
    );
  }
  case tDQSTRING: {
    VALUE literal = string_of_loc(state, state->current_token.start, state->current_token.end);
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
    VALUE typename = parse_type_name(state);
    VALUE types = rb_ary_new();

    VALUE kind = rb_ivar_get(typename, rb_intern_const("@kind"));

    if (state->next_token.type == pLBRACKET) {
      parser_advance(state);
      rb_ary_push(types, parse_type(state));
      if (state->next_token.type == pCOMMA) parser_advance(state);
      parse_type_list(state, pRBRACKET, types);
      parser_advance_assert(state, pRBRACKET);
    }

    if (kind == sym_class) {
      return rbs_class_instance(typename, types);
    } else if (kind == sym_interface) {
      return rbs_interface(typename, types);
    } else if (kind == sym_alias) {
      return rbs_alias(typename);
    } else {
      return Qnil;
    }
  }
  case tLIDENT: {
    VALUE typename = parse_type_name(state);
    return rbs_alias(typename);
  }
  case kSINGLETON: {
    parser_advance_assert(state, pLPAREN);
    parser_advance(state);
    VALUE typename = parse_type_name(state);
    parser_advance_assert(state, pRPAREN);

    VALUE kind = rb_ivar_get(typename, rb_intern_const("@kind"));
    if (kind != sym_class) {
      raise_syntax_error();
    }

    return rbs_class_singleton(typename);
  }
  case pLBRACKET: {
    VALUE types = parse_type_list(state, pRBRACKET, rb_ary_new());
    parser_advance_assert(state, pRBRACKET);
    return rbs_tuple(types);
  }
  case pLBRACE: {
    position start = state->current_token.start;
    VALUE fields = parse_record_attributes(state);
    parser_advance_assert(state, pRBRACE);
    position end = state->current_token.end;
    VALUE location = rbs_location_pp(state->buffer, &start, &end);
    return rbs_record(fields, location);
  }
  case pHAT: {
    return parse_proc_type(state);
  }
  default:
    rb_raise(rb_eRuntimeError, "Parse error in parse_simple: %s", RBS_TOKENTYPE_NAMES[state->current_token.type]);
  }
}

/*
    optional
  | optional '&' ... '&' optional
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
    intersection
  | intersection '|' ... '|' intersection
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
   ... tok `[` vars `]` function ...
       ^^^                                current(start)
                              ^^^         current(end)
*/
VALUE parse_method_type(parserstate *state) {
  VALUE function = Qnil;
  VALUE block = Qnil;
  id_table *table = parser_push_table(state);

  position start = state->next_token.start;

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

  position end = state->current_token.end;

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

VALUE parse_global_decl(parserstate *state) {
  position start = state->current_token.start;
  VALUE typename = ID2SYM(INTERN_TOKEN(state, state->current_token));

  parser_advance_assert(state, pCOLON);

  VALUE type = parse_type(state);
  position end = state->current_token.end;

  return rbs_ast_decl_global(
    typename,
    type,
    rbs_location_pp(state->buffer, &start, &end),
    get_comment(state, start.line)
  );
}

/**
 *
 * ... tUIDENT ... `:` TYPE ...
 *            >
 *                         >
 *
 * */
VALUE parse_const_decl(parserstate *state) {
  position start = state->current_token.start;
  VALUE typename = parse_const_name(state);

  parser_advance_assert(state, pCOLON);

  VALUE type = parse_type(state);
  position end = state->current_token.end;

  return rbs_ast_decl_constant(
    typename,
    type,
    rbs_location_pp(state->buffer, &start, &end),
    get_comment(state, start.line)
  );
}

VALUE parse_type_decl(parserstate *state, position comment_pos, VALUE annotations) {
  position start = state->current_token.start;
  comment_pos = nonnull_pos_or(comment_pos, start);

  parser_advance(state);
  VALUE typename = parse_alias_name(state);

  parser_advance_assert(state, pEQ);

  VALUE type = parse_type(state);
  position end = state->current_token.end;

  return rbs_ast_decl_alias(
    typename,
    type,
    annotations,
    rbs_location_pp(state->buffer, &start, &end),
    get_comment(state, comment_pos.line)
  );
}

VALUE parse_annotation(parserstate *state) {
  VALUE content = rb_funcall(state->buffer, rb_intern("content"), 0);
  rb_encoding *enc = rb_enc_get(content);

  char *p = peek_token(state->lexstate, state->current_token);
  int bytes = state->current_token.end.byte_pos - state->current_token.start.byte_pos;

  VALUE string = rb_enc_str_new(p, bytes, enc);
  VALUE location = rbs_location_current_token(state);

  return rbs_ast_annotation(string, location);
}

/*

  ... tok `[` params `]` ...
      ^^^                     current (start)
                     ^^^      current (end)

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
        start = state->current_token.start;
      }

      if (state->next_token.type == kIN || state->next_token.type == kOUT) {
        switch (state->next_token.type) {
        case kIN:
          variance = ID2SYM(rb_intern("contravariant"));
          break;
        case kOUT:
          variance = ID2SYM(rb_intern("covariant"));
          break;
        }

        parser_advance(state);
        if (null_position_p(start)) {
          start = state->current_token.start;
        }
      }

      parser_advance_assert(state, tUIDENT);
      if (null_position_p(start)) {
        start = state->current_token.start;
      }

      name = ID2SYM(INTERN_TOKEN(state, state->current_token));

      VALUE loc = rbs_location_pp(state->buffer, &start, &state->current_token.end);
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
  ... tok %a{abc def} %a(xyzzy) tok ...
      ^^^                                current (start)
                      ^^^^^^^^^          current (end)
*/
void parse_annotations(parserstate *state, VALUE annotations, position *annot_pos) {
  *annot_pos = NullPosition;

  while (true) {
    if (state->next_token.type == tANNOTATION) {
      parser_advance(state);

      if (null_position_p((*annot_pos))) {
        *annot_pos = state->current_token.start;
      }

      rb_ary_push(annotations, parse_annotation(state));
    } else {
      break;
    }
  }
}

/*
  ... tok ident `?`
      ^^^
                ^^^

  ... tok ident
      ^^^
          ^^^^^

  ... tok kDEF
      ^^^
          ^^^^

  ... tok opr
      ^^^
          ^^^
*/
VALUE parse_method_name(parserstate *state, position *start, position *end) {
  parser_advance(state);

  switch (state->current_token.type)
  {
  case tUIDENT:
  case tLIDENT:
  case tULIDENT:
  KEYWORD_CASES
    if (state->next_token.type == pQUESTION && state->current_token.end.byte_pos == state->next_token.start.byte_pos) {
      *start = state->current_token.start;
      *end = state->next_token.end;
      parser_advance(state);

      ID id = rb_intern3(
        RSTRING_PTR(state->lexstate->string) + start->byte_pos,
        end->byte_pos - start->byte_pos,
        rb_enc_get(state->lexstate->string)
      );

      return ID2SYM(id);
    } else {
      *start = state->current_token.start;
      *end = state->current_token.end;
      return ID2SYM(INTERN_TOKEN(state, state->current_token));
    }

  case tBANGIDENT:
  case tEQIDENT:
  case tQIDENT:
    *start = state->current_token.start;
    *end = state->current_token.end;
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
  ... tok `self` `.` ...
      ^^^
                 ^^^

  ... tok ...
      ^^^

  ... tok `self` `?` `.` ...
      ^^^
                     ^^^
*/
InstanceSingletonKind parse_instance_singleton_kind(parserstate *state) {
  InstanceSingletonKind kind = INSTANCE_KIND;

  if (state->next_token.type == kSELF) {
    parser_advance(state);

    if (state->next_token.type == pDOT) {
      parser_advance(state);
      kind = SINGLETON_KIND;
    } else if (
      state->next_token.type == pQUESTION
    && state->next_token.end.char_pos == state->next_token2.start.char_pos
    && state->next_token2.type == pDOT) {
      parser_advance(state);
      parser_advance(state);
      kind = INSTANCE_SINGLETON_KIND;
    } else {
      raise_syntax_error_e(state, state->next_token, "instance/singleton kind, `self.` or `self?.`");
    }
  }

  return kind;
}

/*
  ... `def` name `:` method_type ...
      ^^^^^                            current (start)
                             ^^^       current (end)
*/
VALUE parse_member_def(parserstate *state, position comment_pos, VALUE annotations) {
  position start = state->current_token.start;
  comment_pos = nonnull_pos_or(comment_pos, start);

  VALUE comment = get_comment(state, comment_pos.line);

  InstanceSingletonKind kind = parse_instance_singleton_kind(state);

  position name_start = NullPosition;
  position name_end = NullPosition;
  VALUE name = parse_method_name(state, &name_start, &name_end);
  VALUE method_types = rb_ary_new();
  VALUE overload = Qfalse;

  parser_advance_assert(state, pCOLON);

  bool loop = true;
  while (loop) {
    switch (state->next_token.type) {
    case pLPAREN:
    case pARROW:
    case pLBRACE:
      rb_ary_push(method_types, parse_method_type(state));
      break;

    case pDOT3:
      overload = Qtrue;
      parser_advance(state);
      loop = false;
      break;

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

  VALUE location = rbs_location_pp(state->buffer, &start, &state->current_token.end);

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
      member = parse_member_def(state, annot_pos, annotations);
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
  ... `interface` interface_name ... `end` ...
      ^^^^^^^^^^^                                current (start)
                                     ^^^^^       current (end)
*/
VALUE parse_interface_decl(parserstate *state, position comment_pos, VALUE annotations) {
  position start = state->current_token.start;
  comment_pos = nonnull_pos_or(comment_pos, start);

  parser_advance(state);

  VALUE name = parse_interface_name(state);
  VALUE params = parse_module_type_params(state);
  VALUE members = parse_interface_members(state);

  parser_advance_assert(state, kEND);

  position end = state->current_token.end;

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
