
#include <stdint.h>
#include <stdio.h>
typedef int64_t _t;

_t data[32];
_t *SP = data;
_t TOS;

#define LITERAL(N) ((_t) N)//  ((void *) N)
#define LABEL(N) ((_t) &&N)

int main()
{
    _t boot[] = { LITERAL(2), LITERAL(4), 
                  LABEL(ADD), LABEL(PRINT), LABEL(EXIT) };
    const _t *IP = boot;
    _t op;
NEXT:
    op = *IP++;

    if ((void *) op >= &&NEXT && (void *) op <= &&EXIT) {
        goto *op;
    }

    // just push the number
    *++SP = TOS;
    TOS = op;
    goto NEXT;

ADD:
    TOS += *SP--;
    goto NEXT;

PRINT:
    printf("%d ", (int) TOS);
    TOS = *SP--;
    goto NEXT;
    
EXIT:
    return 0;
}
