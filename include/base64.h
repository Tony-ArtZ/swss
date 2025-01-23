#ifndef BASE64_H
#define BASE64_H
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

char *base64_encode(const unsigned char *data, size_t input_length, size_t *output_length);
int b64_encode();

#endif /* BASE64_H */