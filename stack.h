#include <stdint.h>
struct stack {
    uint16_t contents[1000];
    int size; //top index is the index of the first field that is unpopulated.
};

uint16_t pop(struct stack *stack);
void push(struct stack *stack, uint16_t value);

