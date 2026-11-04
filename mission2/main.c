#include "investigation.h"

int main(void)
{
    Agent working[16];
    size_t capacity = sizeof working / sizeof working[0];

    return run_investigation(working, capacity);
}
