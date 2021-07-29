#include "parser.h"

static VALUE RBSParser;

static ID id_RBS;
static VALUE RBS;

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

  "kSINGLETON",
  "kSELF",
  "kINSTANCE",

  "tLIDENT",          /* Identifiers starting with lower case */
  "tUIDENT",          /* Identifiers starting with upper case */
  "tULIDENT",         /* Identifiers starting with `_` */
  "tGIDENT",          /* Identifiers starting with `$` */
  "tAIDENT",          /* Identifiers starting with `@` */
  "tA2IDENT",         /* Identifiers starting with `@@` */
};

static void print_token(token tok) {
  printf("%s char=%d...%d\n", names[tok.type], tok.start.char_pos, tok.end.char_pos);
}

static void print_state(lexstate *state) {
  printf("state >> pos=%d, lasttoke=%s\n", state->current.char_pos, names[state->last_token.type]);
}

static VALUE
rbsparser_parse_type(VALUE self, VALUE string, VALUE file, VALUE line, VALUE column)
{
  lexstate state = { string };
  token tok = {NullType};

  tok = rbsparser_nextToken(&state, tok);
  print_state(&state);
  print_token(tok);

  tok = rbsparser_nextToken(&state, tok);
  print_state(&state);
  print_token(tok);

  tok = rbsparser_nextToken(&state, tok);
  print_state(&state);
  print_token(tok);

  return string;
}

void
Init_parser(void)
{
  id_RBS = rb_intern_const("RBS");
  RBS = rb_const_get(rb_cObject, id_RBS);
  RBSParser = rb_define_class_under(RBS, "Parser", rb_cObject);

  rb_define_singleton_method(RBSParser, "parse_type", rbsparser_parse_type, 4);
}
