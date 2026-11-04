#include "decoder.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    char line[256];
    int recovered = 0;
    int rejected = 0;
    int len;

    while (fgets(line, sizeof(line), stdin) != NULL)
    {
        /* Remove trailing \n and \r, if present. */
        len = (int)strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
        {
            line[len - 1] = '\0';
            len--;
        }

        if (process_transmission(line))
        {
            recovered++;
        }
        else
        {
            rejected++;
        }
    }

    printf("Recovered: %d | Rejected: %d\n", recovered, rejected);
    return 0;
}
