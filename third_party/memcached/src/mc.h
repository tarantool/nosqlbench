#ifndef MC_H_INCLUDED
#define MC_H_INCLUDED

#if defined(__cplusplus) && !defined(__STDC_CONSTANT_MACROS)
#define __STDC_CONSTANT_MACROS 1 /* make С++ to be happy */
#endif
#if defined(__cplusplus) && !defined(__STDC_LIMIT_MACROS)
#define __STDC_LIMIT_MACROS 1    /* make С++ to be happy */
#endif
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#if defined(__cplusplus)
extern "C" {
#endif /* defined(__cplusplus) */

/*
 * {{{ Platform-specific definitions
 */

/** \cond 0 **/

#if defined(__CC_ARM)         /* set the alignment to 1 for armcc compiler */
#define MC_PACKED    __packed
#else
#define MC_PACKED
#endif

#if defined(__GNUC__) && !defined(__GNUC_STDC_INLINE__)
#if !defined(MC_SOURCE)
#define MC_PROTO extern inline
#define MC_IMPL extern inline
#else /* defined(MC_SOURCE) */
#define MC_PROTO
#define MC_IMPL
#endif
#define MC_ALWAYSINLINE
#else /* C99 inline */
#if !defined(MC_SOURCE)
#define MC_PROTO inline
#define MC_IMPL inline
#else /* defined(MC_SOURCE) */
#define MC_PROTO extern inline
#define MC_IMPL inline
#endif
#define MC_ALWAYSINLINE __attribute__((always_inline))
#endif /* GNU inline or C99 inline */

#if !defined __GNUC_MINOR__ || defined __INTEL_COMPILER || \
	defined __SUNPRO_C || defined __SUNPRO_CC
#define MC_GCC_VERSION(major, minor) 0
#else
#define MC_GCC_VERSION(major, minor) (__GNUC__ > (major) || \
	(__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
#endif

#if !defined(__has_builtin)
#define __has_builtin(x) 0 /* clang */
#endif

#if MC_GCC_VERSION(2, 9) || __has_builtin(__builtin_expect)
#define mc_likely(x) __builtin_expect((x), 1)
#define mc_unlikely(x) __builtin_expect((x), 0)
#else
#define mc_likely(x) (x)
#define mc_unlikely(x) (x)
#endif

#if MC_GCC_VERSION(4, 5) || __has_builtin(__builtin_unreachable)
#define mc_unreachable() (assert(0), __builtin_unreachable())
#else
MC_PROTO void
mc_unreachable(void) __attribute__((noreturn));
MC_PROTO void
mc_unreachable(void) { assert(0); abort(); }
#define mc_unreachable() (assert(0))
#endif

#define mc_bswap_u8(x) (x) /* just to simplify mc_load/mc_store macroses */

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

#if MC_GCC_VERSION(4, 8) || __has_builtin(__builtin_bswap16)
#define mc_bswap_u16(x) __builtin_bswap16(x)
#else /* !MC_GCC_VERSION(4, 8) */
#define mc_bswap_u16(x) ( \
	(((x) <<  8) & 0xff00) | \
	(((x) >>  8) & 0x00ff) )
#endif

#if MC_GCC_VERSION(4, 3) || __has_builtin(__builtin_bswap32)
#define mc_bswap_u32(x) __builtin_bswap32(x)
#else /* !MC_GCC_VERSION(4, 3) */
#define mc_bswap_u32(x) ( \
	(((x) << 24) & UINT32_C(0xff000000)) | \
	(((x) <<  8) & UINT32_C(0x00ff0000)) | \
	(((x) >>  8) & UINT32_C(0x0000ff00)) | \
	(((x) >> 24) & UINT32_C(0x000000ff)) )
#endif

#if MC_GCC_VERSION(4, 3) || __has_builtin(__builtin_bswap64)
#define mc_bswap_u64(x) __builtin_bswap64(x)
#else /* !MC_GCC_VERSION(4, 3) */
#define mc_bswap_u64(x) (\
	(((x) << 56) & UINT64_C(0xff00000000000000)) | \
	(((x) << 40) & UINT64_C(0x00ff000000000000)) | \
	(((x) << 24) & UINT64_C(0x0000ff0000000000)) | \
	(((x) <<  8) & UINT64_C(0x000000ff00000000)) | \
	(((x) >>  8) & UINT64_C(0x00000000ff000000)) | \
	(((x) >> 24) & UINT64_C(0x0000000000ff0000)) | \
	(((x) >> 40) & UINT64_C(0x000000000000ff00)) | \
	(((x) >> 56) & UINT64_C(0x00000000000000ff)) )
#endif

#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

#define mc_bswap_u16(x) (x)
#define mc_bswap_u32(x) (x)
#define mc_bswap_u64(x) (x)

#else
#error Unsupported __BYTE_ORDER__
#endif


/* {{{ API declaration */
struct mc;

typedef char *(*mc_reserve)(struct mc *p, size_t req, size_t *size);

/*
 * Main mc request object - points to a request buffer.
 *
 * All fields except mc->p should not be accessed directly.
 * Appropriate accessors should be used instead.
*/
struct mc {
	char *s, *p, *e;       /* start, pos, end */
	struct mc_hdr *hdr;    /* pointer to the header */
	mc_reserve reserve;    /* realloc function pointer */
	void *obj;             /* realloc function pointer */
};

enum mc_binary_type {
	MC_BIN_REQUEST  = 0x80,
	MC_BIN_RESPONSE = 0x81
};

enum mc_binary_response {
	MC_BIN_RES_OK              = 0x00,
	MC_BIN_RES_KEY_ENOENT      = 0x01,
	MC_BIN_RES_KEY_EEXISTS     = 0x02,
	MC_BIN_RES_E2BIG           = 0x03,
	MC_BIN_RES_EINVAL          = 0x04,
	MC_BIN_RES_NOT_STORED      = 0x05,
	MC_BIN_RES_DELTA_BADVAL    = 0x06,
	MC_BIN_RES_VBUCKET_BADVAL  = 0x07, // NO in memc
	MC_BIN_RES_AUTH_ERROR      = 0x20, // OR 0x08 in revmapped
	MC_BIN_RES_AUTH_CONTINUE   = 0x21, // OR 0x09 in revmapped
	MC_BIN_RES_UNKNOWN_COMMAND = 0x81,
	MC_BIN_RES_ENOMEM          = 0x82,
	MC_BIN_RES_NOT_SUPPORTED   = 0x83,
	MC_BIN_RES_SERVER_ERROR    = 0x84, // No in memc
	MC_BIN_RES_EBUSY           = 0x85, // No in memc
	MC_BIN_RES_EAGAIN          = 0x86  // No in memc
};

enum mc_binary_cmd {
	MC_BIN_CMD_GET      = 0x00,
	MC_BIN_CMD_SET      = 0x01,
	MC_BIN_CMD_ADD      = 0x02,
	MC_BIN_CMD_REPLACE  = 0x03,
	MC_BIN_CMD_DELETE   = 0x04,
	MC_BIN_CMD_INCR     = 0x05,
	MC_BIN_CMD_DECR     = 0x06,
	MC_BIN_CMD_QUIT     = 0x07,
	MC_BIN_CMD_FLUSH    = 0x08,
	MC_BIN_CMD_GETQ     = 0x09,
	MC_BIN_CMD_NOOP     = 0x0a,
	MC_BIN_CMD_VERSION  = 0x0b,
	MC_BIN_CMD_GETK     = 0x0c,
	MC_BIN_CMD_GETKQ    = 0x0d,
	MC_BIN_CMD_APPEND   = 0x0e,
	MC_BIN_CMD_PREPEND  = 0x0f,
	MC_BIN_CMD_STAT     = 0x10,
	MC_BIN_CMD_SETQ     = 0x11,
	MC_BIN_CMD_ADDQ     = 0x12,
	MC_BIN_CMD_REPLACEQ = 0x13,
	MC_BIN_CMD_DELETEQ  = 0x14,
	MC_BIN_CMD_INCRQ    = 0x15,
	MC_BIN_CMD_DECRQ    = 0x16,
	MC_BIN_CMD_QUITQ    = 0x17,
	MC_BIN_CMD_FLUSHQ   = 0x18,
	MC_BIN_CMD_APPENDQ  = 0x19,
	MC_BIN_CMD_PREPENDQ = 0x1a,
	MC_BIN_CMD_TOUCH    = 0x1c,
	MC_BIN_CMD_GAT      = 0x1d,
	MC_BIN_CMD_GATQ     = 0x1e,

	MC_BIN_CMD_SASL_LIST_MECHS = 0x20,
	MC_BIN_CMD_SASL_AUTH       = 0x21,
	MC_BIN_CMD_SASL_STEP       = 0x22,

	MC_BIN_CMD_GATK     = 0x23,
	MC_BIN_CMD_GATKQ    = 0x24,

	/* These commands are used for range operations and exist within
	 * this header for use in other projects.  Range operations are
	 * not expected to be implemented in the mc server itself.
	 */
	MC_BIN_CMD_RGET      = 0x30,
	MC_BIN_CMD_RSET      = 0x31,
	MC_BIN_CMD_RSETQ     = 0x32,
	MC_BIN_CMD_RAPPEND   = 0x33,
	MC_BIN_CMD_RAPPENDQ  = 0x34,
	MC_BIN_CMD_RPREPEND  = 0x35,
	MC_BIN_CMD_RPREPENDQ = 0x36,
	MC_BIN_CMD_RDELETE   = 0x37,
	MC_BIN_CMD_RDELETEQ  = 0x38,
	MC_BIN_CMD_RINCR     = 0x39,
	MC_BIN_CMD_RINCRQ    = 0x3a,
	MC_BIN_CMD_RDECR     = 0x3b,
	MC_BIN_CMD_RDECRQ    = 0x3c,
	/* End Range operations */
	MC_BIN_CMD_MAX
};

enum mc_binary_datatypes {
	MC_BIN_RAW_BYTES = 0x00
};

struct __attribute__((__packed__)) mc_hdr {
// struct mc_hdr {
	uint8_t  magic;
	uint8_t  cmd;
	uint16_t key_len;

	uint8_t  ext_len;
	uint8_t  data_type;
	union {
		uint16_t reserved;
		uint16_t status;
	};

	uint32_t tot_len;
	uint32_t opaque;
	uint64_t cas;
};

struct __attribute__((__packed__)) mc_get_ext {
	uint32_t flags;
};

struct __attribute__((__packed__)) mc_set_ext {
	uint32_t flags;
	uint32_t expire;
};

struct __attribute__((__packed__)) mc_delta_ext {
	uint64_t delta;
	uint64_t initial;
	uint32_t expire;
};

struct __attribute__((__packed__)) mc_flush_ext {
	uint32_t expire;
};

struct __attribute__((__packed__)) mc_verb_ext {
	uint32_t verbosity;
};

struct __attribute__((__packed__)) mc_touch_ext {
	uint32_t expire;
};

struct mc_response {
	struct mc_hdr  hdr;
	uint32_t       flags;
	uint16_t       key_len;
	const char    *key;
	uint32_t       val_len;
	const char    *val;
};

ssize_t
mc_ensure(struct mc *p, size_t size);

MC_PROTO void
mc_init(struct mc *p, char *buf, size_t size, mc_reserve reserve, void *obj);

MC_PROTO char*
mc_op_add(struct mc *p, const char *key, size_t key_len, const char *val,
	  size_t val_len, uint32_t flags, uint32_t expire);

MC_PROTO char*
mc_op_set(struct mc *p, const char *key, size_t key_len, const char *val,
	  size_t val_len, uint32_t flags, uint32_t expire);

MC_PROTO char*
mc_op_replace(struct mc *p, const char *key, size_t key_len, const char *val,
	      size_t val_len, uint32_t flags, uint32_t expire);

MC_PROTO char*
mc_op_delete(struct mc *p, const char *key, size_t key_len);

MC_PROTO char*
mc_op_get(struct mc *p, const char *key, size_t key_len);

MC_PROTO void
mc_set_cas(struct mc *p, uint64_t cas);

MC_PROTO void
mc_set_opaque(struct mc *p, uint32_t opaque);

MC_PROTO size_t
mc_unused(struct mc *p);

MC_PROTO size_t
mc_used(struct mc *p);

MC_PROTO ssize_t
mc_reply(struct mc_response *p, const char **buf, const char *end);
/* }}} */

MC_IMPL void
mc_init(struct mc *p, char *buf, size_t size, mc_reserve reserve, void *obj)
{
	p->s = buf;
	p->p = p->s;
	p->e = p->s + size;
	p->hdr = NULL;
	p->reserve = reserve;
	p->obj = obj;
}

MC_IMPL size_t
mc_unused(struct mc *p)
{
	return p->e - p->p;
}

ssize_t
mc_ensure(struct mc *p, size_t size)
{
	if (mc_likely(mc_unused(p) >= size))
		return 0;
	if (mc_unlikely(p->reserve == NULL))
		return -1;
	size_t sz;
	char *np = p->reserve(p, size, &sz);
	if (mc_unlikely(np == NULL))
		return -1;
	if (mc_likely(p->hdr != NULL))
		p->hdr = (struct mc_hdr *)(np + (((char *)p->hdr) - p->s));
	p->p = np + (p->p - p->s);
	p->s = np;
	p->e = np + sz;
	return sz;
}

MC_IMPL char*
mc_op_set(struct mc *p, const char *key, size_t key_len, const char *val,
	  size_t val_len, uint32_t flags, uint32_t expire) {
	if (mc_unlikely(mc_ensure(p, sizeof(struct mc_hdr) +
	    sizeof(struct mc_set_ext) + key_len + val_len) == -1))
		return NULL;
	memset(p->p, 0, sizeof(struct mc_hdr) + sizeof(struct mc_set_ext));
	p->hdr = (struct mc_hdr *)p->p;
	p->p += sizeof(struct mc_hdr);
	p->hdr->magic   = MC_BIN_REQUEST;
	p->hdr->cmd     = MC_BIN_CMD_SET;
	p->hdr->key_len = mc_bswap_u16(key_len);
	p->hdr->tot_len = mc_bswap_u32(val_len + key_len +
			sizeof(struct mc_set_ext));
	p->hdr->ext_len = sizeof(struct mc_set_ext);
	struct mc_set_ext *ext = (struct mc_set_ext *)(
			(char *)p->hdr + sizeof(struct mc_hdr));
	ext->flags  = mc_bswap_u32(flags);
	ext->expire = mc_bswap_u32(expire);
	p->p += sizeof(struct mc_set_ext);
	memcpy(p->p, key, key_len); p->p += key_len;
	memcpy(p->p, val, val_len); p->p += val_len;
	return p->p;
}

MC_IMPL char*
mc_op_add(struct mc *p, const char *key, size_t key_len, const char *val,
	  size_t val_len, uint32_t flags, uint32_t expire) {
	if (mc_unlikely(mc_ensure(p, sizeof(struct mc_hdr) +
	    sizeof(struct mc_set_ext) + key_len + val_len) == -1))
		return NULL;
	memset(p->p, 0, sizeof(struct mc_hdr) + sizeof(struct mc_set_ext));
	p->hdr = (struct mc_hdr *)p->p;
	p->p += sizeof(struct mc_hdr);
	p->hdr->magic   = MC_BIN_REQUEST;
	p->hdr->cmd     = MC_BIN_CMD_ADD;
	p->hdr->key_len = mc_bswap_u16(key_len);
	p->hdr->tot_len = mc_bswap_u32(val_len + key_len +
			sizeof(struct mc_set_ext));
	p->hdr->ext_len = sizeof(struct mc_set_ext);
	struct mc_set_ext *ext = (struct mc_set_ext *)(
			(char *)p->hdr + sizeof(struct mc_hdr));
	ext->flags  = mc_bswap_u32(flags);
	ext->expire = mc_bswap_u32(expire);
	p->p += sizeof(struct mc_set_ext);
	memcpy(p->p, key, key_len); p->p += key_len;
	memcpy(p->p, val, val_len); p->p += val_len;
	return p->p;
}

MC_IMPL char*
mc_op_replace(struct mc *p, const char *key, size_t key_len, const char *val,
	  size_t val_len, uint32_t flags, uint32_t expire) {
	if (mc_unlikely(mc_ensure(p, sizeof(struct mc_hdr) +
	    sizeof(struct mc_set_ext) + key_len + val_len) == -1))
		return NULL;
	memset(p->p, 0, sizeof(struct mc_hdr) + sizeof(struct mc_set_ext));
	p->hdr = (struct mc_hdr *)p->p;
	p->p += sizeof(struct mc_hdr);
	p->hdr->magic   = MC_BIN_REQUEST;
	p->hdr->cmd     = MC_BIN_CMD_REPLACE;
	p->hdr->key_len = mc_bswap_u16(key_len);
	p->hdr->tot_len = mc_bswap_u32(val_len + key_len +
			sizeof(struct mc_set_ext));
	p->hdr->ext_len = sizeof(struct mc_set_ext);
	struct mc_set_ext *ext = (struct mc_set_ext *)(
			(char *)p->hdr + sizeof(struct mc_hdr));
	ext->flags  = mc_bswap_u32(flags);
	ext->expire = mc_bswap_u32(expire);
	p->p += sizeof(struct mc_set_ext);
	memcpy(p->p, key, key_len); p->p += key_len;
	memcpy(p->p, val, val_len); p->p += val_len;
	return p->p;
}

MC_IMPL char*
mc_op_delete(struct mc *p, const char *key, size_t key_len) {
	if (mc_unlikely(mc_ensure(p, sizeof(struct mc_hdr) + key_len) == -1))
		return NULL;
	memset(p->p, 0, sizeof(struct mc_hdr));
	p->hdr = (struct mc_hdr *)p->p;
	p->p += sizeof(struct mc_hdr);
	p->hdr->magic   = MC_BIN_REQUEST;
	p->hdr->cmd     = MC_BIN_CMD_DELETE;
	p->hdr->key_len = mc_bswap_u16(key_len);
	p->hdr->tot_len = mc_bswap_u32(key_len);
	memcpy(p->p, key, key_len); p->p += key_len;
	return p->p;
}

MC_IMPL char*
mc_op_get(struct mc *p, const char *key, size_t key_len) {
	if (mc_unlikely(mc_ensure(p, sizeof(struct mc_hdr) + key_len) == -1))
		return NULL;
	memset(p->p, 0, sizeof(struct mc_hdr));
	p->hdr = (struct mc_hdr *)p->p;
	p->p += sizeof(struct mc_hdr);
	p->hdr->magic   = MC_BIN_REQUEST;
	p->hdr->cmd     = MC_BIN_CMD_GET;
	p->hdr->key_len = mc_bswap_u16(key_len);
	p->hdr->tot_len = mc_bswap_u32(key_len);
	memcpy(p->p, key, key_len); p->p += key_len;
	return p->p;
}

MC_IMPL void
mc_set_cas(struct mc *p, uint64_t cas) {
	p->hdr->cas = mc_bswap_u64(cas);
}

MC_IMPL void
mc_set_opaque(struct mc *p, uint32_t opaque) {
	p->hdr->opaque = mc_bswap_u32(opaque);
}

MC_IMPL size_t
mc_used(struct mc *p)
{
	return p->p - p->s;
}

/*
 *
 *
 * Returns  -1 on error
 * Returns   0 on successful parse (advances `buf`)
 * Returns > 0 if buffer size is less then needed (bytes to read)
 */
MC_IMPL ssize_t
mc_reply (struct mc_response *r, const char **buf, const char *end) {
	struct mc_hdr *hdr = (struct mc_hdr *)*buf;
	// Wrong magic
	if (hdr->magic != MC_BIN_RESPONSE)
		return -1;
	// We need more bytes
	ssize_t tot_len = mc_bswap_u32(hdr->tot_len) + sizeof(struct mc_hdr);
	if (tot_len > (end - *buf))
		return tot_len - (end - *buf);
	memcpy(&(r->hdr), hdr, sizeof(struct mc_response));
	r->hdr.key_len = r->key_len = mc_bswap_u16(r->hdr.key_len);
	r->hdr.status  = mc_bswap_u16(r->hdr.status);
	r->hdr.tot_len = mc_bswap_u32(r->hdr.tot_len);
	r->hdr.opaque  = mc_bswap_u32(r->hdr.opaque);
	r->hdr.cas     = mc_bswap_u64(r->hdr.cas);
	if (r->hdr.ext_len) {
		r->flags = *(uint32_t *)(*buf + sizeof(struct mc_hdr));
		r->flags = mc_bswap_u32(r->flags);
	}
	r->key         = *buf + sizeof(struct mc_hdr) + r->hdr.ext_len;
	r->val_len     = r->hdr.tot_len - (r->hdr.ext_len + r->hdr.key_len);
	r->val         = r->key + r->key_len;
	*buf = *buf + tot_len;
	return 0;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#undef MC_SOURCE
#undef MC_PROTO
#undef MC_IMCL
#undef MC_ALWAYSINLINE
#undef MC_GCC_VERSION

#endif
