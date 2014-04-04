#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "funforth.h"

#define DICT_SIZE 8192

char               dict[DICT_SIZE];
char *             DP = dict;
word_hdr_t *       LATEST = NULL;
const word_hdr_t * builtin_words;

const char kernel[] =
#define F(F_CODE) F_CODE " " // extra space as a courtesy to the next F()s
#include "words.inc"
    ;

// builtin_words[xti] == a valid execution token (xt)
enum {
#define N(I, C_TOKEN, F, D, C)  XTI_##C_TOKEN,
#include "words.inc"
        NUM_BUILTINS
};

int engine(user_t *u, const char *input)
{
    assert(sizeof(_t) == sizeof(void *));

    // The dictionary of builtins must be inside the function with the C labels.
    //   The &&CTOK in codeptr is the target of the goto *(WP->codeptr) below.
    //     (note: gcc-extension)
    static const word_hdr_t words[] = {
#define N(IMMED, C_TOKEN, FORTH_TOKEN, D, C_CODE)                     \
        [XTI_##C_TOKEN] = {                                  \
            .name = FORTH_TOKEN,                                \
            .codeptr = &&C_TOKEN,                            \
            .immed = IMMED,                               \
            .do_does = 0,                                 \
            .prev = ((XTI_##C_TOKEN == 0) ? NULL :           \
                    (word_hdr_t *) &words[XTI_##C_TOKEN-1]), \
        },
#include "words.inc"
    };

    if (LATEST == NULL) {  // first time only initialization
        LATEST = (word_hdr_t *) &words[NUM_BUILTINS-1];
        builtin_words = words; // will be the same for every thread anyway
        if (engine(u, kernel) < 0) return -1;
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
        RPUSH(IP);
        IP = WP->codeptr;  // whereas in ENTER IP is set to &WP->body
        PUSH(WP->body);
        NEXT;
    }

    goto *(WP->codeptr); // native word

#define N(I, C_TOKEN, F, D, C_CODE) C_TOKEN: C_CODE; NEXT;
#include "words.inc"

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

    if (def->codeptr == XT(ENTER)->codeptr) {
        const word_hdr_t **w;
        for (w = (const word_hdr_t **) def->body; *w != XT(EXIT); ++w) {
            printf(" %s", (*w)->name);
            if (*w == XT(BRANCH) ||
                *w == XT(BRANCHZ) ||
                *w == XT(DO_TRY) ||
                *w == XT(DO_LIT)) {
                ++w;
                printf("(%d)", *(int *) w);
            }
        }
    }

    printf(" ;%s ", def->immed ? " IMMEDIATE" : "");
    return 1;
}

