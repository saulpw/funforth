#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "funforth.h"

#define DICT_SIZE 8192

char               dict[DICT_SIZE];
const word_hdr_t * builtin_words;
word_hdr_t *       LATEST = NULL;
char *             DP = dict;

const char kernel[] =
#define F(F_CODE) F_CODE " " // extra space as a courtesy to the next F()s
#define N(I, C, F, C_)
#include "words.inc"
#undef N
#undef F
    ;

// builtin_words[xti] == a valid execution token (xt)
enum {
#define F(F_CODE) // and leave it inactive
#define N(I, CTOK, F, C)  XTI_##CTOK,
#include "words.inc"
#undef N
        XTI_MAX
};

int engine(user_t *u, const char *input)
{
    assert(sizeof(_t) == sizeof(void *));

    // the dictionary of builtins must be inside the function with the C labels
    static const word_hdr_t words[] = {
#define N(IMMED, CTOK, FNAME, C_CODE)                     \
        [XTI_##CTOK] = {                                  \
            .name = FNAME,                                \
            .codeptr = &&CTOK,                            \
            .immed = IMMED,                               \
            .do_does = 0,                                 \
            .prev = ((XTI_##CTOK == 0) ? NULL :           \
                    (word_hdr_t *) &words[XTI_##CTOK-1]), \
        },
#include "words.inc"
#undef N
    };

    if (LATEST == NULL) {  // first time only initialization
        LATEST = (word_hdr_t *) &words[XTI_MAX-1];
        builtin_words = words; // will be the same for every thread anyway
    }

    STATE = 0;
    TOS = 0;
    SP = u->parmstack;
    RP = u->retstack;
    IP = NULL;
    WP = NULL;

    CP = input;

    _t acc;

#define NEXT goto CHECK_STATE  // goto _NEXT in release

CHECK_STATE:
    if (SP - u->parmstack < 0)           { ABORT("data stack underflow"); }
    if (SP - u->parmstack >= STACK_SIZE) { ABORT("data stack overflow"); }
    if (RP - u->retstack < 0)            { ABORT("return stack underflow"); }
    if (RP - u->retstack >= STACK_SIZE)  { ABORT("return stack overflow"); }

    if (IP == NULL) { goto INTERPRET_WORD; }

_NEXT:    
    WP = *(const word_hdr_t **) IP++;
    // fall-through
 
DO_WORD: // for EXECUTE to goto
    DPRINTF("\n%s '%s'", (STATE == 1) ? "(compile)" : "(interpret)", WP->name);

    if (WP->do_does) {
        PUSH(WP->body);
        RPUSH(IP);
        IP = WP->codeptr;
        NEXT;
    }

    goto *(WP->codeptr); // native word

#define N(I, CTOK, FNAME, C_CODE) CTOK: C_CODE; NEXT;
#include "words.inc"
#undef N

INTERPRET_WORD:
    PUSH(BLANK_CHAR);
    if (WORD(u) <= 0) return 0; // tokenize the next word from the input stream
    FIND(u);                // try to look it up in the dictionary

    if (TOS == 0) {         // if word was not found
        POP();              //   (discard FIND flag)
        NUMBER(u);          //   convert to number.
        if (STATE == 1) {   // in compile-mode,
            goto LITERAL;   //   push the number as a literal at runtime
        }                   // in interpret mode, it just stays on the stack
        NEXT;
    } else if (POP() < 0) { // if word is an immediate word
        goto EXECUTE;       //   execute regardless
    } else {                // otherwise for non-immediate words
        if (STATE == 1) {   //    if in compile-mode,
            goto COMMA;     //      append this word's xt to the dictionary
        } else {
            goto EXECUTE;   //    in interpret mode, execute now
        }
    }
}

int _ABORT(user_t *u)
{
    printf("ABORT: %s\n", (const char *) POP());

    PRINTSTACK(u);

    return -1;
}

// WORD skips leading whitespace, copies the next token from the input stream
// (CP) into a local static buffer, and pushes that valid asciiz char* onto the
// stack.

int WORD(user_t *u) // ( delimiter_char -- token_ptr )
{
    static char HP[128];
    char delim = POP();
    while (*CP && isspace(*CP)) CP++;
    PUSH(HP);
    char *tmp = HP;
    char ch;
    while ((ch = *CP++)) {
        if (((delim != BLANK_CHAR) && (delim == ch)) || 
                ((delim == BLANK_CHAR) && isspace(ch))) {
            break;
        }
        *tmp++ = ch;
    }
    *tmp = 0;
    if (tmp == HP) { // empty word
        DPRINTF("[eof]\n");
        return 0;
    }
    DPRINTF("\n\t\tWORD>     %s", HP);
    return 1;
}

// NUMBER tries to convert the char* on the stack to an integer, ABORTing
// if it fails.

int NUMBER(user_t *u)  // ( token_ptr -- int )
{
    char *endptr = NULL;
    const char *s = (const char *) TOS;
    TOS = strtoll(s, &endptr, 0);
    if (s == endptr) { // actually if entire token isn't consumed
        ABORT(s);
    }
}

// FIND looks in the dictionary for the char* on the stack.  If
// found, leaves the word_ptr* and a 1 or -1 on top.

// ( token_ptr -- token_ptr 0 | word_ptr 1 | immed_word_ptr -1 )

int FIND(user_t *u)
{
    const word_hdr_t *w;

    for (w=LATEST; w; w = w->prev) {
        if (strcasecmp((const char *) TOS, w->name) == 0) {
            // replace token_ptr with word_ptr
            TOS = (_t) w;
            PUSH(w->immed ? -1 : 1);
            return 1;
        }
    }

    // leave token_ptr on stack and push 0 (not found)
    PUSH(0);

    return 1;
}

int PRINTSTACK(user_t *u)
{
    printf("\n    [%d] %d", (int) (SP - u->parmstack), (int) TOS);

    _t *ptr = SP;
    for (ptr = SP; ptr > u->parmstack; --ptr) {
        printf("\n    [%d] %d", (int) (ptr - u->parmstack - 1), (int) *ptr);
    }

    void **rptr;
    for (rptr = RP; rptr > u->retstack; --rptr) {
        printf("\n  RP[%d] %p", (int) (rptr - u->retstack - 1), *rptr);
    }

    return 1;
}

int WORDS(user_t *u)
{
    const word_hdr_t *def;
    for (def = LATEST; def; def = def->prev) {
        printf("%s ", def->name);
    }
    return 1;
}

int PRINTWORD(user_t *u)
{
    const word_hdr_t *def = (const word_hdr_t *) POP();

    printf("\n:VERBATIM %s ", def->name);

    if (def->codeptr == XT(DO_ENTER)->codeptr) {
        const word_hdr_t **w;
        for (w = (const word_hdr_t **) def->body; *w != XT(DO_EXIT); ++w) {
            printf(" %s", (*w)->name);
            if (*w == XT(DO_BRANCH) ||
                *w == XT(DO_BRANCHZ) ||
                *w == XT(DO_TRY) ||
                *w == XT(DO_LITERAL)) {
                ++w;
                printf("(%d)", *(int *) w);
            }
        }
    }

    printf(" ;%s ", def->immed ? " IMMEDIATE" : "");
    return 1;
}

