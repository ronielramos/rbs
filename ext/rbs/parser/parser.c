#include "parser.h"

VALUE rbsparser_Keywords;

static VALUE RBSParser;

static ID id_RBS;
static VALUE RBS;

static VALUE sym_class;
static VALUE sym_alias;
static VALUE sym_interface;

static const char *names[] = {
  "NullType",
  "pLPAREN",
  "pRPAREN",
  "pCOLON",
  "pCOLON2",
  "pLBRACKET",
  "pRBRACKET",
  "pLBRACE",
  "pRBRACE",
  "pHAT",
  "pARROW",
  "pFATARROW",
  "pCOMMA",
  "pBAR",
  "pAMP",
  "pSTAR",
  "pSTAR2",
  "pDOT",
  "pDOT3",
  "pMINUS",
  "pPLUS",
  "pSLASH",
  "pEQ2",
  "pEQ3",
  "pEQT",
  "pBANG",
  "pQUESTION",

  "kSINGLETON",
  "kSELF",
  "kINSTANCE",

  "tLIDENT",          /* Identifiers starting with lower case */
  "tUIDENT",          /* Identifiers starting with upper case */
  "tULIDENT",         /* Identifiers starting with `_` */
  "tGIDENT",          /* Identifiers starting with `$` */
  "tAIDENT",          /* Identifiers starting with `@` */
  "tA2IDENT",         /* Identifiers starting with `@@` */
  "tBANGIDENT",
  "tEQIDENT",
};

#define INTERN_TOKEN(parserstate, tok) rb_intern2(peek_token(parserstate->lexstate, tok), token_bytes(tok))

static void print_token(token tok) {
  printf("%s char=%d...%d\n", names[tok.type], tok.start.char_pos, tok.end.char_pos);
}

static void parser_advance(parserstate *state) {
  state->current_token = state->next_token;
  state->next_token = rbsparser_next_token(state->lexstate);
}

static void parser_advance_assert(parserstate *state, enum TokenType type) {
  parser_advance(state);
  if (state->current_token.type != type) {
    print_token(state->current_token);
    rb_raise(rb_eRuntimeError, "Unexpected token");
  }
}

static void __attribute__((noreturn)) raise_syntax_error() {
  rb_raise(rb_eRuntimeError, "Syntax error");
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

static VALUE parse_simple(parserstate *state) {
  parser_advance(state);

  switch (state->current_token.type) {
  case pLPAREN: {
    VALUE type = parse_type(state);
    parser_advance_assert(state, pRPAREN);
    return type;
  }
  case kSELF:
    return rbs_self_type(
      rbs_location_tok(state->buffer, &state->current_token)
    );
  case tUIDENT:
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
  default:
    rb_raise(rb_eRuntimeError, "Parse error (parse_type)");
  }
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

static VALUE
rbsparser_parse_type(VALUE self, VALUE buffer, VALUE line, VALUE column)
{
  VALUE string = rb_funcall(buffer, rb_intern("content"), 0);
  lexstate lex = { string };
  lex.current.line = NUM2INT(line);
  lex.current.column = NUM2INT(column);

  parserstate parser = { &lex };
  parser.buffer = buffer;

  parser_advance(&parser);
  return parse_type(&parser);
}

void
Init_parser(void)
{
  id_RBS = rb_intern_const("RBS");
  RBS = rb_const_get(rb_cObject, id_RBS);
  RBSParser = rb_define_class_under(RBS, "Parser", rb_cObject);
  RBS_AST = rb_const_get(RBS, rb_intern("AST"));
  RBS_Types = rb_const_get(RBS, rb_intern("Types"));
  RBS_Types_Bases = rb_const_get(RBS_Types, rb_intern("Bases"));
  RBS_Types_Bases_Self = rb_const_get(RBS_Types_Bases, rb_intern("Self"));
  RBS_Namespace = rb_const_get(RBS, rb_intern("Namespace"));
  RBS_TypeName = rb_const_get(RBS, rb_intern("TypeName"));
  RBS_Types_ClassInstance = rb_const_get(RBS_Types, rb_intern("ClassInstance"));
  RBS_Types_Alias = rb_const_get(RBS_Types, rb_intern("Alias"));
  RBS_Types_Interface = rb_const_get(RBS_Types, rb_intern("Interface"));
  RBS_Types_Union = rb_const_get(RBS_Types, rb_intern("Union"));
  RBS_Types_Intersection = rb_const_get(RBS_Types, rb_intern("Intersection"));
  RBS_Types_ClassSingleton = rb_const_get(RBS_Types, rb_intern("ClassSingleton"));
  RBS_Types_Tuple = rb_const_get(RBS_Types, rb_intern("Tuple"));
  RBS_Types_Optional = rb_const_get(RBS_Types, rb_intern("Optional"));
  RBS_Location = rb_const_get(RBS, rb_intern("Location"));

  sym_class = ID2SYM(rb_intern_const("class"));
  sym_interface = ID2SYM(rb_intern_const("interface"));
  sym_alias = ID2SYM(rb_intern_const("alias"));

  rbsparser_Keywords = rb_hash_new();
  rb_hash_aset(rbsparser_Keywords, rb_str_new_literal("singleton"), INT2FIX(kSINGLETON));
  rb_hash_aset(rbsparser_Keywords, rb_str_new_literal("self"), INT2FIX(kSELF));
  rb_hash_aset(rbsparser_Keywords, rb_str_new_literal("instance"), INT2FIX(kINSTANCE));
  rb_define_const(RBSParser, "Keywords", rbsparser_Keywords);

  rb_define_singleton_method(RBSParser, "_parse_type", rbsparser_parse_type, 3);
}
