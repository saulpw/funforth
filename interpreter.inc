
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

