
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef int64_t _t;

char input[] = "2 4 + . ABORT";

_t    argstack[32];
void *retstack[32];
char  heap[128];
char  dict[128];

typedef void (*voidfunc_t)();
typedef struct word_hdr_t
{
    voidfunc_t codeptr;
    unsigned isfunc : 1;
    char name[11]; 
    const struct word_hdr_t **body;
} word_hdr_t;

      _t            TOS = 0;
      _t           *SP = argstack;
      void *       *RP = retstack;
const word_hdr_t * *IP = NULL;
const char *        CP = input;
      char *        HP = heap;
const word_hdr_t *  WP = NULL;

// array of native labels and functions
const word_hdr_t   *g_natives;

#define TRUE -1   // all 1s
#define FALSE 0   // all 0s

void FIND();
void NUMBER();
void WORD();

#define POP() ({ _t r = TOS; TOS = *SP--; r; })
#define PUSH(v) { *++SP = TOS; TOS = (_t) v; }

#define RPOP() (*RP--)
#define RPUSH(v) { *++RP = (void *) v;}

int main()
{
    assert(sizeof(voidfunc_t) == sizeof(_t));

#define NATIVE_LABEL(FORTHTOK, CLABEL) \
        { .isfunc = 0, .name = FORTHTOK, .codeptr = (voidfunc_t) &&CLABEL },

#define NATIVE_FUNC(FORTHTOK, CFUNC) \
        { .isfunc = 1, .name = FORTHTOK, .codeptr = &CFUNC },

    static word_hdr_t s_natives[] = {
        NATIVE_LABEL("(BRANCH)", DO_BRANCH)
        NATIVE_LABEL("(BRANCHZ)", DO_BRANCHZ)
        NATIVE_LABEL("EXECUTE", EXECUTE)
        NATIVE_LABEL(";", EXIT)
        NATIVE_LABEL("+", ADD)
        NATIVE_LABEL(".", PRINT)
        NATIVE_LABEL("(LITERAL)", DO_LITERAL)
        NATIVE_LABEL("ABORT", ABORT)

        NATIVE_FUNC("NUMBER", NUMBER)
        NATIVE_FUNC("FIND", FIND)
        NATIVE_FUNC("WORD", WORD)

        NATIVE_LABEL("", NEXT)
    };

#define XT_BRANCH(N) (&s_natives[0]), ((void *) N)
#define XT_BRANCHZ(N) (&s_natives[1]), ((void *) N)
#define XT_EXECUTE (&s_natives[2])
#define XT_EXIT (&s_natives[3])
#define XT_NUMBER (&s_natives[8])
#define XT_FIND (&s_natives[9])
#define XT_WORD (&s_natives[10])

    //  : INTERPRET  WORD FIND IF EXECUTE ELSE NUMBER THEN ;
    // =>  [ WORD FIND BRANCHZ(+2) EXECUTE EXIT NUMBER EXIT ]
    static const word_hdr_t *INTERPRET[] = {
        XT_WORD, XT_FIND, XT_BRANCHZ(+2), XT_EXECUTE, XT_EXIT,
        XT_NUMBER, XT_EXIT 
    };

#define BUILTIN(FORTHTOK, BODY) { .isfunc = 0, .name = FORTHTOK, .codeptr = (voidfunc_t) &&DO_ENTER, .body = BODY }
    static word_hdr_t s_builtins[] = {
        BUILTIN("INTERPRET", INTERPRET)
    };

#define XT_INTERPRET (&s_builtins[0])

    //  : QUIT  BEGIN INTERPRET AGAIN ;
    // =>  [ INTERPRET BRANCH(-3) EXIT ]
    static const word_hdr_t *QUIT[] =
        { XT_INTERPRET, XT_BRANCH(-3), XT_EXIT };

    g_natives = s_natives;
    IP = QUIT;

    _t acc;

NEXT:
    WP = *IP++;
    // fall-through
 
DO_WORD:
    if (WP->isfunc) {
        (WP->codeptr)();
        goto NEXT;
    }

    goto *(WP->codeptr);

DO_LITERAL:
    PUSH(*IP++);
    goto NEXT;

ADD:
    TOS += *SP--;
    goto NEXT;

PRINT:
    printf("%d ", (int) TOS);
    TOS = *SP--;
    goto NEXT;

EXECUTE:
    WP = (word_hdr_t *) POP();
    goto DO_WORD;

DO_BRANCH:
    acc = (_t) *IP++;
    IP += acc;
    goto NEXT;

DO_BRANCHZ:
    acc = (_t) *IP++;
    if (POP() == FALSE) {
        IP += acc;
    }
    goto NEXT;

DO_ENTER:
    RPUSH(IP);
    IP = WP->body;
    goto NEXT;

EXIT:
    IP = RPOP();
    goto NEXT;

ABORT:
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
    TOS = atoi((const char *) TOS);
}

// ( nameptr -- nameptr 0 | wordptr -1 )
void FIND()
{
    const word_hdr_t *natives = g_natives;
    PUSH(TOS);
    TOS = FALSE;
    int i;
    for (i=0; natives[i].name[0]; ++i) {
        if (strcmp(*SP, natives[i].name) == 0) {
            *SP = (_t) &natives[i];
            TOS = TRUE;
            break;
        }
    }
}

