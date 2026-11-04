#include "decoder.h"

#include <stdio.h>
#include <string.h>

int decode_message(const char *encoded, int shift, char *decoded, size_t capacity)
{
    size_t len = 0;
    size_t i;

    if (encoded == NULL || decoded == NULL || shift < 0 || shift > 25)
    {
        return 0;
    }

    /* Validate characters first so decoded is never half-written on bad input. */
    for (len = 0; encoded[len] != '\0'; len++)
    {
        char c = encoded[len];
        if (!((c >= 'A' && c <= 'Z') || c == '_'))
        {
            return 0;
        }
    }

    if (capacity < len + 1)
    {
        return 0;
    }

    /* Reverse the string while undoing the cyclic shift on letters. */
    for (i = 0; i < len; i++)
    {
        char c = encoded[len - 1 - i];
        if (c != '_')
        {
            c = (char)('A' + (c - 'A' - shift + 26) % 26);
        }
        decoded[i] = c;
    }
    decoded[len] = '\0';

    return 1;
}

int calculate_checksum(const char *message)
{
    int sum = 0;
    int i;

    if (message == NULL)
    {
        return -1;
    }
    for (i = 0; message[i] != '\0'; i++)
    {
        sum += message[i];
    }
    return sum % 97;
}

int process_transmission(const char *line)
{
    char copy[256];
    char decoded[161];
    char *fields[4];
    char *token;
    int bars = 0;
    int count = 0;
    int key = 0;
    int checksum;
    int i;

    if (line == NULL || strlen(line) >= sizeof(copy))
    {
        printf("[INVALID]\n");
        return 0;
    }

    /* strtok skips empty fields, so require exactly 3 '|' and 4 tokens. */
    for (i = 0; line[i] != '\0'; i++)
    {
        if (line[i] == '|')
        {
            bars++;
        }
    }
    strcpy(copy, line);
    token = strtok(copy, "|");
    while (token != NULL && count < 4)
    {
        fields[count] = token;
        count++;
        token = strtok(NULL, "|");
    }
    if (bars != 3 || count != 4)
    {
        printf("[INVALID]\n");
        return 0;
    }

    /* SENDER: 1-24 chars of A-Z or _ */
    if (strlen(fields[0]) > 24)
    {
        printf("[INVALID]\n");
        return 0;
    }
    for (i = 0; fields[0][i] != '\0'; i++)
    {
        if (!((fields[0][i] >= 'A' && fields[0][i] <= 'Z') || fields[0][i] == '_'))
        {
            printf("[INVALID]\n");
            return 0;
        }
    }

    /* KEY: digits only, value 0-25 */
    for (i = 0; fields[1][i] != '\0'; i++)
    {
        if (fields[1][i] < '0' || fields[1][i] > '9')
        {
            printf("[INVALID]\n");
            return 0;
        }
        key = key * 10 + (fields[1][i] - '0');
        if (key > 25)
        {
            printf("[INVALID]\n");
            return 0;
        }
    }

    /* CHECKSUM: exactly 2 digits, value 0-96 */
    if (strlen(fields[2]) != 2 || fields[2][0] < '0' || fields[2][0] > '9' ||
        fields[2][1] < '0' || fields[2][1] > '9')
    {
        printf("[INVALID]\n");
        return 0;
    }
    checksum = (fields[2][0] - '0') * 10 + (fields[2][1] - '0');
    if (checksum > 96)
    {
        printf("[INVALID]\n");
        return 0;
    }

    /* MESSAGE: 1-160 chars; decode_message rejects bad characters. */
    if (strlen(fields[3]) > 160 || !decode_message(fields[3], key, decoded, sizeof(decoded)))
    {
        printf("[INVALID]\n");
        return 0;
    }

    if (calculate_checksum(decoded) != checksum)
    {
        printf("[INVALID]\n");
        return 0;
    }

    printf("[%s] %s\n", fields[0], decoded);
    return 1;
}

int decode_advanced(const char *encoded, const int *keys, size_t key_count, char *decoded, size_t capacity)
{
    /* Optional TODO. */
    (void)encoded;
    (void)keys;
    (void)key_count;
    (void)decoded;
    (void)capacity;
    return 0;
}

