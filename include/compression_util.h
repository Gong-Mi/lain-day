#ifndef COMPRESSION_UTIL_H
#define COMPRESSION_UTIL_H

// Function to compress a string using zlib
// input: The string to compress
// compressed_data: A pointer to a buffer that will hold the compressed data.
//                  This buffer is allocated by the function; it must be freed by the caller.
// compressed_data_len: A pointer to store the length of the compressed data.
//
// Returns 0 on success, non-zero on error.
int compress_string(const char* input, unsigned char** compressed_data, unsigned long* compressed_data_len);

// Function to decompress a string using zlib
// compressed_data: The compressed data buffer.
// compressed_data_len: The length of the compressed data.
// decompressed_data: A pointer to a buffer that will hold the decompressed data.
//                    This buffer is allocated by the function; it must be freed by the caller.
// decompressed_data_len: A pointer to store the length of the decompressed data.
//
// Returns 0 on success, non-zero on error.
int decompress_string(const unsigned char* compressed_data, unsigned long compressed_data_len,
                      unsigned char** decompressed_data, unsigned long* decompressed_data_len);

#endif // COMPRESSION_UTIL_H
