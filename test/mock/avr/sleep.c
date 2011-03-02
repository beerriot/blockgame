/* sleep.c: Mock definitions for testing */

#include <inttypes.h>
#include <stdio.h>

void sleep_cpu() {
    printf("%s:%d\n", __FILE__, __LINE__);
}
void sleep_enable() {
    printf("%s:%d\n", __FILE__, __LINE__);
}
void sleep_disable() {
    printf("%s:%d\n", __FILE__, __LINE__);
}
