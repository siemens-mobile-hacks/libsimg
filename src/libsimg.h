#pragma once

#include <stdint.h>
#include <stddef.h>

enum SIMGType {
    SIMG_TYPE_WB           = 0x1,
    SIMG_TYPE_RGB332       = 0x5,
    SIMG_TYPE_ARGB4444     = 0x7,
    SIMG_TYPE_RGB565       = 0x8,
    SIMG_TYPE_ARGB8888     = 0xA,
    SIMG_TYPE_RLE_RGB332   = 0x85,
    SIMG_TYPE_RLE_RGB565   = 0x88,
    SIMG_TYPE_RLE_ARGB8888 = 0x8A,
};

int simg_get_bpp_by_type(unsigned int type);
uint8_t *simg_unpack_rle(const uint8_t *rle_bitmap, uint16_t width, uint16_t height, int bpp);
uint8_t *simg_unpack_1bit_raw(const uint8_t *data, uint16_t width, uint16_t height);

int simg_write_png(const char *filename, const uint8_t *pixels, uint16_t width, uint16_t height, int type);

uint32_t simg_addr_to_offset(const uint8_t *p);
uint8_t *simg_find_pit(const uint8_t *buffer, size_t size, int platform);
