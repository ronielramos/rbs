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
  pEQ,              /* = */
  pEQ2,             /* == */
  pEQ3,             /* === */
  pEQT,             /* =~ */
  pBANG,            /* ! */
  pQUESTION,        /* ? */
  pPERCENT,         /* % */

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
  kTYPE,            /* type */
  kUNCHECKED,       /* unchecked */
  kIN,              /* in */
  kOUT,             /* out */
  kEND,             /* end */
  kDEF,             /* def */

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
  tANNOTATION,      /* Annotation */
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

typedef struct id_table {
  size_t size;
  size_t count;
  ID *ids;
  struct id_table *next;
} id_table;

typedef struct {
  position start;
  position end;
  size_t line_size;
  size_t line_count;
  token *tokens;
} comment;

typedef struct {
  lexstate *lexstate;
  token current_token;
  token next_token;
  token next_token2;
  VALUE buffer;
  id_table *vars;
  comment *last_comment;
} parserstate;

extern token NullToken;
extern position NullPosition;

token rbsparser_next_token(lexstate *state);

extern VALUE rbsparser_Keywords;

char *peek_token(lexstate *state, token tok);
int token_chars(token tok);
int token_bytes(token tok);

VALUE parse_type(parserstate *state);
VALUE parse_method_type(parserstate *state);
VALUE parse_signature(parserstate *state);

void pp(VALUE object);

extern VALUE RBSParser;
extern VALUE rbsparser_Keywords;

extern ID id_RBS;
extern VALUE sym_class;
extern VALUE sym_alias;
extern VALUE sym_interface;

extern VALUE RBS_AST;
extern VALUE RBS_AST_Comment;
extern VALUE RBS_AST_Annotation;

extern VALUE RBS_AST_Declarations;

/**
 * RBS::AST::Declarations::ModuleTypeParams
 * */
extern VALUE RBS_AST_Declarations_ModuleTypeParams;
/**
 * RBS::AST::Declarations::ModuleTypeParams::TypeParam
 * */
extern VALUE RBS_AST_Declarations_ModuleTypeParams_TypeParam;

extern VALUE RBS_AST_Declarations_Alias;
extern VALUE RBS_AST_Declarations_Constant;
extern VALUE RBS_AST_Declarations_Global;
extern VALUE RBS_AST_Declarations_Interface;

extern VALUE RBS_AST_Members;
extern VALUE RBS_AST_Members_Alias;
extern VALUE RBS_AST_Members_AttrAccessor;
extern VALUE RBS_AST_Members_AttrReader;
extern VALUE RBS_AST_Members_AttrWriter;
extern VALUE RBS_AST_Members_ClassInstanceVariable;
extern VALUE RBS_AST_Members_ClassVariable;
extern VALUE RBS_AST_Members_Extend;
extern VALUE RBS_AST_Members_Include;
extern VALUE RBS_AST_Members_InstanceVariable;
extern VALUE RBS_AST_Members_MethodDefinition;
extern VALUE RBS_AST_Members_Prepend;
extern VALUE RBS_AST_Members_Private;
extern VALUE RBS_AST_Members_Public;

extern VALUE RBS_Location;
extern VALUE RBS_Namespace;
extern VALUE RBS_TypeName;

extern VALUE RBS_Types_Alias;
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
extern VALUE RBS_Types_Block;
extern VALUE RBS_Types_ClassInstance;
extern VALUE RBS_Types_ClassSingleton;
extern VALUE RBS_Types_Function_Param;
extern VALUE RBS_Types_Function;
extern VALUE RBS_Types_Interface;
extern VALUE RBS_Types_Intersection;
extern VALUE RBS_Types_Literal;
extern VALUE RBS_Types_Optional;
extern VALUE RBS_Types_Proc;
extern VALUE RBS_Types_Record;
extern VALUE RBS_Types_Tuple;
extern VALUE RBS_Types_Union;
extern VALUE RBS_Types_Variable;
extern VALUE RBS_Types;
extern VALUE RBS_MethodType;

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
VALUE rbs_record(VALUE fields, VALUE location);
VALUE rbs_variable(VALUE name, VALUE location);

VALUE rbs_method_type(VALUE type_params, VALUE type, VALUE block, VALUE location);

VALUE rbs_ast_comment(VALUE string, VALUE location);
VALUE rbs_ast_annotation(VALUE string, VALUE location);

/**
 * RBS::AST::Declarations::ModuleTypeParams.new()
 * */
VALUE rbs_ast_decl_module_type_params();

/**
 * RBS::AST::Declarations::ModuleTypeParams::TypeParam.new(name: name, variance: variance, skip_validation: skip_validation);
 * */
VALUE rbs_ast_decl_module_type_params_param(VALUE name, VALUE variance, VALUE skip_validation, VALUE location);

VALUE rbs_ast_decl_constant(VALUE name, VALUE type, VALUE location, VALUE comment);
VALUE rbs_ast_decl_global(VALUE name, VALUE type, VALUE location, VALUE comment);
VALUE rbs_ast_decl_alias(VALUE name, VALUE type, VALUE annotations, VALUE location, VALUE comment);
VALUE rbs_ast_decl_interface(VALUE name, VALUE type_params, VALUE members, VALUE annotations, VALUE location, VALUE comment);

VALUE rbs_ast_members_method_definition(VALUE name, VALUE kind, VALUE types, VALUE annotations, VALUE location, VALUE comment, VALUE overload);
VALUE rbs_ast_members_variable(VALUE klass, VALUE name, VALUE type, VALUE location, VALUE comment);
VALUE rbs_ast_members_mixin(VALUE klass, VALUE name, VALUE args, VALUE annotations, VALUE location, VALUE comment);
VALUE rbs_ast_members_attribute(VALUE klass, VALUE name, VALUE type, VALUE ivar_name, VALUE kind, VALUE annotations, VALUE location, VALUE comment);
VALUE rbs_ast_members_visibility(VALUE klass, VALUE location);
VALUE rbs_ast_members_alias(VALUE new_name, VALUE old_name, VALUE kind, VALUE annotations, VALUE location, VALUE comment);

void rbs_unescape_string(VALUE string);

id_table *parser_push_table(parserstate *state);
void parser_insert_id(parserstate *state, ID id);
bool parser_id_member(parserstate *state, ID id);
void parser_pop_table(parserstate *state);

void print_parser(parserstate *state);
void parser_advance(parserstate *state);
void parser_advance_assert(parserstate *state, enum TokenType type);
void print_token(token tok);

void insert_comment_line(parserstate *state, token token);
VALUE get_comment(parserstate *state, int subject_line);

extern const char *RBS_TOKENTYPE_NAMES[];


#define null_position_p(pos) (pos.byte_pos == -1)
#define nonnull_pos_or(pos1, pos2) (null_position_p(pos1) ? pos2 : pos1)

