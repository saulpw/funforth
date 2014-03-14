
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

char input[] = " 2 4 + . ABORT";

typedef int64_t _t;

#define TRUE -1   // all 1s
#define FALSE 0   // all 0s

typedef void (*voidfunc_t)();

typedef struct word_hdr_t
{
    voidfunc_t codeptr;
    struct word_hdr_t *prev;
    unsigned isfunc : 1;
    char name[19]; 
    void *body;
} word_hdr_t;

// ==============================
      _t           argstack[32];
      void *       retstack[32];
      char         heap[1024];
      char         dict[1024];
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
      int           STATE = FALSE;
// ===============================

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

#define BUILTIN_WORD(FORTHTOK, args...) \
    const word_hdr_t * XT_##FORTHTOK = DO_CREATE(#FORTHTOK); \
    _DOES(&&DO_ENTER); \
    DOES_FORTH("", args, NULL);

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
    NATIVE_LABEL(">BODY", SETBODY)
    // ... ~20 more ...

    NATIVE_FUNC("DOES>", DOES)
    NATIVE_FUNC("CREATE", CREATE)
    NATIVE_FUNC("NUMBER", NUMBER)
    NATIVE_FUNC("FIND", FIND)
    NATIVE_FUNC("WORD", WORD)
    NATIVE_FUNC(".", PRINT)
//    NATIVE_FUNC(":", COLON)
//    IMMED_FUNC(";", SEMICOLON)

    //  : INTERPRET  WORD FIND IF EXECUTE ELSE NUMBER THEN ;
    // =>  [ WORD FIND BRANCHZ(+2) EXECUTE EXIT NUMBER EXIT ]
    BUILTIN_WORD(INTERPRET, 
            XT_WORD, XT_FIND, 
            JMPZ(+2), XT_EXECUTE, XT_EXIT,
            XT_NUMBER, XT_EXIT);

    //  : QUIT  BEGIN INTERPRET AGAIN ;
    // =>  [ INTERPRET BRANCH(-3) EXIT ]
    BUILTIN_WORD(QUIT, XT_INTERPRET, JMP(-3), XT_EXIT);

    BUILTIN_WORD(COLON, XT_WORD, XT_CREATE, XT_DOES, XT_DO_ENTER,
                        XT_SETCOMPILE, XT_HERE, XT_SETBODY, XT_EXIT); 

    // -----

    IP = XT_QUIT->body;

    _t acc;

NEXT:    WP = *IP++; // fall-through
 
DO_WORD:
//    printf("%s\n", WP->name);

    if (WP->isfunc) {
        (WP->codeptr)();
        goto NEXT;
    }

    goto *(WP->codeptr);

EXECUTE:
    WP = (word_hdr_t *) POP();
    goto DO_WORD;

DO_LITERAL:   PUSH(*IP++);                                           goto NEXT;
DO_CONSTANT:  PUSH(*(const _t *) WP->body);                          goto NEXT;
DO_VARIABLE:  PUSH(WP->body);                                        goto NEXT;

DO_BRANCH:    acc = (_t) *IP++; IP += acc;                           goto NEXT;
DO_BRANCHZ:   acc = (_t) *IP++; if (POP() == FALSE) { IP += acc; }   goto NEXT;
DO_ENTER:     RPUSH(IP); IP = WP->body;                              goto NEXT;
EXIT:         IP = RPOP();                                           goto NEXT;

ADD:          TOS += *SP--;                                          goto NEXT;

SETCOMPILE:   STATE = TRUE;                                          goto NEXT;
CLEARCOMPILE: STATE = FALSE;                                         goto NEXT;
SETBODY:      LATEST->body = (void *) POP();                         goto NEXT;
HERE:         PUSH(DP);                                              goto NEXT;

ABORT:        return 0;
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
    TOS = atoi((const char *) TOS);
}

// ( nameptr -- nameptr 0 | wordptr -1 )
void FIND()
{
    const word_hdr_t *w;

    for (w = LATEST; w; w = w->prev) {
        if (strcmp((const char *) TOS, w->name) == 0) {
            TOS = (_t) w;
            PUSH(TRUE);
            return;
        }
    }
    PUSH(FALSE);
}

void PRINT(void)
{
    printf("%d ", (int) POP());
}

void CREATE(void)
{
    DO_CREATE((const char *) POP());
}

void DOES(void)
{
    const word_hdr_t *w = (void *) POP();
    _DOES(w->codeptr);
}

