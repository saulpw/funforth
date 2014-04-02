#include "funforth.h"

int main()
{
    const char tests[] = 

" : pass      42 . ;"
" : fail      -1 . ;"
" : testif       23 42 = IF fail ELSE pass THEN pass ; "
" : testinfloop  1 BEGIN DUP . 1 + AGAIN ;"
" : testuntil    1 BEGIN DUP . 1 + DUP 10 = UNTIL ;"
" : testdoloop  10 0 DO I . LOOP ;"
" : raise     -1 THROW fail ;"
" : testexc   TRY pass CATCH fail FINALLY pass"
"             TRY raise fail CATCH pass FINALLY pass ;"

" WORDS  SEE try  testif  testuntil  testdoloop  testexc  BYE"
;

    user_t user;
    
    return engine(&user, tests);
}

