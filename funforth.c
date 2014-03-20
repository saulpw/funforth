#include <assert.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

char input[] =
    " : CELLS 8 * ;"
    " : OFFSET   - 8 / 1 - ;"
    " : >MARK     1 CELLS ALLOT ;"
    " : >RESOLVE  DUP HERE SWAP OFFSET SWAP ! ;"
    " : <MARK     HERE ;"
    " : <RESOLVE  HERE OFFSET , ;"
    " : [']  WORD FIND =0 ?ABORT LITERAL ; IMMEDIATE"
    " : '    WORD FIND =0 ?ABORT ;"
    " : POSTPONE  ' LITERAL ['] , , ; IMMEDIATE"
    " : IF    POSTPONE BRANCHZ >MARK ; IMMEDIATE"
    " : ELSE  POSTPONE BRANCH  >MARK SWAP >RESOLVE ; IMMEDIATE"
    " : THEN  >RESOLVE ; IMMEDIATE"
    " : ?DUP  DUP IF DUP THEN ;"
    " : BEGIN  <MARK ; IMMEDIATE"
    " : AGAIN  POSTPONE BRANCH <RESOLVE ; IMMEDIATE"
    " : TESTLOOP  1 BEGIN DUP . 1 + AGAIN ;"
    " TESTLOOP"
;

typedef int64_t _t;

#define TRUE -1   // all 1s
#define FALSE 0   // all 0s

typedef void (*voidfunc_t)();

typedef struct word_hdr_t
{
    voidfunc_t codeptr;
    struct word_hdr_t *prev;
    unsigned isfunc : 1;
    unsigned immed : 1;
    unsigned ip_parm : 1; // if the cell this wordptr has a parm
    char name[19]; 
    void *body;
} word_hdr_t;

// ==============================
      _t           argstack[32];
      void *       retstack[32];
      char         heap[1024];
      char         dict[102400];
// ------------------------------
      _t           TOS = 0;
      _t           *SP = argstack;
      void *       *RP = retstack;
const word_hdr_t * *IP = NULL;
const char *        CP = input;
      char *        HP = heap;
const word_hdr_t *  WP = NULL;

      char *        DP = dict;

      word_hdr_t *  LATEST = NULL;

#define INTERPRET_MODE 0
#define COMPILE_MODE 1
      int           STATE = INTERPRET_MODE;
// ===============================

#define POP() ({ _t r = TOS; TOS = *SP--; r; })
#define PUSH(v) { *++SP = TOS; TOS = (_t) v; }

#define RPOP() (*RP--)
#define RPUSH(v) { *++RP = (void *) v;}

#ifdef DEBUG
#define DPRINTF printf
#else
#define DPRINTF(...)
#endif

#define ABORT(MSG) PUSH(MSG); _ABORT();
void _ABORT(void);

void *ALLOT(int nbytes)
{
    void *r = DP;
    DP += nbytes;
    return r;
}

word_hdr_t *DO_CREATE(const char *name)
{
    word_hdr_t * hdr = ALLOT(sizeof(word_hdr_t));
    hdr->prev = LATEST;
    strncpy(hdr->name, name, sizeof(hdr->name));
    LATEST = hdr;
    return hdr;
}

void _DOES(void *label)
{
    LATEST->isfunc = 0;
    LATEST->codeptr = (voidfunc_t) label;
}

void DOES_C(voidfunc_t func)
{
    LATEST->isfunc = 1;
    LATEST->codeptr = func;
}

void comma(const void * v)
{
    const void **ptr = ALLOT(sizeof(_t));
    *ptr = v;
}

void DOES_FORTH(const char *junk, ...)
{
    const word_hdr_t *w;
    LATEST->body = DP;

    va_list ap;
    va_start(ap, junk);
    while (w = va_arg(ap, const word_hdr_t *)) {
        comma(w);
    }
    va_end(ap);
}

#define NATIVE_LABEL(FORTHTOK, CLABEL) \
    const word_hdr_t * XT_##CLABEL = DO_CREATE(FORTHTOK); \
    _DOES(&&CLABEL);

#define NATIVE_FUNC(FORTHTOK, CFUNC) \
    void CFUNC(void); \
    const word_hdr_t * XT_##CFUNC = DO_CREATE(FORTHTOK); \
    DOES_C(&CFUNC);

#define BUILTIN_WORD(FORTHTOK, CTOK, args...) \
    const word_hdr_t * XT_##CTOK = DO_CREATE(FORTHTOK); \
    _DOES(&&DO_ENTER); \
    DOES_FORTH("", args, NULL);

#define MAKE_IMMEDIATE   LATEST->immed = 1
#define HAS_PARM  LATEST->ip_parm = 1

#define JMP(OFFSET) XT_DO_BRANCH, ((void *) (OFFSET))
#define JMPZ(OFFSET) XT_DO_BRANCHZ, ((void *) (OFFSET))


int main()
{
    assert(sizeof(voidfunc_t) == sizeof(_t));
    printf("sizeof(_t) == %d, sizeof(word_hdr_t) == %d\n",
                (int) sizeof(_t), (int) sizeof(word_hdr_t));

    NATIVE_LABEL("BRANCH", DO_BRANCH)   HAS_PARM;
    NATIVE_LABEL("BRANCHZ", DO_BRANCHZ)   HAS_PARM;
    NATIVE_LABEL("EXECUTE", EXECUTE)
    NATIVE_LABEL("=0", EQ0)
    NATIVE_LABEL("+", ADD)
    NATIVE_LABEL("-", SUB)
    NATIVE_LABEL("*", MULT)
    NATIVE_LABEL("/", DIV)
    NATIVE_LABEL("DUP", DUP)
    NATIVE_LABEL("SWAP", SWAP)
    NATIVE_LABEL("!", STORE)
    NATIVE_LABEL("@", FETCH)
    NATIVE_LABEL("(LITERAL)", DO_LITERAL)   HAS_PARM;
    NATIVE_LABEL("(CONSTANT)", DO_CONSTANT)
    NATIVE_LABEL("(ENTER)", DO_ENTER)
    NATIVE_LABEL("EXIT", EXIT)
    NATIVE_LABEL("INTERPRET", INTERPRET)
    NATIVE_LABEL("?ABORT", QABORT)

    NATIVE_LABEL("ALLOT", ALLOT)

#ifndef NO_COMPILER
    NATIVE_LABEL("]", SETCOMPILE)
    NATIVE_LABEL("[", CLEARCOMPILE)
    NATIVE_LABEL(">BODY", SETBODY)
    NATIVE_LABEL("DOES>", DOES)
    NATIVE_LABEL("CREATE", CREATE)
    NATIVE_LABEL("IMMEDIATE", IMMEDIATE)
    NATIVE_LABEL(",", COMMA)
    NATIVE_LABEL("HERE", HERE)
    NATIVE_LABEL("LITERAL", LITERAL)
#endif

    NATIVE_FUNC(".DICT", PRINTDICT)
    NATIVE_FUNC("ABORT", _ABORT)
    NATIVE_FUNC("NUMBER", NUMBER)
    NATIVE_FUNC("FIND", FIND)
    NATIVE_FUNC("WORD", WORD)
    NATIVE_FUNC(".", PRINT)
    NATIVE_FUNC("TYPE", TYPE)
    NATIVE_FUNC(".S", PRINTSTACK)

    //  : QUIT  BEGIN INTERPRET AGAIN ;
    BUILTIN_WORD("QUIT", QUIT,
            XT_INTERPRET, JMP(-3), XT_EXIT);

    //  : : WORD CREATE DOES> DO_ENTER HERE >BODY ] ;
    BUILTIN_WORD(":", COLON,
            XT_WORD, XT_CREATE, XT_DO_LITERAL, XT_DO_ENTER, XT_DOES,
            XT_HERE, XT_SETBODY, XT_SETCOMPILE, XT_EXIT); 

    BUILTIN_WORD(";", SEMICOLON,
            XT_DO_LITERAL, XT_EXIT, XT_COMMA, 
            XT_CLEARCOMPILE, XT_EXIT); MAKE_IMMEDIATE;
    // -----

    IP = XT_QUIT->body;

    _t acc;

#ifdef RELEASE
#define NEXT goto _NEXT
#else
#define NEXT goto CHECK_STATE
CHECK_STATE:
    if (IP == NULL) { ABORT("IP == NULL"); }
    if (SP - argstack < 0) { ABORT("data stack underflow"); }
    if (SP - argstack > sizeof(argstack)/sizeof(argstack[0])) { ABORT("data stack overflow"); }
    if (RP - retstack < 0) { ABORT("return stack underflow"); }
    if (RP - retstack > sizeof(retstack)/sizeof(retstack[0])) { ABORT("return stack overflow"); }
#endif

_NEXT:    
    WP = *IP++; // fall-through
 
DO_WORD:
    DPRINTF("%s%s\n", (STATE == COMPILE_MODE) ? "| " : "", WP->name);

    if (WP->isfunc) {
        (WP->codeptr)();
        NEXT;
    }

    goto *(WP->codeptr);

//  : INTERPRET  WORD FIND =0 IF NUMBER STATE @ IF ' DO_LITERAL , , THEN ELSE IF>0 STATE @ IF , EXIT ELSE EXECUTE THEN ;
INTERPRET:
        WORD();
        FIND();
        if (TOS == 0) { // not found, try to convert to number
            POP();
            NUMBER();
            if (STATE == COMPILE_MODE) {
                comma(XT_DO_LITERAL);
                comma((void *) POP());
            }
            NEXT;
        } else if (POP() > 0) { //  not-immediate
            if (STATE == COMPILE_MODE) {
                comma((void *) POP());
                NEXT;
            }
        }

//        goto EXECUTE;  // fall-through instead

EXECUTE:
    WP = (word_hdr_t *) POP();
    goto DO_WORD;


DO_LITERAL:   PUSH(*IP++);                                           NEXT;
DO_CONSTANT:  PUSH(*(const _t *) WP->body);                          NEXT;
DO_VARIABLE:  PUSH(WP->body);                                        NEXT;

DO_BRANCH:    DPRINTF("JMP(%+d)\n", *(int *) IP);
              acc = (_t) *IP++; IP += acc;                           NEXT;
DO_BRANCHZ:   DPRINTF("JMPZ(%+d)\n", *(int *) IP);
              acc = (_t) *IP++; if (POP() == FALSE) { IP += acc; }   NEXT;
DO_ENTER:     RPUSH(IP); IP = WP->body;                              NEXT;
EXIT:         IP = RPOP();                                           NEXT;

EQ0:          TOS = (TOS == 0);                                      NEXT;

ADD:          TOS = *SP-- + TOS;                                     NEXT;
SUB:          TOS = *SP-- - TOS;                                     NEXT;
MULT:         TOS = *SP-- * TOS;                                     NEXT;
DIV:          TOS = *SP-- / TOS;                                     NEXT;

DUP:          PUSH(TOS);                                             NEXT;
SWAP:         acc = TOS; TOS = *SP; *SP = acc;                       NEXT;

STORE:        *(_t *) TOS = *SP--; TOS = *SP--;                      NEXT;
FETCH:        TOS = *(_t *) TOS;                                     NEXT;
ALLOT:        TOS = (_t) ALLOT(TOS);                                 NEXT;

#ifndef NO_COMPILER
SETCOMPILE:   STATE = COMPILE_MODE;                                  NEXT;
CLEARCOMPILE: STATE = INTERPRET_MODE;                                NEXT;
SETBODY:      LATEST->body = (void *) POP();                         NEXT;
IMMEDIATE:    LATEST->immed = 1;                                     NEXT;
HERE:         PUSH(DP);                                              NEXT;
CREATE:       DO_CREATE((const char *) POP());                       NEXT;
COMMA:        comma((void *) POP());                                 NEXT;
DOES:         _DOES(((const word_hdr_t *) POP())->codeptr);          NEXT;
LITERAL:      comma(XT_DO_LITERAL); comma((void *) POP());           NEXT;
#endif

QABORT:        if (POP()) { _ABORT(); }                              NEXT;

    return 0;
}


void WORD()
{
    while (*CP && isspace(*CP)) CP++;
    PUSH(HP);
    char *tmp = HP;
    while (*CP && !isspace(*CP)) {
        *tmp++ = *CP++;
    }
    *tmp = 0;
    if (tmp == HP) { // empty word
        DPRINTF("[eof]\n");
        exit(0);
    }
    DPRINTF(">>        %s\n", HP);
}

void NUMBER()
{
    char *endptr = NULL;
    const char *s = (const char *) TOS;
    TOS = strtoll(s, &endptr, 0);
    if (s == endptr) { // actually if entire token isn't consumed
        ABORT(s);
    }
}

// ( nameptr -- nameptr 0 | wordptr -1 )
void FIND()
{
    for (WP = LATEST; WP; WP = WP->prev) {
        if (strcmp((const char *) TOS, WP->name) == 0) {
            TOS = (_t) WP;
            PUSH(WP->immed ? -1 : 1);
            return;
        }
    }
    PUSH(0);
}

void PRINT(void)
{
    printf("%d ", (int) POP());
}

void TYPE(void)
{
    printf("%s ", (const char *) POP());
}

void PRINTSTACK(void)
{
    _t *ptr = SP;
    printf("--SP[%d] %d\n", (int) (ptr - argstack), (int) TOS);
    for (ptr = SP; ptr > argstack; --ptr) {
        printf("  SP[%d] %d\n", (int) (ptr - argstack - 1), (int) *ptr);
    }
    void **rptr;
    for (rptr = RP; rptr > retstack; --rptr) {
        printf("  RP[%d] %p\n", (int) (rptr - retstack), *rptr);
    }

}

void _ABORT(void)
{
    printf("ABORT: %s\n", (const char *) POP());

    PRINTSTACK();

    abort();
}

void PRINTDICT(void)
{
    for (WP = LATEST; WP && !WP->isfunc; WP = WP->prev) {
        printf(":VERBATIM %s ", WP->name);
        const word_hdr_t **w;
        for (w = WP->body; strcmp((*w)->name, "EXIT"); ++w) {
            printf(" %s", (*w)->name);
            if ((*w)->ip_parm) {
                ++w;
                printf("(%d) ", *(int *) w);
            }
        }
        printf(" ; \n");
    }
}

