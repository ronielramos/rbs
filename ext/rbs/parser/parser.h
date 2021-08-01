#include "ruby.h"
#include "ruby/re.h"
#include "ruby/encoding.h"

enum TokenType {
  NullType,         /* (Nothing) */
  pEOF,              /* EOF */

  pLPAREN,          /* ( */
  pRPAREN,          /* ) */
  pCOLON,           /* : */
  pCOLON2,          /* :: */
  pLBRACKET,        /* [ */
  pRBRACKET,        /* ] */
  pLBRACE,          /* { */
  pRBRACE,          /* } */
  pHAT,             /* ^ */
  pARROW,           /* -> */
  pFATARROW,        /* => */
  pCOMMA,           /* , */
  pBAR,             /* | */
  pAMP,             /* & */
  pSTAR,            /* * */
  pSTAR2,           /* ** */
  pDOT,             /* . */
  pDOT3,            /* ... */
  pMINUS,           /* - */
  pPLUS,            /* + */
  pSLASH,           /* / */
  pEQ2,             /* == */
  pEQ3,             /* === */
  pEQT,             /* =~ */
  pBANG,            /* ! */
  pQUESTION,        /* ? */

  kBOOL,            /* bool */
  kBOT,             /* bot */
  kCLASS,           /* class */
  kFALSE,           /* false */
  kINSTANCE,        /* instance */
  kINTERFACE,       /* interface */
  kNIL,             /* nil */
  kSELF,            /* self */
  kSINGLETON,       /* singleton */
  kTOP,             /* top */
  kTRUE,            /* true */
  kVOID,            /* void */

  tLIDENT,          /* Identifiers starting with lower case */
  tUIDENT,          /* Identifiers starting with upper case */
  tULIDENT,         /* Identifiers starting with `_` */
  tGIDENT,          /* Identifiers starting with `$` */
  tAIDENT,          /* Identifiers starting with `@` */
  tA2IDENT,         /* Identifiers starting with `@@` */
  tBANGIDENT,       /* Identifiers ending with `!` */
  tEQIDENT,         /* Identifiers ending with `=` */
  tQIDENT,          /* Quoted identifier */

  tCOMMENT,         /* Comment */
  tLINECOMMENT,     /* Comment of all line */

  tDQSTRING,        /* Double quoted string */
  tSQSTRING,        /* Single quoted string */
  tINTEGER,         /* Integer */
  tSYMBOL,          /* Symbol */
};

typedef struct {
  int byte_pos;
  int char_pos;
  int line;
  int column;
} position;

typedef struct {
  enum TokenType type;
  position start;
  position end;
} token;

typedef struct {
  VALUE string;
  position current;
  bool first_token_of_line;
} lexstate;

typedef struct {
  lexstate *lexstate;
  token current_token;
  token next_token;
  token next_token2;
  VALUE buffer;
} parserstate;

extern token NullToken;
extern position NullPosition;

token rbsparser_next_token(lexstate *state);

extern VALUE rbsparser_Keywords;

char *peek_token(lexstate *state, token tok);
int token_chars(token tok);
int token_bytes(token tok);

VALUE parse_type(parserstate *state);

void pp(VALUE object);

extern VALUE RBS_AST;
extern VALUE RBS_Types;
extern VALUE RBS_Types_Bases_Any;
extern VALUE RBS_Types_Bases_Bool;
extern VALUE RBS_Types_Bases_Bottom;
extern VALUE RBS_Types_Bases_Class;
extern VALUE RBS_Types_Bases_Instance;
extern VALUE RBS_Types_Bases_Nil;
extern VALUE RBS_Types_Bases_Self;
extern VALUE RBS_Types_Bases_Top;
extern VALUE RBS_Types_Bases_Void;
extern VALUE RBS_Types_Bases;
extern VALUE RBS_Types_ClassInstance;
extern VALUE RBS_Types_Alias;
extern VALUE RBS_Types_Interface;
extern VALUE RBS_Types_Union;
extern VALUE RBS_Types_Intersection;
extern VALUE RBS_Types_ClassSingleton;
extern VALUE RBS_Types_Tuple;
extern VALUE RBS_Types_Optional;
extern VALUE RBS_Types_Literal;
extern VALUE RBS_Namespace;
extern VALUE RBS_TypeName;
extern VALUE RBS_Location;
extern VALUE RBS_Types_Function;
extern VALUE RBS_Types_Function_Param;
extern VALUE RBS_Types_Block;
extern VALUE RBS_Types_Proc;

VALUE rbs_base_type(VALUE klass, VALUE location);
VALUE rbs_namespace(VALUE path, VALUE absolute);
VALUE rbs_type_name(VALUE namespace, VALUE name);
VALUE rbs_class_instance(VALUE typename, VALUE type_args);
VALUE rbs_class_singleton(VALUE typename);
VALUE rbs_alias(VALUE typename);
VALUE rbs_interface(VALUE typename, VALUE type_args);
VALUE rbs_union(VALUE types);
VALUE rbs_intersection(VALUE types);
VALUE rbs_tuple(VALUE types);
VALUE rbs_optional(VALUE type);
VALUE rbs_location(VALUE buffer, int start_pos, int end_pos);
VALUE rbs_location_pp(VALUE buffer, const position *start_pos, const position *end_pos);
VALUE rbs_location_tok(VALUE buffer, const token *tok);
VALUE rbs_location_current_token(parserstate *state);
VALUE rbs_function(VALUE required_positional_params, VALUE optional_positional_params, VALUE rest_positional_params, VALUE trailing_positional_params, VALUE required_keywords, VALUE optional_keywords, VALUE rest_keywords, VALUE return_type);
VALUE rbs_function_param(VALUE type, VALUE name, VALUE location);
VALUE rbs_block(VALUE type, VALUE required);
VALUE rbs_proc(VALUE function, VALUE block, VALUE location);
VALUE rbs_literal(VALUE literal, VALUE location);
