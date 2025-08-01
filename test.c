#include <stdint.h>
#include <stdio.h>
#include "stack.h"
#include <stdbool.h>
#include <stdlib.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>

int main() {
    uint8_t number = 200;
    uint8_t hundred = number / 100;
    number = number % 100;
    uint8_t ten = number / 10;
    number = number % 10;
    uint8_t one = number;
    printf("%d%d%d", hundred, ten, one);
    printf("\a");
}
