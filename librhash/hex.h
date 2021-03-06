/* hex.h - conversion for hexadecimal and base32 strings. */
#ifndef HEX_H
#define HEX_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void rhash_byte_to_hex(char *dest, const unsigned char *src, unsigned len, int upper_case);
void rhash_byte_to_base32(char* dest, const unsigned char* src, unsigned len, int upper_case);
void rhash_byte_to_base64(char* dest, const unsigned char* src, unsigned len);
char* rhash_print_hex_byte(char *dest, const unsigned char byte, int upper_case);
int  rhash_urlencode(char *dst, const char *name);
int  rhash_sprintI64(char *dst, uint64_t number);

#define BASE32_LENGTH(bytes) (((bytes) * 8 + 4) / 5)
#define BASE64_LENGTH(bytes) ((((bytes) + 2) / 3) * 4)

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* HEX_H */
