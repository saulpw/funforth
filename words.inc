

#ifndef N   // Native word
#define N(IMMED, C_TOKEN, FORTH_TOKEN, STACK_COMMENT, C_CODE)
#endif

#ifndef F   // forth definition (as a string)
#define F(FORTH_CODE)
#endif

#define C(CTOK, FTOK, STACK_COMMENT) \
    N(0, CTOK, FTOK, STACK_COMMENT, if ((acc = CTOK(u)) <= 0) return acc)

// arithmetic
N(0,INCR,   "1+",     ( a -- a+1 ),      ++TOS                  )
N(0,DECR,   "1-",     ( a -- a-1 ),      --TOS                  )
N(0,TWOMULT,"2*",     ( a -- a*2 ),      TOS <<= 1              )
N(0,TWODIV, "2/",     ( a -- a/2 ),      TOS >>= 1              )
N(0,ADD,    "+",      ( a b -- a+b ),    TOS = *SP-- + TOS      )
N(0,SUB,    "-",      ( a b -- a-b ),    TOS = *SP-- - TOS      )
N(0,MULT,   "*",      ( a b -- a*b ),    TOS = *SP-- * TOS      )
N(0,DIV,    "/",      ( a b -- a/b ),    TOS = *SP-- / TOS      )

N(0,MOD,    "MOD",    ( a b -- a%b ),    TOS = *SP-- % TOS      )
N(0,ABS,    "ABS",    ( a -- |a| ),      TOS = abs(TOS)         )
N(0,NEGATE, "NEGATE", ( a -- -a ),       TOS = -TOS             )
N(0,MAX,    "MAX",    ( a b -- a|b ),    acc = *SP--; if (acc > TOS) TOS = acc         )
N(0,MIN,    "MIN",    ( a b -- a|b ),    acc = *SP--; if (acc < TOS) TOS = acc         )
N(0,MULTDIV,"*/",     ( a b c -- a*b/c ),TOS = (long long) SP[0] * SP[-1]/TOS; SP -= 2 )
N(0,DIVMOD, "/MOD",   ( a b -- a%b a/b ),acc = SP[0] / TOS; SP[0] %= TOS; TOS = acc    )

// bit twiddling
N(0,AND,    "AND",    ( a b -- a&b ),    TOS = *SP-- & TOS      )
N(0,OR,     "OR",     ( a b -- a|b ),    TOS = *SP-- | TOS      )
N(0,XOR,    "XOR",    ( a b -- a^b ),    TOS = *SP-- ^ TOS      )
N(0,INVERT, "INVERT", ( a -- ~a ),       TOS = ~TOS             )
N(0,LSHIFT, "LSHIFT", ( a b -- a<<b ),   TOS = *SP-- << TOS     )
N(0,RSHIFT, "RSHIFT", ( a b -- a>>b ),   TOS = *SP-- >> TOS     )

// comparison
N(0,EQ0,    "0=",     ( a | fl ),        TOS = (TOS == 0)       )
N(0,LT0,    "0<",     ( a | fl ),        TOS = (TOS < 0)        )
N(0,EQ,     "=",      ( a | fl ),        TOS = (*SP-- == TOS)   )
N(0,GTEQ,   ">=",     ( a | fl ),        TOS = (*SP-- >= TOS)   )
N(0,LTEQ,   "<=",     ( a | fl ),        TOS = (*SP-- <= TOS)   )
N(0,LT,     "<",      ( a | fl ),        TOS = (*SP-- <  TOS)   )
N(0,GT,     ">",      ( a | fl ),        TOS = (*SP-- >  TOS)   )

// stack manipulation
N(0,DUP,    "DUP",    ( a -- a a ),            PUSH(TOS)                       )
N(0,NIP,    "NIP",    ( a b -- b ),            SP -= 1                         )
N(0,SWAP,   "SWAP",   ( a b -- b a ),          acc = TOS; TOS = *SP; *SP = acc )
N(0,QDUP,   "?DUP",   ( a|0 -- a a | 0 ),      if (TOS) PUSH(TOS)              )
N(0,OVER,   "OVER",   ( a b -- a b a ),        PUSH(*SP)                       )
N(0,DROP,   "DROP",   ( a -- ),                TOS = *SP--                     )
N(0,TWODROP,"2DROP",  ( a b -- ),              TOS = SP[-1]; SP -= 2           )
N(0,TWODUP, "2DUP",   ( a b -- a b a b ),      PUSH(*SP); PUSH(*SP)            )
N(0,TWOOVER,"2OVER",  ( a b c d -- a b c d a b ), PUSH(SP[-2]); PUSH(SP[-2])   )
N(0,ROT,    "ROT",    ( a b c -- b c a ),      acc = SP[-1]; SP[-1] = SP[0]; SP[0] = TOS; TOS = acc )
N(0,TWOSWAP,"2SWAP",  ( a b c d -- c d a b ),  acc = TOS; TOS = SP[-1]; SP[-1] = acc;
                                               acc = SP[0]; SP[0] = SP[-2]; SP[-2] = acc )

// to and from the return stack
N(0,RPUSH, ">R", (S: a --   ) (R:   -- a ),  RPUSH(POP())     )
N(0,RPOP,  "R>", (S:   -- a ) (R: a --   ),  PUSH(RPOP())     )
N(0,RCOPY, "R@", (S:   -- a ) (R: a -- a ),  PUSH(*RP)        )

N(0,RDROP2,"2RDROP", (R: a b -- ),           RP -= 2                           )
N(0,RPUSH2,"2>R",(S: a b -- ) (R: -- a b ),  RPUSH(*SP); RPUSH(POP()); POP()   )
N(0,RPOP2, "2R>",(S: -- a b ) (R: a b -- ),  PUSH(RP[-1]); PUSH(RP[0]); RP -= 2)
N(0,RCOPY2,"2R@",(S: -- a b ) (R: a b -- a b ), PUSH(RP[-1]); PUSH(RP[0])      )

N(0,DEPTH, "DEPTH",  ( -- n ),    PUSH(SP - u->parmstack) )
N(0,RDEPTH,"RDEPTH", ( -- n ),    PUSH(RP - u->retstack)  )

// memory access
N(0,FETCH,    "@", ( ptr -- v ),       TOS = *(_t *) TOS                   )
N(0,STORE,    "!", ( v ptr -- ),       *(_t *) TOS = *SP--; TOS = *SP--    )
N(0,ADDSTORE, "+!",( incr ptr --),     *(_t *) TOS += *SP--; TOS = *SP--   )

N(0,CFETCH,   "C@", ( ptr -- ch ),     TOS = *(char *) TOS                 )
N(0,CSTORE,   "C!", ( ch ptr -- ),     *(char *) TOS = *SP--; TOS = *SP--  )
N(0,CCOMMA,   "C,", ( ch -- ),         *DP++ = POP()                       )

N(0,FILL,  "FILL", ( ptr n val -- ), memset((void *) SP[-1], SP[0], TOS);                 SP -= 2; POP() )
N(0,MOVE,  "MOVE", ( src dst n -- ), memmove((void *) SP[0], (const void *) SP[-1], TOS); SP -= 2; POP() )

// internal words
N(0,DO_DOES, "DO_DOES", (R: -- ip ), RPUSH(IP);
                                     IP = WP->codeptr;
                                     PUSH(WP->body)                           )
N(0,ENTER,   "ENTER",   (R: -- ip ), RPUSH(IP);
                                     IP = (const word_hdr_t **) &WP->body     )
N(0,EXIT,    "EXIT",    (R: ip -- ), IP = RPOP()                              )
N(0,EXECUTE, "EXECUTE", ( xt -- ),   WP = (word_hdr_t *) POP(); goto DO_WORD  )
N(0,BRANCH, "(BRANCH)", ( -- ),      acc = (_t) *IP++; IP += acc;             )
N(0,BRANCHZ,"(BRANCHZ)",( fl -- ),   acc = (_t) *IP++; if (!POP()) IP += acc  )

N(0,DO_LITERAL,  "(LITERAL)",  ( -- k ),    PUSH(*IP++)                   )
N(0,DO_CONSTANT, "(CONSTANT)", ( -- k ),    PUSH(*(const _t *) &WP->body) )
N(0,DO_VARIABLE, "(VARIABLE)", ( -- &v ),   PUSH(&WP->body)               )

// unbalanced return stack operations are much easier as intrinsics
N(0,DO_LOOP, "(LOOPINC)", (R: n i -- n i+1 ) (S: -- i==n ), PUSH(++RP[0] == RP[-1]) )
N(0,I,       "I",         (R: n i -- n i )   (S:  -- i ),   PUSH(RP[0])              ) 


N(0,DO_TRY,     "(TRY)", (R: -- sp exc_ip ebp ), acc = (_t) *IP++;
                                                 RPUSH(SP);
                                                 RPUSH(IP + acc);
                                                 RPUSH(FRAME);
                                                 FRAME = RP  )

// UNTRY is at the end of every TRY clause, pops the exception frame 
// and drops the exception handler IP and saved SP
N(0,DO_UNTRY,   "(UNTRY)", (R: sp exc_ip ebp),   assert(FRAME != NULL);
                                                 RP = FRAME;
                                                 FRAME = RPOP();
                                                 RP -= 2 )

N(0,THROW,      "THROW",   ( exc -- ),           RP = FRAME;
                                                 FRAME = RPOP();
                                                 IP = RPOP(); 
                                                 SP = RPOP()  )

N(0,BYE,       "BYE",    ( -- ),      return 0                    )
N(0,QABORT,    "?ABORT", ( flag -- ), if (POP()) ABORT("?ABORT")  )

C(_ABORT,      "ABORT",  ( -- )                        )
C(NUMBER,      "NUMBER", ( str -- i )                  )
C(FIND,        "FIND",   ( str -- xt|str  flag|0 )     )

// the minimum necessary compiler words
N(0,COLON,     ":",         ( -- ), STATE = 1; goto CREATE                 )
N(0,VERBATIM,  ":VERBATIM", ( -- ), STATE = 2; goto CREATE                 )
N(1,SEMICOLON, ";",         ( -- ), STATE = 0; COMMA((_t) XT(EXIT))        )

// gets the next word from the input stream and adds a dictionary entry for it
N(0,CREATE, "CREATE", ( -- ), {
    PUSH(BLANK_CHAR);
    WORD(u);

    word_hdr_t * hdr = (word_hdr_t *) DP;
    DP += sizeof(word_hdr_t);

    strncpy(hdr->name, (const char *) POP(), sizeof(hdr->name));
    hdr->do_does = 0;
    hdr->codeptr = &&ENTER;
    hdr->prev = LATEST;
    LATEST = hdr;
})


#include "compiler.inc"
#include "interpreter.inc"

#undef C
#undef N
#undef F
