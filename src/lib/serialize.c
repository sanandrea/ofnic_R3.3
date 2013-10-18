/* Copyright 2013 (C) Universita' di Roma La Sapienza
 *
 * This file is part of OFNIC Uniroma GE.
 *
 * OFNIC is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OFNIC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OFNIC.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "serialize.h"

uint8_t serialize_uint16(uint16_t value, char *dest, uint16_t maxlen)
{
    uint16_t val_nw_order;
    char* valPtr;
    uint8_t i;

    val_nw_order = value;

    /* Abort: dest too small */
    if (maxlen < sizeof (uint16_t)) return 0;

    valPtr = (char*) & val_nw_order;
    for (i = 0; i < sizeof (uint16_t); i++) {
        dest[i] = valPtr[i];
    }
    return sizeof (uint16_t);
}

uint8_t serialize_uint32(uint32_t value, char *dest, uint16_t maxlen)
{
    uint32_t val_nw_order = value;
    char* valPtr;
    uint8_t i;

    /* Abort: dest too small */
    if (maxlen < sizeof (uint32_t)) return 0;

    valPtr = (char*) & val_nw_order;
    for (i = 0; i < sizeof (uint32_t); i++) {
        dest[i] = valPtr[i];
    }
    return sizeof (uint32_t);
}

uint8_t serialize_uint64(uint64_t value, char *dest, uint16_t maxlen)
{
    char* valPtr;
    uint8_t i;

    /* Abort: dest too small */
    if (maxlen < sizeof (uint64_t)) return 0;

    valPtr = (char*) & value;
    for (i = 0; i < sizeof (uint32_t); i++) {
        dest[i] = valPtr[i];
    }
    return sizeof (uint64_t);
}

uint8_t unserialize_uint16(const char* src, uint16_t len, uint16_t * value)
{
    uint16_t val_nw_order;
    char* valPtr;
    uint8_t i;

    /* Abort: src too small */
    if (sizeof (uint16_t) > len) return 0;

    valPtr = (char*) & val_nw_order;
    for (i = 0; i < sizeof (uint16_t); i++) {
        valPtr[i] = src[i];
    }

    *value = val_nw_order;

    return sizeof (uint16_t);
}

uint8_t unserialize_uint32(const char* src, uint16_t len, uint32_t * value)
{
    uint32_t val_nw_order;
    char* valPtr;
    uint8_t i;

    /* Abort: src too small */
    if (sizeof (uint32_t) > len) return 0;

    valPtr = (char*) & val_nw_order;
    for (i = 0; i < sizeof (uint32_t); i++) {
        valPtr[i] = src[i];
    }

    *value = val_nw_order;

    return sizeof (uint32_t);
}
uint8_t unserialize_uint64(const char* src, uint16_t len, uint64_t * value)
{
    uint32_t val_nw_order;
    char* valPtr;
    uint8_t i;

    /* Abort: src too small */
    if (sizeof (uint64_t) > len) return 0;

    valPtr = (char*) & val_nw_order;
    for (i = 0; i < sizeof (uint64_t); i++) {
        valPtr[i] = src[i];
    }

    *value = val_nw_order;

    return sizeof (uint64_t);
}
