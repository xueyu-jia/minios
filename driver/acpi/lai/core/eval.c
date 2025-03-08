/*
 * Lightweight AML Interpreter
 * Copyright (C) 2018-2023 The lai authors
 */

#include <driver/acpi/lai/core.h>

#include "aml_opcodes.h"
#include "eval.h"
#include "exec_impl.h"
#include "libc.h"
#include "ns_impl.h"

static uint32_t bswap32(uint32_t dword) {
    return (uint32_t)((dword >> 24) & 0xFF) | ((dword << 8) & 0xFF0000) | ((dword >> 8) & 0xFF00) |
           ((dword << 24) & 0xFF000000);
}

static uint8_t char_to_hex(char character) {
    if (character <= '9')
        return character - '0';
    else if (character >= 'A' && character <= 'F')
        return character - 'A' + 10;
    else if (character >= 'a' && character <= 'f')
        return character - 'a' + 10;

    return 0;
}

static char hex_to_char(uint8_t hex) {
    return hex < 0xa ? '0' + hex : hex - 10 + 'A';
}

int lai_is_name(char character) {
    if ((character >= '0' && character <= '9') || (character >= 'A' && character <= 'Z') ||
        character == '_' || character == ROOT_CHAR || character == PARENT_CHAR ||
        character == MULTI_PREFIX || character == DUAL_PREFIX)
        return 1;

    else
        return 0;
}

void lai_eisaid(lai_variable_t *object, const char *id) {
    size_t n = lai_strlen(id);
    if (lai_strlen(id) != 7) {
        if (lai_create_string(object, n) != LAI_ERROR_NONE)
            lai_panic("could not allocate memory for string");
        memcpy(lai_exec_string_access(object), id, n);
        return;
    }

    // convert a string in the format "UUUXXXX" to an integer
    // "U" is an ASCII character, and "X" is an ASCII hex digit
    object->type = LAI_INTEGER;

    uint32_t out = 0;
    out |= ((id[0] - 0x40) << 26);
    out |= ((id[1] - 0x40) << 21);
    out |= ((id[2] - 0x40) << 16);
    out |= char_to_hex(id[3]) << 12;
    out |= char_to_hex(id[4]) << 8;
    out |= char_to_hex(id[5]) << 4;
    out |= char_to_hex(id[6]);

    out = bswap32(out);
    object->integer = (uint64_t)out & 0xFFFFFFFF;
}

void lai_eisaid_to_str(uint32_t eisaid, char *buf) {
    const uint32_t val = bswap32(eisaid);
    buf[0] = 0x40 + ((val >> 26) & 0x1f);
    buf[1] = 0x40 + ((val >> 21) & 0x1f);
    buf[2] = 0x40 + ((val >> 16) & 0x1f);
    buf[3] = hex_to_char((val >> 12) & 0xf);
    buf[4] = hex_to_char((val >> 8) & 0xf);
    buf[5] = hex_to_char((val >> 4) & 0xf);
    buf[6] = hex_to_char((val >> 0) & 0xf);
    buf[7] = 0;
}
