
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

char input[] = " 2 4 + . : 1+ 1 + ; 4 1+ 1+ . ABORT ";

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
      int           COMPILING = FALSE;
// ===============================

#define SWAP() { _t r = TOS; TOS = *SP; *SP = r; }
#define POP() ({ _t r = TOS; TOS = *SP--; r; })
#define PUSH(v) { *++SP = TOS; TOS = (_t) v; }

#define RPOP() (*RP--)
#define RPUSH(v) { *++RP = (void *) v;}

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

void comma(_t v)
{
    _t *ptr = ALLOT(sizeof(_t));
    *ptr = v;
}

void DOES_FORTH(const char *junk, ...)
{
    const word_hdr_t *w;
    LATEST->body = DP;

    va_list ap;
    va_start(ap, junk);
    while (w = va_arg(ap, const word_hdr_t *)) {
        comma((_t) w);
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

#define IMMEDIATE   LATEST->immed = 1

#define JMP(OFFSET) XT_DO_BRANCH, ((void *) OFFSET)
#define JMPZ(OFFSET) XT_DO_BRANCHZ, ((void *) OFFSET)

int main()
{
    assert(sizeof(voidfunc_t) == sizeof(_t));
    printf("sizeof(_t) == %d, sizeof(word_hdr_t) == %d\n",
                (int) sizeof(_t), (int) sizeof(word_hdr_t));

    NATIVE_LABEL("(BRANCH)", DO_BRANCH)
    NATIVE_LABEL("(BRANCHZ)", DO_BRANCHZ)
    NATIVE_LABEL("EXECUTE", EXECUTE)
    NATIVE_LABEL("+", ADD)
    NATIVE_LABEL("(LITERAL)", DO_LITERAL)
    NATIVE_LABEL("(CONSTANT)", DO_CONSTANT)
    NATIVE_LABEL("ABORT", ABORT)
    NATIVE_LABEL("(ENTER)", DO_ENTER)
    NATIVE_LABEL("EXIT", EXIT)
    NATIVE_LABEL("]", SETCOMPILE)
    NATIVE_LABEL("[", CLEARCOMPILE)
    NATIVE_LABEL("HERE", HERE)
    NATIVE_LABEL("INTERPRET", INTERPRET)
    NATIVE_LABEL(">BODY", SETBODY)
    // ... ~20 more ...

    NATIVE_FUNC("DOES>", DOES)
    NATIVE_FUNC("CREATE", CREATE)
    NATIVE_FUNC("NUMBER", NUMBER)
    NATIVE_FUNC("FIND", FIND)
    NATIVE_FUNC("WORD", WORD)
    NATIVE_FUNC(".", PRINT)
    NATIVE_FUNC(",", COMMA)

    //  : QUIT  BEGIN INTERPRET AGAIN ;
    BUILTIN_WORD("QUIT", QUIT,
            XT_INTERPRET, JMP(-3), XT_EXIT);

    //  : : WORD CREATE DOES> DO_ENTER HERE >BODY ] ;
    BUILTIN_WORD(":", COLON,
            XT_WORD, XT_CREATE, XT_DO_LITERAL, XT_DO_ENTER, XT_DOES,
            XT_HERE, XT_SETBODY, XT_SETCOMPILE, XT_EXIT); 

    BUILTIN_WORD(";", SEMICOLON,
            XT_DO_LITERAL, XT_EXIT, XT_COMMA, 
            XT_CLEARCOMPILE, XT_EXIT); IMMEDIATE;
    // -----

    IP = XT_QUIT->body;

    _t acc;

NEXT:    WP = *IP++; // fall-through
 
DO_WORD:
    if (WP == NULL) goto ABORT;

//    printf("%s\n", WP->name);

    if (WP->isfunc) {
        (WP->codeptr)();
        goto NEXT;
    }

    goto *(WP->codeptr);

EXECUTE:
    WP = (word_hdr_t *) POP();
    goto DO_WORD;

INTERPRET:
        WORD();
        FIND();
        if (TOS == 0) { // not found, try to convert to number
            POP();
            NUMBER();
            if (COMPILING) {
                comma((_t) XT_DO_LITERAL);
                comma(POP());
            } 
            goto NEXT;
        } 

        if (POP() > 0) { //  not-immediate
            if (COMPILING) {
                comma(POP());
                goto NEXT;
            }
        }

        goto EXECUTE;

DO_LITERAL:   PUSH(*IP++);                                           goto NEXT;
DO_CONSTANT:  PUSH(*(const _t *) WP->body);                          goto NEXT;
DO_VARIABLE:  PUSH(WP->body);                                        goto NEXT;

DO_BRANCH:    acc = (_t) *IP++; IP += acc;                           goto NEXT;
DO_BRANCHZ:   acc = (_t) *IP++; if (POP() == FALSE) { IP += acc; }   goto NEXT;
DO_ENTER:     RPUSH(IP); IP = WP->body;                              goto NEXT;
EXIT:         IP = RPOP();                                           goto NEXT;

ADD:          TOS += *SP--;                                          goto NEXT;

SETCOMPILE:   COMPILING = TRUE;                                      goto NEXT;
CLEARCOMPILE: COMPILING = FALSE;                                     goto NEXT;
SETBODY:      LATEST->body = (void *) POP();                         goto NEXT;
HERE:         PUSH(DP);                                              goto NEXT;

ABORT:        
    while (SP != argstack) {
        printf("SP[%d] %d\n", (int) (SP - argstack), (int) POP());
    }
    while (RP != retstack) {
        printf("RP[%d] %p\n", (int) (RP - retstack), RPOP());
    }
//    for (WP = LATEST; WP; WP = WP->prev) printf("%s\n", WP->name);
    // IP, CP, HP, WP, DP

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
}

void NUMBER()
{
    char *endptr = NULL;
    const char *s = (const char *) TOS;
    TOS = strtoll(s, (char **) endptr, 0);
    if (s == endptr) {
        printf("'%s' not found\n", s);
        abort();
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

void CREATE(void)
{
    DO_CREATE((const char *) POP());
}

void COMMA(void)
{
    comma(POP());
}

void DOES(void)
{
    const word_hdr_t *w = (void *) POP();
    _DOES(w->codeptr);
}

