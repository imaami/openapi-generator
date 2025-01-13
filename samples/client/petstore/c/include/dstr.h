#ifndef OPENAPI_GENERATOR_C_LIBCURL_DSTR_H_
#define OPENAPI_GENERATOR_C_LIBCURL_DSTR_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

/**
 * @brief A dynamic string object
 */
struct dstr {
	union {
		char const *ref; //!< Pointer to immutable string
		char       *buf; //!< Pointer to allocated memory
	};
	size_t  cap; //!< Total size of the allocated memory
	size_t  len; //!< Length of the current string content
};

/**
 * @brief Initialize an empty dynamic string object RAII-style
 * @return The initialized empty object by value
 */
static inline struct dstr
dstr_init (void)
{
	return (struct dstr){
		.ref = "",
		.cap = 0U,
		.len = 0U
	};
}

/**
 * @brief Free the internal resources of a dynamic string object
 * @param dstr Pointer to the object to uninitialize
 * @note This is the complement method to dstr_init(), and as such does
 *       not free the object itself
 */
static inline void
dstr_fini (struct dstr *dstr)
{
	if (dstr) {
		if (dstr->cap)
			free(dstr->buf);
		*dstr = dstr_init();
	}
}

/**
 * @brief Initialize a dynamic string object RAII-style with preallocation
 * @param len Reserve initial space for at least this string length
 * @return The initialized object by value
 */
extern struct dstr
dstr_init_reserve (size_t len);

/**
 * @brief Set the content of a dynamic string object
 *
 * Equivalent to `dstr_write(dst, src, src_len, 0)`.
 *
 * @note `src` and `dst` are assumed to be valid pointers and are not
 *       null-checked. `src_len` is assumed to be the accurate string
 *       length of `src`, and `src` is not scanned for premature null
 *       bytes. The caller is responsible for valid input.
 *
 * @param dst Pointer to the destination object
 * @param src Pointer to the source string to set
 * @param src_len Length of the source string
 * @return true on success, false on failure
 */
extern bool
dstr_set (struct dstr *dst,
          char const  *src,
          size_t       src_len);

static inline char const *
dstr_get (struct dstr const *dstr)
{
	return dstr->cap ? dstr->buf : dstr->ref;
}

static inline size_t
dstr_len (struct dstr const *dstr)
{
	return dstr->len;
}

/**
 * @brief Append a string to a dynamic string object
 *
 * Equivalent to `dstr_write(dst, src, src_len, dst->len)`.
 *
 * @note `src` and `dst` are assumed to be valid pointers and are not
 *       null-checked. `src_len` is assumed to be the accurate string
 *       length of `src`, and `src` is not scanned for premature null
 *       bytes. The caller is responsible for valid input.
 *
 * @param dst Pointer to the destination object
 * @param src Pointer to the source string to append
 * @param src_len Length of the source string
 * @return true on success, false on failure
 */
extern bool
dstr_cat (struct dstr *dst,
          char const  *src,
          size_t       src_len);

/**
 * @brief Write a string to a specific offset in a dynamic string object
 *
 * This function writes a string to a specific offset in the destination
 * string. The greatest valid offset is `dst->len`, otherwise the result
 * would have a gap between the end of the current content and the start
 * of the new content.
 *
 * `dstr_write(dst, src, src_len, dst->len)` is exactly equivalent to
 * `dstr_cat(dst, src, src_len)`, and `dstr_write(dst, src, src_len, 0)`
 * is exactly equivalent to `dstr_set(dst, src, src_len)`.
 *
 * If `dst_off + src_len` is less than the value of `dst->len` before the
 * operation, the content string is null-terminated at `dst_off + src_len`
 * and that becomes the new length of the content string. In other words,
 * this is not a substring replacing function.
 *
 * @note `src` and `dst` are assumed to be valid pointers and are not
 *       null-checked. `src_len` is assumed to be the accurate string
 *       length of `src`, and `src` is not scanned for premature null
 *       bytes. The caller is responsible for valid input.
 *
 * @param dst Pointer to the destination object
 * @param src Pointer to the source string to write
 * @param src_len Length of the source string
 * @param dst_off Offset in the destination object to write to
 * @return true on success, false on failure
 */
extern bool
dstr_write (struct dstr *dst,
            char const  *src,
            size_t       src_len,
            size_t       dst_off);

#endif /* OPENAPI_GENERATOR_C_LIBCURL_DSTR_H_ */
