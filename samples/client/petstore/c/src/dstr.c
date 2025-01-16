#include <stdint.h>
#include <string.h>

#include "../include/dstr.h"

/**
 * @brief Calculate a rounded-up buffer size for a string `len` bytes long
 *
 * Because `len` is a string length and does not count the null terminator,
 * but the return value does, the return value is never 0. Additionally, as
 * the return value is a multiple of 64, the smallest possible return value
 * is 64 (even when `len` is 0).
 *
 * If `len` is greater than `SIZE_MAX`-64, `SIZE_MAX` is returned.
 * A `SIZE_MAX` return always indicates an error.
 *
 * @param len String length in bytes without the null terminator
 * @return Rounded up capacity in bytes including the null terminator
 * @retval At least 64 up to `SIZE_MAX`-64 in steps of 64 for valid input
 * @retval `SIZE_MAX` if the input length is greater than `SIZE_MAX`-64
 */
static inline size_t
dstr_need (size_t len)
{
	return len > SIZE_MAX - 64U ? SIZE_MAX : (len + 64U) & ~(size_t)63U;
}

/**
 * @brief Add two sizes and saturate on overflow
 *
 * This function is used to prevent overflows when adding two sizes. If the
 * result would be greater than `SIZE_MAX`, `SIZE_MAX` is returned.
 *
 * @note There's no way to tell apart an overflow and a sum that is exactly
 *       `SIZE_MAX`, but that's not a problem when adding string lengths. If
 *       the resulting string length would be `SIZE_MAX`, there would be no
 *       space for the null terminator anyway.
 *
 * @param a The first size
 * @param b The second size
 * @return The sum, or `SIZE_MAX` on overflow
 * @retval 0...`SIZE_MAX`-1 when the sum is a valid string length
 * @retval `SIZE_MAX` on overflow, or when the result would be too long for
 *         a string with a null terminator
 */
static inline size_t
dstr_sadd (size_t a,
           size_t b)
{
	a += b;
	return a >= b ? a : SIZE_MAX;
}

struct dstr
dstr_init_reserve (size_t len)
{
	size_t cap = dstr_need(len);
	if (len < cap) {
		char *buf = calloc(1U, cap);
		if (buf) {
			return (struct dstr){
				.buf = buf,
				.cap = cap,
				.len = 0U
			};
		}
	}
	return dstr_init();
}

static bool
dstr_write_ (struct dstr *dst,
             char const  *src,
             size_t       src_len,
             size_t       dst_off)
{
	size_t new_len = dstr_sadd(dst_off, src_len);
	size_t new_cap = dstr_need(new_len);
	if (new_len >= new_cap) {
		// Length would not leave space for null terminator
		return false;
	}

	if (dst->cap < new_cap) {
		char *new_buf = dst->cap ? realloc(dst->buf, new_cap)
		                         : calloc(1U, new_cap);
		if (!new_buf) {
			// Allocation failed
			return false;
		}

		dst->buf = new_buf;
		dst->cap = new_cap;
	}

	if (src_len > 0U) {
		memcpy(&dst->buf[dst_off], src, src_len);
	}
	dst->buf[new_len] = '\0';
	dst->len = new_len;

	return true;
}

bool
dstr_set (struct dstr *dst,
          char const  *src,
          size_t       src_len)
{
	return dstr_write_(dst, src, src_len, 0U);
}

bool
dstr_cat (struct dstr *dst,
          char const  *src,
          size_t       src_len)
{
	return dstr_write_(dst, src, src_len, dst->len);
}

bool
dstr_write (struct dstr *dst,
            char const  *src,
            size_t       src_len,
            size_t       dst_off)
{
	if (dst_off > dst->len) {
		// Would leave gap between end of string and offset
		return false;
	}
	return dstr_write_(dst, src, src_len, dst_off);
}
