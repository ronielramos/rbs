#include "ruby.h"
#include "ruby/re.h"
#include "ruby/encoding.h"

typedef struct {
  int byte_pos;
  int char_pos;
  int line;
  int column;
} position;

enum TokenType {
  NullType,         /* (Nothing) */

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

  kSINGLETON,       /* singleton */
  kSELF,            /* self */
  kINSTANCE,        /* instance */

  tLIDENT,          /* Identifiers starting with lower case */
  tUIDENT,          /* Identifiers starting with upper case */
  tULIDENT,         /* Identifiers starting with `_` */
  tGIDENT,          /* Identifiers starting with `$` */
  tAIDENT,          /* Identifiers starting with `@` */
  tA2IDENT,         /* Identifiers starting with `@@` */
};

typedef struct {
  enum TokenType type;
  position start;
  position end;
} token;

typedef struct {
  VALUE string;
  position current;
  token last_token;
} lexstate;

token rbsparser_nextToken(lexstate *state, token token);
