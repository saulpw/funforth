
// native words

N(0,DO_ENTER,    "(ENTER)",    RPUSH(IP); IP = (const word_hdr_t **) &WP->body)
N(0,DO_EXIT,     "(EXIT)",     IP = RPOP()                                    )

//   EXECUTE  ( xt -- )  
N(0,EXECUTE,     "EXECUTE",    WP = (word_hdr_t *) POP(); goto DO_WORD;       )

N(0,DO_BRANCH,   "(BRANCH)",   acc = (_t) *IP++; IP += acc                    )
N(0,DO_BRANCHZ,  "(BRANCHZ)",  acc = (_t) *IP++; if (! POP()) { IP += acc; }  )

N(0,EQ0,         "0=",         TOS = (TOS == 0)                               )
N(0,EQ,          "=",          TOS = (*SP-- == TOS)                           )
N(0,GTEQ,        ">=",         TOS = (*SP-- >= TOS)                           )
N(0,LTEQ,        "<=",         TOS = (*SP-- <= TOS)                           )
N(0,LT,          "<",          TOS = (*SP-- <  TOS)                           )
N(0,GT,          ">",          TOS = (*SP-- >  TOS)                           )

N(0,ADD,         "+",          TOS = *SP-- + TOS                              )
N(0,SUB,         "-",          TOS = *SP-- - TOS                              )
N(0,MULT,        "*",          TOS = *SP-- * TOS                              )
N(0,DIV,         "/",          TOS = *SP-- / TOS                              )

//   DUP   ( a -- a a )       DUPlicate the top-of-stack item
//   OVER  ( a b -- a b a )   copy the item under the top-of-stack
//   SWAP  ( a b -- b a )     SWAP the top two items
//   DROP  ( a -- )           discard the top item
F(" : 2DUP  OVER OVER ;")  // ( a b -- a b a b )
F(" : 2DROP DROP DROP ;")  // ( a b -- )
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
N(0,QABORT,      "?ABORT",     if (POP()) ABORT("?ABORT")                     )

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
N(0,CELLS,       "CELLS",      TOS *= sizeof(_t)                              )
N(0,BL,          "BL",         PUSH(BLANK_CHAR)                               )
N(0,ALLOT,       "ALLOT",      DP += POP()                                    )
N(0,IMMEDIATE,   "IMMEDIATE",  LATEST->immed = 1                              )
N(0,COMMA,       ",",          COMMA(POP())                                   )
N(0,HERE,        "HERE",       PUSH(DP)                                       )
N(0,DEPTH,       "DEPTH",      PUSH(SP - u->parmstack)                        )
N(0,LITERAL,     "LITERAL",    COMMA((_t) XT(DO_LITERAL)); COMMA(POP())       )

// gets the next word from the input stream and adds a dictionary entry for it
N(0,CREATE, "CREATE", {
    PUSH(BLANK_CHAR);
    WORD(u);

    word_hdr_t * hdr = (word_hdr_t *) DP;
    DP += sizeof(word_hdr_t);

    strncpy(hdr->name, (const char *) POP(), sizeof(hdr->name));
    hdr->do_does = 0;
    hdr->codeptr = &&DO_ENTER;
    hdr->prev = LATEST;
    LATEST = hdr;
})

// DOES> sets it up so that the defined compiler word includes an action, which
// is used for the words it creates at runtime.
N(1,META_DOES,  "DOES>",       COMMA((_t) XT(DOES))  )
N(0,DOES,       "(DOES>)",     LATEST->do_does = 1;
                               LATEST->codeptr = (void *) IP;
                               goto DO_EXIT
                           )
// the classic definition of CONSTANT
F(" : CONSTANT   CREATE , DOES> @ ; IMMEDIATE")
F(" : VARIABLE   CREATE 0 , DOES> ; IMMEDIATE")
F(" : ( 41 WORD DROP ; IMMEDIATE")  // ( inline comments )  41 == ')'

//  ' (TICK)   ( -- xt )   quotes the next word in the input stream
F(" : '    BL WORD FIND 0= ?ABORT ;")
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
F(" : POSTPONE  ' LITERAL ['] , , ; IMMEDIATE")

// immediate (compile-time) TICK
N(1,COMPILETICK, "[']",        PUSH(BLANK_CHAR);
                               WORD(u);
                               FIND(u);
                               if (POP() == 0) { _ABORT(u); }
                               goto LITERAL
                           )

/* 
   OFFSET   ( &ptr[a] &ptr[b] -- a-b )  delta in cells between two pointers
   >MARK    ( -- &src_ip )  holds a slot for a forward branch offset.
                              use immediately after a POSTPONE (BRANCH*)
   >RESOLVE ( &src_ip -- )  fills the formerly >MARKed slot with the delta
   <MARK    ( -- &tgt_ip )  saves a branch target for <RESOLVE
   <RESOLVE ( &tgt_ip -- )  fills HERE with the <MARKed target.
                              use immediately after a POSTPONE (BRANCH*)
 */
F(" : OFFSET   - 1 CELLS / 1 - ;")
F(" : >MARK     HERE  1 CELLS ALLOT ;")
F(" : >RESOLVE  HERE OVER OFFSET SWAP ! ;")
F(" : <MARK     HERE ;")
F(" : <RESOLVE  HERE OFFSET , ;")

// BEGIN/AGAIN/UNTIL are straight-forward
F(" : BEGIN  <MARK ; IMMEDIATE")
F(" : AGAIN  POSTPONE (BRANCH)   <RESOLVE ; IMMEDIATE")
F(" : UNTIL  POSTPONE (BRANCHZ)  <RESOLVE ; IMMEDIATE")

// as are IF/ELSE/THEN
F(" : IF    POSTPONE (BRANCHZ) >MARK ; IMMEDIATE")
F(" : ELSE  POSTPONE (BRANCH)  >MARK SWAP >RESOLVE ; IMMEDIATE")
F(" : THEN  >RESOLVE ; IMMEDIATE")

//  DO/LOOP stores the loop counter and limit on the return stack
F(": DO    POSTPONE 2>R <MARK ; IMMEDIATE")
F(": LOOP  POSTPONE (LOOP) POSTPONE (BRANCHZ) <RESOLVE POSTPONE 2RDROP ; IMMEDIATE")

// (TRY) creates an exception frame on the return stack
// CATCH starts the exception handler
// FINALLY resolves the resume point
F(": TRY   POSTPONE (TRY)  >MARK ; IMMEDIATE")
F(": CATCH POSTPONE (CATCH) POSTPONE (BRANCH) >MARK SWAP >RESOLVE ; IMMEDIATE")
F(": FINALLY  >RESOLVE ; IMMEDIATE")



N(0,DO_TRY,     "(TRY)",       acc = (_t) *IP++;
                               RPUSH(SP);
                               RPUSH(IP + acc);
                               RPUSH(FRAME);
                               FRAME = RP
                           )
N(0,DO_CATCH,   "(CATCH)",     assert(FRAME != NULL);
                               RP = FRAME;
                               FRAME = RPOP();
                               RP -= 2; // drop catch block IP and saved SP
                           )
N(0,THROW,      "THROW",       RP = FRAME;
                               FRAME = RPOP();
                               IP = RPOP(); 
                               SP = RPOP();
                           )

N(0,PRINT,       ".",          printf("%d ", (int) POP())  )
N(0,TYPE,        "TYPE",       puts((const char *) POP())  )
N(0,EMIT,        "EMIT",       putchar((char) POP())       )
N(0,BYE,         "BYE",        exit(0)                     )

#define C(CTOK, FTOK) N(0, CTOK, FTOK, if (CTOK(u) <= 0) return)
C(_ABORT,      "ABORT" )
C(NUMBER,      "NUMBER")
C(FIND,        "FIND"  )
C(WORD,        "WORD"  )
C(WORDS,       "WORDS" )
C(PRINTSTACK,  ".S"    )
C(PRINTWORD,   ".W" )
F(" : SEE  ' .W ;") // SEE WORD  to show the definition of a word
#undef C

