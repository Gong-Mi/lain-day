#include "compression_util.h"
#include <zlib.h> // zlib header
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHUNK 16384 // Define a chunk size for zlib operations

int compress_string(const char* input, unsigned char** compressed_data, unsigned long* compressed_data_len) {
    int ret;
    z_stream strm;
    int flush;
    unsigned char out[CHUNK];

    // Allocate deflate state
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK)
        return ret;

    // Convert input string to byte array
    strm.avail_in = strlen(input);
    strm.next_in = (unsigned char*)input;

    // Dynamically grow the output buffer
    unsigned long current_buffer_size = CHUNK;
    *compressed_data = (unsigned char*)malloc(current_buffer_size);
    if (*compressed_data == NULL) {
        deflateEnd(&strm);
        return Z_MEM_ERROR;
    }
    *compressed_data_len = 0;

    // Compress until end of stream
    do {
        strm.avail_out = CHUNK;
        strm.next_out = out;
        flush = (strm.avail_in == 0) ? Z_FINISH : Z_NO_FLUSH;
        ret = deflate(&strm, flush);    // no bad return value
        if (ret == Z_STREAM_ERROR) {
            free(*compressed_data);
            *compressed_data = NULL;
            deflateEnd(&strm);
            return ret;
        }

        // Resize output buffer if needed
        unsigned long bytes_produced = CHUNK - strm.avail_out;
        if ((*compressed_data_len + bytes_produced) > current_buffer_size) {
            current_buffer_size *= 2;
            unsigned char* temp = (unsigned char*)realloc(*compressed_data, current_buffer_size);
            if (temp == NULL) {
                free(*compressed_data);
                *compressed_data = NULL;
                deflateEnd(&strm);
                return Z_MEM_ERROR;
            }
            *compressed_data = temp;
        }

        memcpy(*compressed_data + *compressed_data_len, out, bytes_produced);
        *compressed_data_len += bytes_produced;

    } while (flush != Z_FINISH);

    // Clean up and return
    deflateEnd(&strm);
    return Z_OK;
}

int decompress_string(const unsigned char* compressed_data, unsigned long compressed_data_len,
                      unsigned char** decompressed_data, unsigned long* decompressed_data_len) {
    int ret;
    z_stream strm;
    unsigned char out[CHUNK];

    // Allocate inflate state
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    strm.avail_in = compressed_data_len;
    strm.next_in = (unsigned char*)compressed_data;

    // Dynamically grow the output buffer
    unsigned long current_buffer_size = CHUNK;
    *decompressed_data = (unsigned char*)malloc(current_buffer_size);
    if (*decompressed_data == NULL) {
        inflateEnd(&strm);
        return Z_MEM_ERROR;
    }
    *decompressed_data_len = 0;

    // Decompress until deflate stream ends or error
    do {
        strm.avail_out = CHUNK;
        strm.next_out = out;
        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
            free(*decompressed_data);
            *decompressed_data = NULL;
            inflateEnd(&strm);
            return ret;
        }

        unsigned long bytes_produced = CHUNK - strm.avail_out;
        if ((*decompressed_data_len + bytes_produced) > current_buffer_size) {
            current_buffer_size *= 2;
            unsigned char* temp = (unsigned char*)realloc(*decompressed_data, current_buffer_size);
            if (temp == NULL) {
                free(*decompressed_data);
                *decompressed_data = NULL;
                inflateEnd(&strm);
                return Z_MEM_ERROR;
            }
            *decompressed_data = temp;
        }
        memcpy(*decompressed_data + *decompressed_data_len, out, bytes_produced);
        *decompressed_data_len += bytes_produced;

    } while (ret != Z_STREAM_END);

    // Clean up and return
    inflateEnd(&strm);
    return Z_OK;
}