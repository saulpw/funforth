#include <assert.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef signed long _t;

#define STACK_SIZE 32
#define DICT_SIZE 8192

#define DPRINTF //printf

#define NEXT goto CHECK_STATE  // goto _NEXT in release

#define BLANK_CHAR 32

// xti = index into builtin_words[].  builtin_words[xti] == a valid xt
enum {
#define F(F_CODE) // and leave in place until the end of the file
#define N(I, CTOK, F, C)  XTI_##CTOK,
#include "words.inc"
#undef N
        XTI_MAX
};


typedef struct word_hdr_t
{
    void *codeptr;
    struct word_hdr_t *prev;
    unsigned immed : 1;
    unsigned do_does : 1;    // if 1, codeptr is actually new IP
    char name[19]; 
    _t body[];
} word_hdr_t;

// global data for all threads
// ==============================
      char         dict[DICT_SIZE];
const word_hdr_t * builtin_words;    // words[] for global access with XT()
      word_hdr_t * LATEST = NULL;    // most recently defined Forth word
      char        *DP = dict;        // Dictionary Pointer

typedef struct user_t {
      _t           parmstack[STACK_SIZE];
      void *       retstack[STACK_SIZE];

      void **      _FRAME;        // saved RP for TRY/THROW/CATCH/FINALLY
      int          _STATE;        // 0 is interpret, 1 is compile mode


      _t           _TOS;          // Top Of parameter Stack
      _t           *_SP;          // parameter Stack Pointer
      void *       *_RP;          // Return stack Pointer
const word_hdr_t * *_IP;          // Instruction Pointer
const char         *_CP;          // Character Pointer (input stream)
const word_hdr_t *  _WP;          // Word Pointer

} user_t;


#define TOS u->_TOS
#define SP u->_SP
#define RP u->_RP
#define IP u->_IP
#define CP u->_CP
#define WP u->_WP
#define STATE u->_STATE
#define FRAME u->_FRAME

void  _PUSH(user_t *u, _t v)     { *++SP = TOS; TOS = (v); }
_t    _POP(user_t *u)            { _t r = TOS; TOS = *SP--; return r; }

void  _RPUSH(user_t *u, void *v) { *++RP = v; }
void *_RPOP(user_t *u)           { return *RP--; }

void  _COMMA(_t val)  { *(_t *) DP = val; DP += sizeof(_t); }
int   _ABORT(user_t *u);

#define PUSH(v)  _PUSH(u, (_t) (v))
#define POP(v)   _POP(u)
#define RPUSH(v) _RPUSH(u, (void *) (v))
#define RPOP(v)  _RPOP(u)
#define COMMA(v) _COMMA((_t) (v))
#define ABORT(MSG) do { PUSH(MSG); return _ABORT(u); } while (0)

#define XT(CTOK) (&builtin_words[XTI_##CTOK])

int engine(user_t *u, const char *input)
{
    // the dictionary of builtins must be inside the function with the C labels
    static const word_hdr_t words[] = {
#define N(IMMED, CTOK, FNAME, C_CODE)                 \
        [XTI_##CTOK] = {                                  \
            .name = FNAME,                            \
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
    WORD(u);         // tokenize the next word from the input stream
    FIND(u);         // try to look it up in the dictionary

    if (TOS == 0) {         // if word was not found
        POP();              //   (discard FIND flag)
        NUMBER(u);       //   convert to number.
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

const word_hdr_t *_FIND(const char *name)
{
    const word_hdr_t *w;

    for (w=LATEST; w; w = w->prev) {
        if (strcasecmp(name, w->name) == 0) {
            return w;
        }
    }

    return NULL;
}

// FIND looks in the dictionary for the char* on the stack.  If
// found, leaves the word_ptr* and a 1 or -1 on top.

// ( token_ptr -- token_ptr 0 | word_ptr 1 | immed_word_ptr -1 )

int FIND(user_t *u)
{
    const word_hdr_t *w = _FIND((const char *) TOS);

    if (w == NULL) {
        // not found, leave token_ptr on stack
        PUSH(0);
    } else {
        // replace token_ptr with word_ptr
        TOS = (_t) w;
        PUSH(w->immed ? -1 : 1);
    }

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

int main()
{
    assert(sizeof(_t) == sizeof(void *));

    char boot_input[] =

#undef F
#define F(F_CODE) F_CODE " " // extra space as a courtesy to successive F()s
#define N(I, C, F, C_)
#include "words.inc"

// some tests
" : pass      42 . ;"
" : fail      -1 . ;"
" : testif       23 42 = IF fail ELSE pass THEN pass ; "
" : testinfloop  1 BEGIN DUP . 1 + AGAIN ;"
" : testuntil    1 BEGIN DUP . 1 + DUP 10 = UNTIL ;"
" : testdoloop  10 0 DO I . LOOP ;"
" : raise     -1 THROW fail ;"
" : testexc   TRY pass CATCH fail FINALLY pass"
"             TRY raise fail CATCH pass FINALLY pass ;"

" WORDS SEE try testif testuntil testdoloop testexc BYE"
;

    user_t user;
    engine(&user, boot_input);
}

