
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef int64_t _t;

char input[] = "2 4 + . ;";

_t   data[32];
char heap[128];
char dict[128];

typedef void (*voidfunc_t)();
typedef struct
{
    voidfunc_t codeptr;
    unsigned isfunc : 1;
    char name[11]; 
} word_hdr_t;

        _t *SP = data;
const word_hdr_t **IP = 0;
const char *CP = input;
      char *HP = heap;
const word_hdr_t *WP = NULL;
        _t TOS = 0;

const word_hdr_t *g_natives;

#define LITERAL(N) ((_t) N)
#define LABEL(N) ((_t) &&N)
#define FUNCTION(F) ((_t) (&F))

void QUIT();
void FIND();
void NUMBER();
void WORD();

#define POP() ({ _t r = TOS; TOS = *SP--; r; })
#define PUSH(v) { *++SP = TOS; TOS = (_t) v; }

int main()
{
    assert(sizeof(voidfunc_t) == sizeof(_t));

#define NATIVE_LABEL(FORTHTOK, CLABEL) \
        { .isfunc = 0, .name = FORTHTOK, .codeptr = (voidfunc_t) &&CLABEL },

#define NATIVE_FUNC(FORTHTOK, CFUNC) \
        { .isfunc = 1, .name = FORTHTOK, .codeptr = &CFUNC },

    static word_hdr_t s_natives[] = {
        NATIVE_LABEL("(BRANCH)", DO_BRANCH)
        NATIVE_LABEL("INTERPRET", INTERPRET)
        NATIVE_LABEL(";", EXIT)
        NATIVE_LABEL("+", ADD)
        NATIVE_LABEL(".", PRINT)
        NATIVE_LABEL("EXECUTE", EXECUTE)
        NATIVE_LABEL("(LITERAL)", DO_LITERAL)

        NATIVE_FUNC("NUMBER", NUMBER)
        NATIVE_FUNC("FIND", FIND)
        NATIVE_FUNC("WORD", WORD)

        NATIVE_LABEL("", NEXT)
    };

#define XT_BRANCH(N) (&s_natives[0]), ((void *) N)
#define XT_INTERPRET (&s_natives[1])

    // : QUIT  BEGIN INTERPRET AGAIN ; => INTERPRET BRANCH(-3) EXIT
    static const word_hdr_t *QUIT[] =
        { XT_INTERPRET, XT_BRANCH(-3), 0 };

    g_natives = s_natives;
    IP = QUIT;

    _t acc;

NEXT:
    WP = *IP++;

    if (! WP) {
        return 0;
    }

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
    (WP->codeptr)();
    goto NEXT;

INTERPRET:
    WORD();
    FIND();
    if (POP()) { // found
        goto EXECUTE;
    }
    NUMBER();
    goto NEXT;

DO_BRANCH:
    acc = (_t) *IP++;
    IP += acc;
    goto NEXT;

EXIT:
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

void FIND()
{
    const word_hdr_t *natives = g_natives;
    PUSH(TOS);
    TOS = 0; // FALSE
    int i;
    for (i=0; natives[i].name[0]; ++i) {
        if (strcmp(*SP, natives[i].name) == 0) {
            *SP = (_t) &natives[i];
            TOS = -1; // TRUE
            break;
        }
    }
}

