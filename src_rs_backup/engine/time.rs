//! ECC 时间编码的 Rust 实现
//! 与 C 版本逻辑一致：24-bit data + 5-bit Hamming parity + 1-bit overall parity + 2-bit noise

use thiserror::Error;

const DATA_BITS: usize = 24;
const PARITY_BITS: usize = 5;
const HAMMING_CODEWORD_BITS: usize = DATA_BITS + PARITY_BITS; // 29
const SECDED_CODEWORD_BITS: usize = HAMMING_CODEWORD_BITS + 1; // 30

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TimeDecodeStatus {
    NoError,
    SingleBitErrorCorrected,
    DoubleBitErrorDetected,
    OverallParityError,
}

#[derive(Debug, Clone, Copy)]
pub struct DecodedTime {
    pub data: u32,
    pub status: TimeDecodeStatus,
}

#[derive(Debug, Error)]
pub enum TimeError {
    #[error("double bit error detected in time codeword")]
    DoubleBitError,
}

/// 判断一个位置是否是 2 的幂（即校验位位置）
fn is_power_of_two(n: usize) -> bool {
    n != 0 && (n & (n - 1)) == 0
}

fn get_bit(value: u32, pos: usize) -> u32 {
    (value >> (pos - 1)) & 1
}

fn set_bit(value: &mut u32, pos: usize, bit: u32) {
    if bit != 0 {
        *value |= 1 << (pos - 1);
    } else {
        *value &= !(1 << (pos - 1));
    }
}

fn flip_bit(value: &mut u32, pos: usize) {
    *value ^= 1 << (pos - 1);
}

/// 编码：原始 24-bit 时间 → 30-bit ECC codeword
pub fn encode(data: u32) -> u32 {
    let mut codeword: u32 = 0;
    let mut data_idx = 0;

    // 1. 放置数据位到非校验位位置
    for i in 1..=HAMMING_CODEWORD_BITS {
        if !is_power_of_two(i) {
            set_bit(&mut codeword, i, get_bit(data, data_idx + 1));
            data_idx += 1;
        }
    }

    // 2. 计算 Hamming 校验位
    for p_pos in [1, 2, 4, 8, 16] {
        let mut parity = 0;
        for i in 1..=HAMMING_CODEWORD_BITS {
            if i & p_pos != 0 {
                parity ^= get_bit(codeword, i);
            }
        }
        set_bit(&mut codeword, p_pos, parity);
    }

    // 3. 计算整体奇偶校验位（SECDED）
    let mut overall_parity = 0;
    for i in 1..=HAMMING_CODEWORD_BITS {
        overall_parity ^= get_bit(codeword, i);
    }
    set_bit(&mut codeword, SECDED_CODEWORD_BITS, overall_parity);

    codeword
}

/// 解码：30-bit ECC codeword → 原始时间 + 状态
pub fn decode(received: u32) -> DecodedTime {
    let mut corrected = received;

    // 1. 计算 syndrome
    let mut syndrome = 0usize;
    for p_pos in [1, 2, 4, 8, 16] {
        let mut parity = 0;
        for i in 1..=HAMMING_CODEWORD_BITS {
            if i & p_pos != 0 {
                parity ^= get_bit(received, i);
            }
        }
        if parity != 0 {
            syndrome |= p_pos;
        }
    }

    // 2. 计算整体奇偶校验
    let mut overall_parity_check = 0;
    for i in 1..=SECDED_CODEWORD_BITS {
        overall_parity_check ^= get_bit(received, i);
    }

    // 3. 判断错误类型
    let status = if syndrome == 0 {
        if overall_parity_check == 0 {
            TimeDecodeStatus::NoError
        } else {
            TimeDecodeStatus::OverallParityError
        }
    } else if overall_parity_check == 1 {
        if syndrome < SECDED_CODEWORD_BITS {
            flip_bit(&mut corrected, syndrome);
        }
        TimeDecodeStatus::SingleBitErrorCorrected
    } else {
        TimeDecodeStatus::DoubleBitErrorDetected
    };

    // 4. 提取数据位
    let mut extracted = 0u32;
    let mut data_idx = 0;
    for i in 1..=HAMMING_CODEWORD_BITS {
        if !is_power_of_two(i) {
            set_bit(&mut extracted, data_idx + 1, get_bit(corrected, i));
            data_idx += 1;
        }
    }

    DecodedTime {
        data: extracted,
        status,
    }
}

/// 将时间单位转换为游戏内的小时和分钟
/// 16 units = 1 second, 960 units = 1 minute, 57600 units = 1 hour
pub fn to_hm(raw_units: u32) -> (u32, u32) {
    let total_seconds = raw_units / 16;
    let total_minutes = total_seconds / 60;
    let hours = (total_minutes / 60) % 24;
    let minutes = total_minutes % 60;
    (hours, minutes)
}

// =============================================================================
// 测试
// =============================================================================
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_roundtrip() {
        let original: u32 = 12345;
        let codeword = encode(original);
        let decoded = decode(codeword);
        assert_eq!(decoded.data, original);
        assert_eq!(decoded.status, TimeDecodeStatus::NoError);
    }

    #[test]
    fn test_single_bit_error_corrected() {
        let original: u32 = 99999;
        let mut codeword = encode(original);
        // 翻转第 3 位（单比特错误）
        flip_bit(&mut codeword, 3);
        let decoded = decode(codeword);
        assert_eq!(decoded.data, original);
        assert_eq!(decoded.status, TimeDecodeStatus::SingleBitErrorCorrected);
    }

    #[test]
    fn test_double_bit_error_detected() {
        let original: u32 = 55555;
        let mut codeword = encode(original);
        // 翻转两个位
        flip_bit(&mut codeword, 3);
        flip_bit(&mut codeword, 5);
        let decoded = decode(codeword);
        assert_eq!(decoded.status, TimeDecodeStatus::DoubleBitErrorDetected);
    }

    #[test]
    fn test_time_conversion() {
        // 8 hours = 8 * 3600 * 16 = 460800 units
        let units = 8 * 3600 * 16;
        let (h, m) = to_hm(units);
        assert_eq!(h, 8);
        assert_eq!(m, 0);
    }
}
