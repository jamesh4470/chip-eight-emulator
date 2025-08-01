#include "stack.h"
#include <stdint.h>
#include <stdio.h>

uint16_t pop(struct stack *stack) {
    uint16_t value = stack->contents[stack->size - 1];
    stack->contents[stack->size - 1] = 0;
    stack->size--;
    return value;
}

void push(struct stack *stack, uint16_t value) {
    stack->contents[stack->size] = value;
    stack->size++;
}

