#include "ecc_time.h"
#include <stdint.h>

// Constants for the SECDED algorithm
#define DATA_BITS 24
#define PARITY_BITS 5 // For 24 data bits, 2^5 = 32 >= 24 + 5 + 1 = 30
#define HAMMING_CODEWORD_BITS (DATA_BITS + PARITY_BITS) // 29 bits
#define SECDED_CODEWORD_BITS (HAMMING_CODEWORD_BITS + 1) // 30 bits (including overall parity)

/**
 * @brief Helper function to get a bit at a specific position (1-indexed).
 */
static inline int get_bit(uint32_t value, int pos) {
    if (pos < 1 || pos > 32) return 0;
    return (value >> (pos - 1)) & 1;
}

/**
 * @brief Helper function to set a bit at a specific position (1-indexed).
 */
static inline void set_bit(uint32_t *value, int pos, int bit) {
    if (pos < 1 || pos > 32) return;
    if (bit) {
        *value |= (1U << (pos - 1));
    } else {
        *value &= ~(1U << (pos - 1));
    }
}

/**
 * @brief Helper function to flip a bit at a specific position (1-indexed).
 */
static inline void flip_bit(uint32_t *value, int pos) {
    if (pos < 1 || pos > 32) return;
    *value ^= (1U << (pos - 1));
}

uint32_t encode_time_with_ecc(uint32_t data) {
    uint32_t codeword = 0;
    int data_idx = 0;

    // 1. Place data bits into the codeword
    for (int i = 1; i <= HAMMING_CODEWORD_BITS; ++i) {
        if (!((i & (i - 1)) == 0 && i != 0)) { // If 'i' is NOT a power of 2
            set_bit(&codeword, i, get_bit(data, data_idx + 1));
            data_idx++;
        }
    }

    // 2. Calculate Hamming parity bits
    for (int p_pos = 1; p_pos <= HAMMING_CODEWORD_BITS; p_pos <<= 1) {
        int parity = 0;
        for (int i = 1; i <= HAMMING_CODEWORD_BITS; ++i) {
            if (i & p_pos) {
                parity ^= get_bit(codeword, i);
            }
        }
        set_bit(&codeword, p_pos, parity);
    }

    // 3. Calculate overall parity for SECDED
    int overall_parity = 0;
    for (int i = 1; i <= HAMMING_CODEWORD_BITS; ++i) {
        overall_parity ^= get_bit(codeword, i);
    }
    set_bit(&codeword, SECDED_CODEWORD_BITS, overall_parity);

    return codeword;
}

DecodedTimeResult decode_time_with_ecc(uint32_t received_codeword) {
    DecodedTimeResult result;
    uint32_t corrected_codeword = received_codeword;

    // 1. Calculate Hamming syndrome
    int syndrome = 0;
    for (int p_pos = 1; p_pos <= HAMMING_CODEWORD_BITS; p_pos <<= 1) {
        int parity = 0;
        for (int i = 1; i <= HAMMING_CODEWORD_BITS; ++i) {
            if (i & p_pos) {
                parity ^= get_bit(received_codeword, i);
            }
        }
        if (parity != 0) {
            syndrome |= p_pos;
        }
    }

    // 2. Calculate overall parity check
    int overall_parity_check = 0;
    for (int i = 1; i <= SECDED_CODEWORD_BITS; ++i) {
        overall_parity_check ^= get_bit(received_codeword, i);
    }

    // 3. Interpret syndrome and overall parity check
    if (syndrome == 0) {
        if (overall_parity_check == 0) {
            result.status = NO_ERROR;
        } else {
            result.status = OVERALL_PARITY_ERROR;
        }
    } else {
        if (overall_parity_check == 1) {
            if (syndrome < SECDED_CODEWORD_BITS) {
                flip_bit(&corrected_codeword, syndrome);
            }
            result.status = SINGLE_BIT_ERROR_CORRECTED;
        } else {
            result.status = DOUBLE_BIT_ERROR_DETECTED;
        }
    }

    // 4. Extract data bits from the (potentially corrected) codeword
    uint32_t extracted_data = 0;
    int data_idx = 0;
    for (int i = 1; i <= HAMMING_CODEWORD_BITS; ++i) {
        if (!((i & (i - 1)) == 0 && i != 0)) { // If 'i' is NOT a power of 2
            set_bit(&extracted_data, data_idx + 1, get_bit(corrected_codeword, i));
            data_idx++;
        }
    }
    result.data = extracted_data;

    return result;
}
