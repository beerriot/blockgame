/* interrupt.c: Mock definitions for testing */

#include <inttypes.h>
#include <stdio.h>

void cli() {
    printf("%s:%d\n", __FILE__, __LINE__);
}
void sei() {
    printf("%s:%d\n", __FILE__, __LINE__);
}

