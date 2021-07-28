#include "parser.h"

static VALUE RBSParser;

static ID id_RBS;
static VALUE RBS;

static VALUE
rbs_parser_parse_type(VALUE self, VALUE string, VALUE file, VALUE line, VALUE column)
{
  lexstate state = { string };

  return string;
}

void
Init_parser(void)
{
  id_RBS = rb_intern_const("RBS");
  RBS = rb_const_get(rb_cObject, id_RBS);
  RBSParser = rb_define_class_under(RBS, "Parser", rb_cObject);

  rb_define_singleton_method(RBSParser, "parse_type", rbs_parser_parse_type, 4);
}
