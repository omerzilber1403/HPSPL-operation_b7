#include "bitwise.h"

#include <string.h>

#define CHANNEL_MAX 16
#define FLAG_CONFIRMED 0x01u
#define FLAG_CANCELLED 0x02u

/* Value of one hex/decimal/binary digit, or -1 if c is not a digit. */
static int digit_value(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

/* Little-Endian: the low byte comes first. */
static uint16_t read_le16(const uint8_t *bytes)
{
    return (uint16_t)((unsigned)bytes[0] | ((unsigned)bytes[1] << 8));
}

int parse_base_number(const char *text, unsigned base, uint32_t *out)
{
    uint32_t value = 0;

    if (text == NULL || out == NULL || (base != 2 && base != 10 && base != 16))
        return 0;
    if (text[0] == '0' &&
        ((base == 2 && (text[1] == 'b' || text[1] == 'B')) ||
         (base == 16 && (text[1] == 'x' || text[1] == 'X'))))
        text += 2;
    if (*text == '\0')
        return 0;

    for (; *text != '\0'; text++) {
        int digit = digit_value(*text);

        if (digit < 0 || (unsigned)digit >= base)
            return 0;
        /* check before multiplying so value never wraps past UINT32_MAX */
        if (value > (UINT32_MAX - (uint32_t)digit) / base)
            return 0;
        value = value * base + (uint32_t)digit;
    }
    *out = value;
    return 1;
}

int format_binary(uint32_t value, unsigned width, char *out, size_t capacity)
{
    if (out == NULL || capacity == 0)
        return 0;
    out[0] = '\0';
    /* shifting a uint32_t by 32 is undefined, so width 32 always fits */
    if (width < 1 || width > 32 || capacity < (size_t)width + 1 ||
        (width < 32 && (value >> width) != 0))
        return 0;

    for (unsigned i = 0; i < width; i++)
        out[i] = ((value >> (width - 1 - i)) & 1u) ? '1' : '0';
    out[width] = '\0';
    return 1;
}

int decode_twos_complement(uint32_t raw, unsigned bits, int32_t *out)
{
    if (out == NULL || bits < 2 || bits > 16 || (raw >> bits) != 0)
        return 0;
    /* sign bit on: value is raw - 2^bits (computed in int32_t, no overflow) */
    if (raw & (1u << (bits - 1)))
        *out = (int32_t)raw - (int32_t)(1u << bits);
    else
        *out = (int32_t)raw;
    return 1;
}

int encode_twos_complement(int32_t value, unsigned bits, uint32_t *out)
{
    if (out == NULL || bits < 2 || bits > 16)
        return 0;
    int32_t limit = (int32_t)(1u << (bits - 1)); /* 2^(bits-1) */
    if (value < -limit || value >= limit)
        return 0;
    *out = value < 0 ? (uint32_t)(value + 2 * limit) : (uint32_t)value;
    return 1;
}

int unpack_packet(uint16_t word, PacketFields *out)
{
    if (out == NULL)
        return 0;
    out->type = (uint8_t)((word >> 12) & 0x0Fu);
    out->index = (uint8_t)((word >> 8) & 0x0Fu);
    out->payload = (uint8_t)(word & 0xFFu);
    return 1;
}

int pack_packet(const PacketFields *fields, uint16_t *out)
{
    if (fields == NULL || out == NULL || fields->type > 15 || fields->index > 15)
        return 0;
    *out = (uint16_t)(((unsigned)fields->type << 12) |
                      ((unsigned)fields->index << 8) | fields->payload);
    return 1;
}

int decode_channel(const PacketFields *packets, size_t count,
                   uint8_t type, uint8_t key,
                   char *out, size_t capacity)
{
    uint8_t message[CHANNEL_MAX];
    int seen[CHANNEL_MAX] = {0};
    size_t length = 0;

    if (out == NULL || capacity == 0)
        return 0;
    out[0] = '\0';
    if (packets == NULL)
        return 0;

    for (size_t i = 0; i < count; i++) {
        uint8_t index = packets[i].index;

        if (packets[i].type != type)
            continue;
        if (index >= CHANNEL_MAX || seen[index])
            return 0;
        seen[index] = 1;
        message[index] = packets[i].payload ^ key;
        length++;
    }
    /* length distinct indices with 0..length-1 all present means no holes */
    if (length < 2 || length > capacity)
        return 0;
    for (size_t i = 0; i < length; i++) {
        if (!seen[i])
            return 0;
    }
    if (message[length - 1] != '\0')
        return 0;
    for (size_t i = 0; i + 1 < length; i++) {
        if (message[i] < 0x20 || message[i] > 0x7E)
            return 0;
    }

    for (size_t i = 0; i < length; i++)
        out[i] = (char)message[i];
    return 1;
}

int parse_meeting_record(const uint8_t raw[10], MeetingRecord *out)
{
    MeetingRecord empty = {0};
    MeetingRecord record;
    uint8_t checksum = 0;
    int32_t adjust;

    if (out != NULL)
        *out = empty;
    if (raw == NULL || out == NULL)
        return 0;
    for (int i = 0; i < 9; i++)
        checksum ^= raw[i];

    record.meeting_id = read_le16(&raw[0]);
    record.base_minute = read_le16(&raw[2]);
    record.location_code = read_le16(&raw[4]);
    record.flags = raw[6];
    decode_twos_complement(raw[8], 8, &adjust); /* any byte is valid at 8 bits */
    record.minute_adjust = (int16_t)adjust;

    int32_t final_minute = (int32_t)record.base_minute + adjust;
    if (record.meeting_id == 0 || record.location_code == 0 ||
        record.base_minute > 1439 || (record.flags & ~(FLAG_CONFIRMED | FLAG_CANCELLED)) ||
        record.flags == (FLAG_CONFIRMED | FLAG_CANCELLED) ||
        raw[7] != 0 || checksum != raw[9] ||
        final_minute < 0 || final_minute > 1439)
        return 0;

    record.final_minute = (uint16_t)final_minute;
    *out = record;
    return 1;
}

int find_confirmed_meeting(FILE *input, MeetingRecord *out)
{
    MeetingRecord empty = {0};
    MeetingRecord record;
    MeetingRecord found = {0};
    uint8_t raw[10];
    size_t read_count;
    int matches = 0;

    if (out == NULL)
        return 0;
    *out = empty;
    if (input == NULL)
        return 0;

    while ((read_count = fread(raw, 1, sizeof(raw), input)) == sizeof(raw)) {
        /* a valid record never has both flags, so == CONFIRMED means active */
        if (parse_meeting_record(raw, &record) && record.flags == FLAG_CONFIRMED) {
            found = record;
            matches++;
        }
    }
    /* leftover bytes = record cut in the middle */
    if (read_count != 0 || ferror(input) || matches != 1)
        return 0;
    *out = found;
    return 1;
}

int resolve_location(const PacketFields *packets, size_t count,
                     uint8_t key, uint16_t location_code,
                     char *out, size_t capacity)
{
    char message[CHANNEL_MAX];
    uint32_t code = 0;

    if (out == NULL || capacity == 0)
        return 0;
    out[0] = '\0';
    /* "LOC:HHHH=" is 9 chars, PLACE needs at least one more */
    if (!decode_channel(packets, count, 0x0E, key, message, sizeof(message)) ||
        strlen(message) < 10 || strncmp(message, "LOC:", 4) != 0 || message[8] != '=')
        return 0;

    /* digit by digit: parse_base_number would also accept a "0x" prefix here */
    for (int i = 4; i < 8; i++) {
        int digit = digit_value(message[i]);

        if (digit < 0)
            return 0;
        code = code * 16 + (uint32_t)digit;
    }
    if (code != location_code)
        return 0;

    const char *place = &message[9];
    for (size_t i = 0; place[i] != '\0'; i++) {
        if (place[i] < 'A' || place[i] > 'Z')
            return 0;
    }
    if (strlen(place) + 1 > capacity)
        return 0;
    strcpy(out, place);
    return 1;
}
