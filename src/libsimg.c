#include <string.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "libsimg.h"

#define RGB332_TRANSPARENT_COLOR 0xC0
#define RGB565_TRANSPARENT_COLOR 0xE000

static void RGB332_to_RGBA8888(uint8_t pixel, uint8_t *dest) {
    if (pixel == RGB332_TRANSPARENT_COLOR) {
        memset(dest, 0, 4);
        return;
    }

    uint8_t r = (pixel >> 5) & 0x07;
    uint8_t g = (pixel >> 2) & 0x07;
    uint8_t b = (pixel & 0x03);
    r = (r << 5) | (r << 2) | (r >> 1);
    g = (g << 5) | (g << 2) | (g >> 1);
    b = (b << 6) | (b << 4) | (b << 2) | b;
    dest[0] = r;
    dest[1] = g;
    dest[2] = b;
    dest[3] = 0xFF;
}

static void RGB565_to_RGBA8888(uint16_t pixel, uint8_t *dest) {
    if (pixel == RGB565_TRANSPARENT_COLOR) {
        memset(dest, 0, 4);
        return;
    }
    uint8_t r = (pixel >> 11) & 0x1F;
    uint8_t g = (pixel >> 5) & 0x3F;
    uint8_t b = pixel & 0x1F;
    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);
    dest[0] = r;
    dest[1] = g;
    dest[2] = b;
    dest[3] = 0xFF;
}

static void ARGB444_to_RGBA8888(uint16_t pixel, uint8_t *dest) {
    uint8_t r = (pixel >> 8) & 0x0F;
    uint8_t g = (pixel >> 4) & 0x0F;
    uint8_t b = pixel & 0x0F;
    r = (r << 4) | r;
    g = (g << 4) | g;
    b = (b << 4) | b;
    dest[0] = r;
    dest[1] = g;
    dest[2] = b;
    dest[3] = 0xFF;
}

static void ARGB8888_to_RGBA8888(const uint8_t *argb8888, uint8_t *dest) {
     dest[0] = argb8888[2];
     dest[1] = argb8888[1];
     dest[2] = argb8888[0];
     dest[3] = argb8888[3];
}

int simg_get_bpp_by_type(unsigned int type) {
    if (type & 0x80) {
        type &= ~0x80;
    }
    switch (type) {
        case SIMG_TYPE_WB: return 1;
        case SIMG_TYPE_RGB332: return 8;
        case SIMG_TYPE_RGB565: case SIMG_TYPE_ARGB4444: return 16;
        case SIMG_TYPE_ARGB8888: return 32;
        default: return 0;
    }
}

uint8_t *simg_unpack_rle(const uint8_t *rle_bitmap, uint16_t width, uint16_t height, int bpp) {
    if (rle_bitmap) {
        const uint16_t permission = width * height;

        const size_t bytes_per_pixel = bpp / 8;
        const size_t bitmap_size = permission * bytes_per_pixel;
        uint8_t *bitmap = malloc(bitmap_size);
        if (bitmap) {
            size_t read_size = 0, copy_size = 0;
            while (copy_size < bitmap_size) {
                const uint8_t cmd = rle_bitmap[read_size];
                if ((cmd & 0x80) == 0) {
                    for(int i = 0; i < cmd; i++, copy_size += bytes_per_pixel) {
                        memcpy(bitmap + copy_size, rle_bitmap + read_size + 1, bytes_per_pixel);
                    }
                    read_size += bytes_per_pixel + 1;
                } else {
                    const size_t s = (0x100 - cmd) * bytes_per_pixel;
                    memcpy(bitmap + copy_size, rle_bitmap + read_size + 1, s);
                    copy_size += s;
                    read_size += s + 1;
                }
            }
            return bitmap;
        }
    }
    return NULL;
}

uint8_t *simg_unpack_1bit_raw(const uint8_t *data, uint16_t width, uint16_t height) {
    int bytes_per_row = (width + 7) / 8;
    uint8_t *rgba = malloc(width * height * 4);
    if (rgba) {
        const uint8_t bg[4] = {0xFF, 0xFF, 0xFF, 0xFF};
        const uint8_t fg[4] = {0x00, 0x00, 0x00, 0xFF};
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                const int byte_idx = y * bytes_per_row + (x / 8);
                const int bit = (data[byte_idx] >> (7 - (x % 8))) & 1; // MSB first

                const uint8_t *color = (bit == 0) ? bg : fg;
                const int idx = (y * width + x) * 4;
                memcpy(rgba + idx, color, 4);
            }
        }
    }
    return rgba;
}

int simg_write_png(const char *filename, const uint8_t *pixels, uint16_t width, uint16_t height, int type) {
    if (pixels) {
        uint8_t *rgba = NULL;
        type &= ~0x80;
        if (type != SIMG_TYPE_WB) {
            rgba = malloc(width * height * 4);
            if (rgba) {
                for (int i = 0; i < width * height; i++) {
                    if (type == SIMG_TYPE_ARGB8888) {
                        ARGB8888_to_RGBA8888(pixels + i * 4, rgba + i * 4);
                    } else if (type == SIMG_TYPE_RGB565) {
                        const uint16_t pixel = pixels[i * 2] | (pixels[i * 2 + 1] << 8);
                        RGB565_to_RGBA8888(pixel, rgba + i * 4);
                    } else if (type == SIMG_TYPE_ARGB4444) {
                        const uint16_t pixel = pixels[i * 2] | (pixels[i * 2 + 1] << 8);
                        ARGB444_to_RGBA8888(pixel, rgba + i * 4);
                    }
                    else if (type == SIMG_TYPE_RGB332) {
                        RGB332_to_RGBA8888(pixels[i], rgba + i * 4);
                    } else {
                        free(rgba);
                        rgba = NULL;
                        break;
                    }
                }
            }
        } else {
            rgba = simg_unpack_1bit_raw(pixels, width, height);
        }
        if (rgba) {
            stbi_write_png(filename, width, height, 4, rgba, width * 4);
            free(rgba);
            return 1;
        }
    }
    return 0;
}

uint32_t simg_addr_to_offset(const uint8_t *p) {
    const uint32_t addr = p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
    if (addr < 0xA0000000 || addr > 0xA8000000) {
        return 0;
    }
    return addr - 0xA0000000;
}

uint8_t *simg_find_pit(const uint8_t *buffer, size_t size, int platform) {
    const uint8_t sig[] = {0x52, 0x45, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t *found = memmem(buffer, size, sig, sizeof(sig));
    if (!found) return NULL;

    found += sizeof(sig);
    found = memmem(found, 0xFF, "RESOURCE", 8);
    if (!found) return NULL;

    if (found + 12 > buffer + size) return NULL;
    found += 12;
    if (platform == 0) { // SGOLD
        return found;
    }

    // NSG & ELKA
    if (found + 8 + 4 > buffer + size) return NULL;
    found += 8;
    uint32_t offset = simg_addr_to_offset(found);
    if (!offset || offset >= size) return NULL;

    offset += (platform == 1) ? 24 : 28; // NSG : ELKA
    if (offset >= size) return NULL;
    found = (uint8_t*)buffer + offset;
    if (found + 4 > buffer + size) return NULL;

    offset = simg_addr_to_offset(found);
    if (!offset || offset >= size) return NULL;
    found = (uint8_t*)buffer + offset;
    return found;
}