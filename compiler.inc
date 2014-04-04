
// compiler words
N(0,BL,       "BL",      ( -- ),       PUSH(BLANK_CHAR)                       )
N(0,CELLS,    "CELLS",   ( n -- n*s ), TOS *= sizeof(_t)                      )
N(0,CELLPLUS, "CELL+",   ( p -- p++ ), TOS += sizeof(_t)                      )
N(0,ALLOT,    "ALLOT",   ( n -- ),     DP += POP()                            )
N(0,HERE,     "HERE",    ( -- dp ),    PUSH(DP)                               )
N(0,ALIGN,    "ALIGN",   ( -- ),       acc = (_t) DP & (sizeof(_t)-1);
                                       if (acc) DP += sizeof(_t) - acc        )
N(0,ALIGNED,  "ALIGNED", ( a -- b ),   acc = TOS & (sizeof(_t)-1); 
                                       if (acc) TOS += sizeof(_t) - acc       )
N(0,COMMA,    ",",       ( n -- ),     COMMA(POP())                           )
N(0,LITERAL,  "LITERAL", ( n -- ),     COMMA((_t) XT(DO_LITERAL)); COMMA(POP())   )
N(0,IMMEDIATE,"IMMEDIATE", ( -- ),     LATEST->immed = 1                      )
N(1,RECURSE,  "RECURSE", ( -- ),       COMMA((_t) LATEST)                     )
N(1,CLRSTATE, "[",       ( -- ),       STATE = 0                              )
N(0,SETSTATE, "]",       ( -- ),       STATE = 1                              )

// DOES> sets it up so that the defined compiler word includes an action, which
// is used for the words it creates at runtime.
N(1,META_DOES,"DOES>",   ( -- ),  COMMA((_t) XT(DOES))  )
N(0,DOES,     "(DOES>)", ( -- ),  LATEST->do_does = 1;
                                  LATEST->codeptr = (void *) IP;
                                  goto EXIT )

N(0,TO_BODY, ">BODY", ( xt -- ptr ), TOS = (_t) ((word_hdr_t *) TOS)->body   )

// ( inline comments )  41 == ')'
F(": ( 41 WORD DROP ; IMMEDIATE")

// the classic definition of CONSTANT
F(": CONSTANT   CREATE , DOES> @ ;")
F(": VARIABLE   CREATE 0 , DOES> ;")

//  ' (TICK)   ( -- xt )   quotes the next word in the input stream
F(": '    BL WORD FIND 0= ?ABORT ;")
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

// immediate (compile-time) TICK, compiles succeeding token as a literal
N(1,COMPILETICK, "[']",  ( -- ),
        PUSH(BLANK_CHAR);  WORD(u);
        FIND(u);  if (POP() == 0) { _ABORT(u); }
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
F(": OFFSET    - 1 CELLS / 1 - ;")
F(": >MARK     HERE  1 CELLS ALLOT ;")
F(": >RESOLVE  HERE OVER OFFSET SWAP ! ;")
F(": <MARK     HERE ;")
F(": <RESOLVE  HERE OFFSET , ;")

// BEGIN/AGAIN/UNTIL are straight-forward
F(": BEGIN  <MARK ; IMMEDIATE")
F(": AGAIN  POSTPONE (BRANCH)   <RESOLVE ; IMMEDIATE")
F(": UNTIL  POSTPONE (BRANCHZ)  <RESOLVE ; IMMEDIATE")

// as are IF/ELSE/THEN
F(": IF    POSTPONE (BRANCHZ) >MARK ; IMMEDIATE")
F(": ELSE  POSTPONE (BRANCH)  >MARK SWAP >RESOLVE ; IMMEDIATE")
F(": THEN  >RESOLVE ; IMMEDIATE")

//  DO/LOOP stores the loop counter and limit on the return stack
F(": DO    POSTPONE 2>R <MARK ; IMMEDIATE")
F(": LOOP  POSTPONE (LOOPINC) POSTPONE (BRANCHZ) <RESOLVE POSTPONE 2RDROP ; IMMEDIATE")

// (TRY) creates an exception frame on the return stack
// CATCH starts the exception handler
// FINALLY resolves the resume point
F(": TRY   POSTPONE (TRY)  >MARK ; IMMEDIATE")
F(": CATCH POSTPONE (UNTRY) POSTPONE (BRANCH) >MARK SWAP >RESOLVE ; IMMEDIATE")
F(": FINALLY  >RESOLVE ; IMMEDIATE")

