#pragma once
#include <stdint.h>
static inline uint32_t __crc32b(uint32_t a, uint8_t b){return a^b;}
static inline uint32_t __crc32h(uint32_t a, uint16_t b){return a^b;}
static inline uint32_t __crc32w(uint32_t a, uint32_t b){return a^b;}
static inline uint32_t __crc32d(uint32_t a, uint64_t b){return a^(uint32_t)b;}
static inline uint32_t __crc32cb(uint32_t a, uint8_t b){return a^b;}
static inline uint32_t __crc32ch(uint32_t a, uint16_t b){return a^b;}
static inline uint32_t __crc32cw(uint32_t a, uint32_t b){return a^b;}
static inline uint32_t __crc32cd(uint32_t a, uint64_t b){return a^(uint32_t)b;}
