#include <assert.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef long _t; // sizeof(_t) >= sizeof(word_hdr_t *); must be signed also

#define STACK_SIZE 32
#define DICT_SIZE 8192

#define DPRINTF //printf

#define NEXT goto CHECK_STATE  // goto _NEXT in release

#define BLANK_CHAR 32

// xti = index into builtin_words[].  builtin_words[xti] == a valid xt
enum {
#define N(IMMED, CTOK, FORTHNAME, C_CODE)  XTI_##CTOK,
#include "words.inc"
#undef N
        XTI_MAX
};

char input[];

typedef struct word_hdr_t
{
    void *codeptr;
    struct word_hdr_t *prev;
    unsigned immed : 1;
    unsigned do_does : 1;    // if 1, codeptr is actually new IP
    char name[19]; 
    _t body[];
} word_hdr_t;

// global data
// ==============================
      _t           parmstack[STACK_SIZE];
      void *       retstack[STACK_SIZE];
      char         dict[DICT_SIZE];

#define XT(CTOK) (&builtin_words[XTI_##CTOK])
const word_hdr_t * builtin_words;    // words[] for global access

      void **      exc_frame;        // for TRY/THROW/CATCH/FINALLY

// registers
// ------------------------------
      _t           TOS = 0;          // Top Of parameter Stack
      _t           *SP = parmstack;  // parameter Stack Pointer
      void *       *RP = retstack;   // Return stack Pointer
const word_hdr_t * *IP = NULL;       // Instruction Pointer
const char         *CP = input;      // Character Pointer (input stream)
const word_hdr_t *  WP = NULL;       // Word Pointer

      char         *DP = dict;       // Dictionary Pointer

      word_hdr_t *  LATEST = NULL;   // most recently defined Forth word

      int           STATE = 0;       // 0 is interpret, 1 is compile mode
// ===============================

int  _ABORT(void);
void _COMMA(_t val)  { *(_t *) DP = val; DP += sizeof(_t); }
void _PUSH(_t v)     { *++SP = TOS; TOS = (v); }
void _RPUSH(void *v) { *++RP = v; }
   _t POP()          { _t r = TOS; TOS = *SP--; return r; }
void *RPOP()         { return *RP--; }

#define PUSH(v)  _PUSH((_t) (v))
#define RPUSH(v) _RPUSH((void *) (v))
#define COMMA(v) _COMMA((_t) (v))
#define ABORT(MSG) do { PUSH(MSG); _ABORT(); } while (0)

int main()
{
    // the dictionary of builtins
    static const word_hdr_t words[] = {
#define N(IMMED, CTOK, FORTHNAME, C_CODE)                 \
        [XTI_##CTOK] = {  .name = FORTHNAME,              \
            .codeptr = &&CTOK,                            \
            .immed = IMMED,                               \
            .do_does = 0,                                 \
            .prev = ((XTI_##CTOK+1 == XTI_MAX) ? NULL :   \
                    (word_hdr_t *) &words[XTI_##CTOK+1]), \
        },

#include "words.inc"
#undef N
    };

    builtin_words = words;
    LATEST = (word_hdr_t *) words;

    _t acc;

CHECK_STATE:
    if (SP - parmstack < 0)           { ABORT("data stack underflow"); }
    if (SP - parmstack >= STACK_SIZE) { ABORT("data stack overflow"); }
    if (RP - retstack < 0)           { ABORT("return stack underflow"); }
    if (RP - retstack >= STACK_SIZE) { ABORT("return stack overflow"); }

    if (IP == NULL) { goto INTERPRET_WORD; }

_NEXT:    
    WP = *(const word_hdr_t **) IP++;
    // fall-through
 
DO_WORD: // for EXECUTE to goto
    DPRINTF("\n%s '%s'", (STATE == 1) ? "(compile)" : "(interpret)", WP->name);
//    PRINTSTACK();

    if (WP->do_does) {
        PUSH(WP->body);
        RPUSH(IP);
        IP = WP->codeptr;
        NEXT;
    }

    goto *(WP->codeptr); // native word

#define N(I, CTOK, FORTHNAME, C_CODE) CTOK: C_CODE; NEXT;
#include "words.inc"
#undef N

INTERPRET_WORD:
    PUSH(BLANK_CHAR);
    WORD();
    FIND();
    if (TOS == 0) { // not found, try to convert to number
        POP();      // discard FIND flag
        NUMBER();
        if (STATE == 1) {
            goto LITERAL;
        }
        NEXT;
    } else if (POP() > 0) { //  not-immediate
        if (STATE == 1) {
            goto COMMA;
        }
    }

    // EXECUTE immediately if TOS <0 or if STATE != COMPILE_MODE
    goto EXECUTE;

}

int _ABORT(void)
{
    printf("ABORT: %s\n", (const char *) POP());

    PRINTSTACK();

    abort();
}


// WORD skips leading whitespace, copies the next token from the input stream
// (CP) into a local static buffer, and pushes that valid asciiz char* onto the
// stack.

int WORD() // ( delimiter_char -- token_ptr )
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
        exit(0);
    }
    DPRINTF("\n\t\tWORD>     %s", HP);
}

// NUMBER tries to convert the char* on the stack to an integer, ABORTing
// if it fails.

int NUMBER()  // ( token_ptr -- int )
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

const word_hdr_t *_FIND(const char *name)
{
    const word_hdr_t *w;

    for (w=LATEST; w; w = w->prev) {
        if (strcmp(name, w->name) == 0) {
            return w;
        }
    }

    return NULL;
}

int FIND()
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
}

int PRINTSTACK(void)
{
    printf("\n    [%d] %d", (int) (SP - parmstack), (int) TOS);

    _t *ptr = SP;
    for (ptr = SP; ptr > parmstack; --ptr) {
        printf("\n    [%d] %d", (int) (ptr - parmstack - 1), (int) *ptr);
    }

    void **rptr;
    for (rptr = RP; rptr > retstack; --rptr) {
        printf("\n  RP[%d] %p", (int) (rptr - retstack - 1), *rptr);
    }

}

int PRINTDICT(void)
{
    printf("\n");
    const word_hdr_t *def;

    for (def = LATEST; def != XT(DO_ENTER); def = def->prev) {
        printf(":VERBATIM %s ", def->name);

        if (def->codeptr == XT(DO_ENTER)->codeptr) {
          const word_hdr_t **w;
          for (w = (const word_hdr_t **) def->body; *w != XT(DO_EXIT); ++w) {
            printf(" %s", (*w)->name);
            if (*w == XT(DO_BRANCH) ||
                *w == XT(DO_BRANCHZ) ||
                *w == XT(DO_TRY) ||
                *w == XT(DO_LITERAL)) {
                ++w;
                printf("(%d) ", *(int *) w);
            }
          }
        }
        printf(" ; \n");
    }
}

char input[] =
" : 2DUP  OVER OVER ;"  // ( a b -- a b a b )
" : 2DROP DROP DROP ;" // ( a b -- )

" : CONSTANT   CREATE , DOES> @ ; IMMEDIATE"
" : VARIABLE   CREATE 0 , DOES> ; IMMEDIATE"

" 8 CONSTANT cellsize"   // cellsize == sizeof(_t)
" : CELLS cellsize * ;"  // for '2 CELLS ALLOT' allocation notation
" : BL 32 ;"             // 32 == ' ' must match BLANK_CHAR
" : ( 41 WORD DROP ; IMMEDIATE"  // ( inline comments )  41 == ')'
" : OFFSET   - cellsize / 1 - ;" // ( &ptr[a] &ptr[b] -- a-b )

//  ' (TICK)   ( -- xt )   quotes the next word in the input stream
" : '    BL WORD FIND 0= ?ABORT ;"

/*
 * POSTPONE is used in the definition of a compile (immediate) word W.  It
 * reads the next token T at W compile time; then it is in W's run-time
 * execution stream.
   : POSTPONE     \ POSTPONE is used in the compilation of another word
        '         \ it gets the xt of the succeeding token,
        LITERAL   \ compiles it as a compile-time literal,
        ['] ,     \ pushes the xt of the comma-compile word
        ,         \ compile , into the current definition
   ; IMMEDIATE
 */
" : POSTPONE  ' LITERAL ['] , , ; IMMEDIATE"

    /*  >MARK    ( -- &src_ip )  holds a slot for a forward branch offset.
     *                           use immediately after a POSTPONE (BRANCH*)
     *  >RESOLVE ( &src_ip -- )  fills the formerly >MARKed slot with the delta
     *  <MARK    ( -- &tgt_ip )  saves a branch target for <RESOLVE
     *  <RESOLVE ( &tgt_ip -- )  fills HERE with the <MARKed target.
                                 use immediately after a POSTPONE (BRANCH*)
     */

" : >MARK     HERE  1 CELLS ALLOT ;"
" : >RESOLVE  HERE OVER OFFSET SWAP ! ;"
" : <MARK     HERE ;"
" : <RESOLVE  HERE OFFSET , ;"

// BEGIN/AGAIN/UNTIL are straight-forward
" : BEGIN  <MARK ; IMMEDIATE"
" : AGAIN  POSTPONE (BRANCH) <RESOLVE ; IMMEDIATE"
" : UNTIL  POSTPONE (BRANCHZ) <RESOLVE ; IMMEDIATE"

// as are IF/ELSE/THEN
" : IF    POSTPONE (BRANCHZ) >MARK ; IMMEDIATE"
" : ELSE  POSTPONE (BRANCH)  >MARK SWAP >RESOLVE ; IMMEDIATE"
" : THEN  >RESOLVE ; IMMEDIATE"

//  DO/LOOP stores the loop counter and limit on the return stack
" : DO    POSTPONE 2>R <MARK ; IMMEDIATE"
" : LOOP  POSTPONE (LOOP) POSTPONE (BRANCHZ) <RESOLVE POSTPONE 2RDROP ; IMMEDIATE"

// TRY creates an exception frame on the return stack
" : TRY      POSTPONE (TRY)  >MARK ; IMMEDIATE"
" : CATCH    POSTPONE (CATCH) POSTPONE (BRANCH) >MARK SWAP >RESOLVE ; IMMEDIATE"
" : FINALLY  >RESOLVE ; IMMEDIATE"

// some tests
" : pass      42 . ;"
" : fail      -1 . ;"
" : testif       23 42 = IF fail ELSE pass THEN pass ; "
" : testinfloop  1 BEGIN DUP . 1 + AGAIN ;"
" : testuntil    1 BEGIN DUP . 1 + DUP 10 = UNTIL ;"
" : testdoloop  10 0 DO I . LOOP ;"
" : raise     THROW fail ;"
" : testexc   TRY pass CATCH fail FINALLY pass"
"             TRY raise fail CATCH pass FINALLY pass ;"

" testif testuntil testdoloop testexc"
;

