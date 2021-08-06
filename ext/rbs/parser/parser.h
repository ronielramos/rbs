#include "ruby.h"
#include "ruby/re.h"
#include "ruby/encoding.h"

#include "ruby_objs.h"

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
  position start;
  position end;
} range;

typedef struct {
  enum TokenType type;
  range range;
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

char *peek_token(lexstate *state, token tok);
int token_chars(token tok);
int token_bytes(token tok);

VALUE parse_type(parserstate *state);
VALUE parse_method_type(parserstate *state);
VALUE parse_signature(parserstate *state);

void pp(VALUE object);


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

#define RANGE_BYTES(range) (range.end.byte_pos - range.start.byte_pos)
