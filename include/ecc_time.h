#ifndef ECC_TIME_H
#define ECC_TIME_H

#include <stdint.h>

/**
 * @brief Enum for decoding error status.
 */
typedef enum {
    NO_ERROR,                   // No error detected
    SINGLE_BIT_ERROR_CORRECTED, // Single bit error detected and corrected
    DOUBLE_BIT_ERROR_DETECTED,  // Double bit error detected (uncorrectable)
    OVERALL_PARITY_ERROR        // Single error in the overall parity bit, data is correct
} EccTimeErrorStatus;

/**
 * @brief Struct to hold the decoded time data and its error status.
 */
typedef struct {
    uint32_t data;
    EccTimeErrorStatus status;
} DecodedTimeResult;

/**
 * @brief Encodes 24 data bits into a 32-bit integer containing a 30-bit SECDED codeword.
 *
 * The codeword structure is 1-indexed. Parity bits (P1, P2, P4, P8, P16) are at powers of 2.
 * Data bits fill the remaining positions up to 29.
 * The overall parity bit (P0) is placed at position 30.
 * The final 32-bit integer holds this 30-bit codeword in its lower bits.
 *
 * @param data The 24-bit unsigned integer data to encode.
 * @return The 32-bit integer containing the SECDED codeword.
 */
uint32_t encode_time_with_ecc(uint32_t data);

/**
 * @brief Decodes a 32-bit integer containing a 30-bit SECDED codeword.
 *
 * Detects and corrects single bit errors, detects double bit errors.
 *
 * @param received_codeword The 32-bit unsigned integer received codeword.
 * @return A DecodedTimeResult struct containing the extracted data and error status.
 */
DecodedTimeResult decode_time_with_ecc(uint32_t received_codeword);

#endif // ECC_TIME_H
