#include "funforth.h"

int main()
{
    const char tests[] = 

" : pass      42 . ;"
" : fail      -1 . ;"
" : testif       23 42 = IF fail ELSE pass THEN pass ;"
" : testinfloop  1 BEGIN DUP . 1 + AGAIN ;"
" : testuntil    1 BEGIN DUP . 1 + DUP 10 = UNTIL pass ;"
" : testdoloop  10 0 DO I . LOOP pass ;"
" : raise     -42 THROW fail ;"
" : testexc   TRY pass CATCH fail FINALLY pass"
"             TRY raise fail CATCH -42 = IF pass ELSE fail THEN FINALLY pass ;"
" : test*/   42 1000000000 * 1000000000 /  DUP . 42 = IF fail ELSE pass THEN"
"            42 1000000000   1000000000 */ DUP . 42 = IF pass ELSE fail THEN ;"
" WORDS  CR 5 SPACES SEE TRY CR testif  CR testuntil  CR testdoloop  CR testexc  CR test*/ CR BYE "
;

    user_t user;
    
    return engine(&user, tests);
}

