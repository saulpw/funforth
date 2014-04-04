
// interpreter words

C(WORD,        "WORD",   ( ch -- str )                 )

C(WORDS,       "WORDS",  ( -- )                        )
C(PRINTSTACK,  ".S",     ( -- )                        )
C(PRINTWORD,   ".W",     ( xt -- )                     )

N(0,PRINT,     ".",      ( i -- ),    printf("%d ", (int) POP())  )
N(0,TYPE,      "TYPE",   ( str -- ),  puts((const char *) POP())  )
N(0,EMIT,      "EMIT",   ( ch -- ),   putchar((char) POP())       )

F(": CR  10 EMIT 13 EMIT ;")
F(": SPACE  BL EMIT ;")
F(": SPACES 0 DO SPACE LOOP ;") // 8 SPACES
F(": SEE  ' .W ;") // SEE WORD  to show the definition of a word

N(0,INTERPRET_WORD, "", ( -- ),
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
    })
