#ifndef FUNFORTH_H_
#define FUNFORTH_H_

typedef signed long _t;

#ifdef DEBUG
#define DPRINTF printf
#else
#define DPRINTF
#endif

#define STACK_SIZE 32

#define BLANK_CHAR ' '

typedef struct word_hdr_t
{
    void *codeptr;
    struct word_hdr_t *prev;
    unsigned immed : 1;
    unsigned do_does : 1;    // if 1, codeptr is actually new IP
    char name[19]; 
    _t body[];
} word_hdr_t;

// per-instance data
typedef struct user_t
{
          _t           parmstack[STACK_SIZE];
          void *       retstack[STACK_SIZE];

          void **      _FRAME;     // saved RP for TRY/THROW/CATCH/FINALLY
          int          _STATE;     // 0 is interpret, 1 is compile mode

          _t           _TOS;       // Top Of parameter Stack
          _t           *_SP;       // parameter Stack Pointer
          void *       *_RP;       // Return stack Pointer
    const word_hdr_t * *_IP;       // Instruction Pointer
    const char         *_CP;       // Character Pointer (input stream)
    const word_hdr_t *  _WP;       // Word Pointer
} user_t;

// global data for all threads/instances
// ==============================
extern       char         dict[];
extern       char        *DP;            // Dictionary Pointer
extern       word_hdr_t * LATEST;        // most recently defined Forth word
extern const word_hdr_t * builtin_words; // words[] for global access with XT()

extern const char         kernel[];      // initial definitions

#define TOS u->_TOS
#define SP u->_SP
#define RP u->_RP
#define IP u->_IP
#define CP u->_CP
#define WP u->_WP
#define STATE u->_STATE
#define FRAME u->_FRAME

#define INLINE static inline

INLINE void  _PUSH(user_t *u, _t v)     { *++SP = TOS; TOS = (v); }
INLINE _t    _POP(user_t *u)            { _t r = TOS; TOS = *SP--; return r; }

INLINE void  _RPUSH(user_t *u, void *v) { *++RP = v; }
INLINE void *_RPOP(user_t *u)           { return *RP--; }

INLINE void  _COMMA(_t val)  { *(_t *) DP = val; DP += sizeof(_t); }
extern int   _ABORT(user_t *u);

extern void init_user(user_t *u);
extern int engine(user_t *u, const word_hdr_t *xt, const char *input);

#define PUSH(v)  _PUSH(u, (_t) (v))
#define POP(v)   _POP(u)
#define RPUSH(v) _RPUSH(u, (void *) (v))
#define RPOP(v)  _RPOP(u)
#define COMMA(v) _COMMA((_t) (v))
#define ABORT(MSG) do { PUSH(MSG); return ABORTPRINT(u); } while (0)

#define XT(CTOK) (&builtin_words[XTI_##CTOK])


#endif
