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
#ifndef MWA_SERIALIZE_H
#define MWA_SERIALIZE_H


#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <string.h>

uint8_t b;
/**
 * @brief converts uint16 to network byte order, and write to the dest
 *
 * @param value - to be written
 * @param dest - where to write serialized value
 *
 * @return number of written bytes
 *         FALSE (0) - error occured
 */
uint8_t serialize_uint16(uint16_t value, char *dest, uint16_t maxlen);

/**
 * @brief extracts uin16 value (host byte order) for byte stream (network byte
 *        order)
 *
 * @param src - byte stream that holds the value
 * @param len - lenght of the stream
 * @param value - return value, 16-bit value from the stream
 *
 * @return number of written bytes
 *         FALSE (0) - error occured
 */
uint8_t unserialize_uint16(const char* src, uint16_t len, uint16_t* value);


/**
 * @brief converts uint32 to network byte order, and write to the dest
 *
 * @param value - to be written
 * @param dest - where to write serialized value
 *
 * @return number of written bytes
 *         FALSE (0) - error occured
 */
uint8_t serialize_uint32(uint32_t value, char *dest, uint16_t maxlen);

/**
 * @brief extracts uin32 value (host byte order) for byte stream (network byte
 *        order)
 *
 * @param src - byte stream that holds the value
 * @param len - lenght of the stream
 * @param value - return value, 32-bit value from the stream
 *
 * @return number of written bytes
 *         FALSE (0) - error occured
 */
uint8_t unserialize_uint32(const char* src, uint16_t len, uint32_t* value);

uint8_t serialize_uint64(uint64_t value, char *dest, uint16_t maxlen);
uint8_t unserialize_uint64(const char* src, uint16_t len, uint64_t* value);

#ifdef __cplusplus
}
#endif

#endif
