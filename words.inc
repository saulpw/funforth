 
// native words

N(0,DO_ENTER,    "(ENTER)",    RPUSH(IP); IP = (const word_hdr_t **) &WP->body)
N(0,DO_EXIT,     "(EXIT)",     IP = RPOP()                                    )

//   EXECUTE  ( xt -- )  
N(0,EXECUTE,     "EXECUTE",    WP = (word_hdr_t *) POP(); goto DO_WORD;       )

N(0,DO_BRANCH,   "(BRANCH)",   acc = (_t) *IP++; IP += acc                    )
N(0,DO_BRANCHZ,  "(BRANCHZ)",  acc = (_t) *IP++; if (! POP()) { IP += acc; }  )

N(0,EQ0,         "0=",         TOS = (TOS == 0)                               )
N(0,EQ,          "=",          TOS = (*SP-- == TOS)                           )

N(0,ADD,         "+",          TOS = *SP-- + TOS                              )
N(0,SUB,         "-",          TOS = *SP-- - TOS                              )
N(0,MULT,        "*",          TOS = *SP-- * TOS                              )
N(0,DIV,         "/",          TOS = *SP-- / TOS                              )

//   DUP   ( a -- a a )       DUPlicate the top-of-stack item
//   OVER  ( a b -- a b a )   copy the item under the top-of-stack
//   SWAP  ( a b -- b a )     SWAP the top two items
//   DROP  ( a -- )           discard the top item
N(0,DUP,         "DUP",        PUSH(TOS)                                      )
N(0,OVER,        "OVER",       PUSH(SP[0])                                    )
N(0,SWAP,        "SWAP",       acc = TOS; TOS = *SP; *SP = acc                )
N(0,DROP,        "DROP",       POP()                                          )

//    @  ( ptr -- val )        fetch the value at *ptr
//    !  ( val ptr -- )        store the value to *ptr
N(0,FETCH,       "@",          TOS = *(_t *) TOS                              )
N(0,STORE,       "!",          *(_t *) TOS = *SP--; TOS = *SP--               )

//   (LITERAL)  ( -- val )     inline literal from instruction stream
//   (VARIABLE) ( -- &var )    address of variable
//   (CONSTANT) ( -- val )     value of constant from definition in dictionary
N(0,DO_LITERAL,  "(LITERAL)",  PUSH(*IP++)                                    )
N(0,DO_VARIABLE, "(VARIABLE)", PUSH(&WP->body)                                )
N(0,DO_CONSTANT, "(CONSTANT)", PUSH(*(const _t *) &WP->body)                  )

//  ?ABORT      ( b -- )      abort if b is not zero
N(0,QABORT,      "?ABORT",     if (POP()) _ABORT()                            )

// transferring data to and from the return stack

//  >R  ( S: a --   ) ( R:   -- a )  from data stack 'to' Return stack
//  R>  ( S:   -- a ) ( R: a --   )  from Return stack 'to' data stack
//  R@  ( S:   -- a ) ( R: a -- a )  push top of Return stack, like RP@

//  2>R  ( S: a b --     ) ( R:     -- a b )
//  2R>  ( S:     -- a b ) ( R: a b --     )
//  2R@  ( S:     -- a b ) ( R: a b -- a b )

N(0,RPUSH,       ">R",         RPUSH(POP())                                   )
N(0,RPOP,        "R>",         PUSH(RPOP())                                   )
N(0,RCOPY,       "R@",         PUSH(*RP)                                      )

N(0,RPUSH2,      "2>R",        RPUSH(*SP); RPUSH(POP()); POP()                )
N(0,RPOP2,       "2R>",        PUSH(RP[-1]); PUSH(RP[0]); RP -= 2             )
N(0,RCOPY2,      "2R@",        PUSH(RP[-1]); PUSH(RP[0]);                     )
N(0,RDROP2,      "2RDROP",     RP -= 2 )

// unbalanced return stack operations are much easier as intrinsics

// (LOOP)   ( R: n i -- n i+1 )  ( S:  -- i==n ) for-loop-ish intrinsic
// I        ( R: n i -- n i )    ( S:  -- i    ) the first-level loop index
N(0,DO_LOOP,     "(LOOP)",     PUSH(RP[0] == RP[-1]); ++((_t *) RP)[0]        )
N(0,I,           "I",          PUSH(RP[0])                                    )

// -- compiler words

N(0,COLON,       ":",          STATE = 1; goto CREATE;                        )
N(1,SEMICOLON,   ";",          COMMA((_t) XT(DO_EXIT)); STATE = 0             )
N(1,CLEARCOMPILE,"[",          STATE = 0                                      )
N(0,SETCOMPILE,  "]",          STATE = 1                                      )
N(0,ALLOT,       "ALLOT",      DP += POP()                                    )
N(0,IMMEDIATE,   "IMMEDIATE",  LATEST->immed = 1                              )
N(0,COMMA,       ",",          COMMA(POP())                                   )
N(0,HERE,        "HERE",       PUSH(DP)                                       )
N(0,LITERAL,     "LITERAL",    COMMA((_t) XT(DO_LITERAL)); COMMA(POP())       )

N(0,CREATE, "CREATE", {
    PUSH(BLANK_CHAR);
    WORD();
    word_hdr_t * hdr = (word_hdr_t *) DP;
    DP += sizeof(word_hdr_t);
    strncpy(hdr->name, (const char *) POP(), sizeof(hdr->name));
    hdr->do_does = 0;
    hdr->codeptr = &&DO_ENTER;
    hdr->prev = LATEST;
    LATEST = hdr;
})

N(1,COMPILETICK, "[']",        PUSH(BLANK_CHAR);
                               WORD();
                               FIND();
                               if (POP() == 0) { _ABORT(); }
                               goto LITERAL
                           )

// DOES> sets it up so that the defined compiler word includes an action, which
// is used for the words it creates at runtime.
N(1,META_DOES,  "DOES>",       COMMA((_t) XT(DOES))                           )
N(0,DOES,       "(DOES>)",     LATEST->do_does = 1;
                               LATEST->codeptr = (void *) IP;
                               goto DO_EXIT
                           )

N(0,DO_TRY,     "(TRY)",       acc = (_t) *IP++;
                               RPUSH(SP);
                               RPUSH(IP + acc);
                               RPUSH(exc_frame);
                               exc_frame = RP
                           )
N(0,DO_CATCH,   "(CATCH)",     assert(exc_frame != NULL);
                               RP = exc_frame;
                               exc_frame = RPOP();
                               RP -= 2; // drop catch block IP and saved SP
                           )
N(0,THROW,      "THROW",       RP = exc_frame;
                               exc_frame = RPOP();
                               IP = RPOP(); 
                               SP = RPOP();
                           )

N(0,PRINT,       ".",          printf("%d ", (int) POP())                     )
N(0,TYPE,        "TYPE",       printf("%s", (const char *) POP())             )
N(0,EMIT,        "EMIT",       printf("%c", (char) POP())                     )

#define C(CTOK, FTOK) N(0, CTOK, FTOK, CTOK())
C(_ABORT,      "ABORT" )
C(NUMBER,      "NUMBER")
C(FIND,        "FIND"  )
C(WORD,        "WORD"  )
C(PRINTSTACK,  ".S"    )
C(PRINTDICT,   ".DICT" )
#undef C

