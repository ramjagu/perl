/*    hv.h
 *
 *    Copyright (C) 1991, 1992, 1993, 1996, 1997, 1998, 1999,
 *    2000, 2001, 2002, 2003, 2005, 2006, 2007, 2008, by Larry Wall and others
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

/* entry in hash value chain */
struct he {
    /* Keep hent_next first in this structure, because sv_free_arenas take
       advantage of this to share code between the he arenas and the SV
       body arenas  */
    HE		*hent_next;	/* next entry in chain */
    HEK		*hent_hek;	/* hash key */
    union {
	SV	*hent_val;	/* scalar value that was hashed */
	Size_t	hent_refcount;	/* references for this shared hash key */
    } he_valu;
};

/* hash key -- defined separately for use as shared pointer */
struct hek {
    U32		hek_hash;	/* hash of key */
    I32		hek_len;	/* length of hash key */
    char	hek_key[1];	/* variable-length hash key */
    /* the hash-key is \0-terminated */
    /* after the \0 there is a byte for flags, such as whether the key
       is UTF-8 */
};

struct shared_he {
    struct he shared_he_he;
    struct hek shared_he_hek;
};

/* Subject to change.
   Don't access this directly.
   Use the funcs in mro.c
*/

struct mro_alg {
    AV *(*resolve)(pTHX_ HV* stash, U32 level);
    const char *name;
    U16 length;
    U16	kflags;	/* For the hash API - set HVhek_UTF8 if name is UTF-8 */
    U32 hash; /* or 0 */
};

struct mro_meta {
    /* a hash holding the different MROs private data.  */
    HV      *mro_linear_all;
    /* a pointer directly to the current MROs private data.  If mro_linear_all
       is NULL, this owns the SV reference, else it is just a pointer to a
       value stored in and owned by mro_linear_all.  */
    SV      *mro_linear_current;
    HV      *mro_nextmethod; /* next::method caching */
    U32     cache_gen;       /* Bumping this invalidates our method cache */
    U32     pkg_gen;         /* Bumps when local methods/@ISA change */
    const struct mro_alg *mro_which; /* which mro alg is in use? */
    HV      *isa;            /* Everything this class @ISA */
};

#define MRO_GET_PRIVATE_DATA(smeta, which)		   \
    (((smeta)->mro_which && (which) == (smeta)->mro_which) \
     ? (smeta)->mro_linear_current			   \
     : Perl_mro_get_private_data(aTHX_ (smeta), (which)))

/* Subject to change.
   Don't access this directly.
*/

union _xhvnameu {
    HEK *xhvnameu_name;		/* When xhv_name_count is 0 */
    HEK **xhvnameu_names;	/* When xhv_name_count is non-0 */
};

struct xpvhv_aux {
    union _xhvnameu xhv_name_u;	/* name, if a symbol table */
    AV		*xhv_backreferences; /* back references for weak references */
    HE		*xhv_eiter;	/* current entry of iterator */
    I32		xhv_riter;	/* current root of iterator */

/* Concerning xhv_name_count: When non-zero, xhv_name_u contains a pointer 
 * to an array of HEK pointers, this being the length. The first element is
 * the name of the stash, which may be NULL. If xhv_name_count is positive,
 * then *xhv_name is one of the effective names. If xhv_name_count is nega-
 * tive, then xhv_name_u.xhvnameu_names[1] is the first effective name.
 */
    I32		xhv_name_count;
    struct mro_meta *xhv_mro_meta;
    HV *	xhv_super;	/* SUPER method cache */
};

/* hash structure: */
/* This structure must match the beginning of struct xpvmg in sv.h. */
struct xpvhv {
    HV*		xmg_stash;	/* class package */
    union _xmgu	xmg_u;
    STRLEN      xhv_keys;       /* total keys, including placeholders */
    STRLEN      xhv_max;        /* subscript of last element of xhv_array */
};

/* hash a key */
/* The use of a temporary pointer and the casting games
 * is needed to serve the dual purposes of
 * (a) the hashed data being interpreted as "unsigned char" (new since 5.8,
 *     a "char" can be either signed or unsigned, depending on the compiler)
 * (b) catering for old code that uses a "char"
 *
 * The "hash seed" feature was added in Perl 5.8.1 to perturb the results
 * to avoid "algorithmic complexity attacks".
 *
 * If USE_HASH_SEED is defined, hash randomisation is done by default
 * If USE_HASH_SEED_EXPLICIT is defined, hash randomisation is done
 * only if the environment variable PERL_HASH_SEED is set.
 * (see also perl.c:perl_parse() and S_init_tls_and_interp() and util.c:get_hash_seed())
 */
#ifndef PERL_HASH_SEED
#   if defined(USE_HASH_SEED) || defined(USE_HASH_SEED_EXPLICIT)
#       define PERL_HASH_SEED PL_hash_seed
#   else
#       define PERL_HASH_SEED "PeRlHaShhAcKpErl"
#   endif
#endif

#define PERL_HASH_SEED_U32   *((U32*)PERL_HASH_SEED)
#define PERL_HASH_SEED_U64_1 (((U64*)PERL_HASH_SEED)[0])
#define PERL_HASH_SEED_U64_2 (((U64*)PERL_HASH_SEED)[1])
#define PERL_HASH_SEED_U16_x(idx) (((U16*)PERL_HASH_SEED)[idx])

/* legacy - only mod_perl should be doing this.  */
#ifdef PERL_HASH_INTERNAL_ACCESS
#define PERL_HASH_INTERNAL(hash,str,len) PERL_HASH(hash,str,len)
#endif

/* Uncomment one of the following lines to use an alternative hash algorithm.
#define PERL_HASH_FUNC_SDBM
#define PERL_HASH_FUNC_DJB2
#define PERL_HASH_FUNC_SUPERFAST
#define PERL_HASH_FUNC_MURMUR3
#define PERL_HASH_FUNC_SIPHASH
#define PERL_HASH_FUNC_ONE_AT_A_TIME
#define PERL_HASH_FUNC_BUZZHASH16
*/

#if !(defined(PERL_HASH_FUNC_SDBM) || defined(PERL_HASH_FUNC_DJB2) || defined(PERL_HASH_FUNC_SUPERFAST) \
        || defined(PERL_HASH_FUNC_MURMUR3) || defined(PERL_HASH_FUNC_ONE_AT_A_TIME) || defined(PERL_HASH_FUNC_BUZZHASH16))
#define PERL_HASH_FUNC_MURMUR3
#endif

#if defined(PERL_HASH_FUNC_BUZZHASH16)
/* "BUZZHASH16"
 *
 * I whacked this together while just playing around.
 *
 * The idea is that instead of hashing the actual string input we use the
 * bytes of the string as an index into a table of randomly generated
 * 16 bit values.
 *
 * A left rotate is used to "mix" in previous bits as we go, and I borrowed
 * the avalanche function from one-at-a-time for the final step. A lookup
 * into the table based on the lower 8 bits of the length combined with
 * the length itself is used as an itializer.
 *
 * The resulting hash value has no actual bits fed in from the string so
 * I would guess it is pretty secure, although I am not a cryptographer
 * and have no idea for sure. Nor has it been rigorously tested. On the
 * other hand it is reasonably fast, and seems to produce reasonable
 * distributions.
 *
 * Yves Orton
 */


#define PERL_HASH_FUNC "BUZZHASH16"
#define PERL_HASH_SEED_BYTES 512 /* 2 bytes per octet value, 2 * 256 */
/* Find best way to ROTL32 */
#if defined(_MSC_VER)
  #include <stdlib.h>  /* Microsoft put _rotl declaration in here */
  #define BUZZHASH_ROTL32(x,r)  _rotl(x,r)
#else
  /* gcc recognises this code and generates a rotate instruction for CPUs with one */
  #define BUZZHASH_ROTL32(x,r)  (((U32)x << r) | ((U32)x >> (32 - r)))
#endif

#define PERL_HASH(hash,str,len) \
     STMT_START        { \
        const char * const s_PeRlHaSh_tmp = (str); \
        const unsigned char *s_PeRlHaSh = (const unsigned char *)s_PeRlHaSh_tmp; \
        const unsigned char *end_PeRlHaSh = (const unsigned char *)s_PeRlHaSh + len; \
        U32 hash_PeRlHaSh = (PERL_HASH_SEED_U16_x(len & 0xff) << 16) + len; \
        while (s_PeRlHaSh < end_PeRlHaSh) { \
            hash_PeRlHaSh ^= PERL_HASH_SEED_U16_x((U8)*s_PeRlHaSh++); \
            hash_PeRlHaSh += BUZZHASH_ROTL32(hash_PeRlHaSh,11); \
        } \
        hash_PeRlHaSh += (hash_PeRlHaSh << 3); \
        hash_PeRlHaSh ^= (hash_PeRlHaSh >> 11); \
        (hash) = (hash_PeRlHaSh + (hash_PeRlHaSh << 15)); \
    } STMT_END

#elif defined(PERL_HASH_FUNC_SIPHASH)
#define PERL_HASH_FUNC "SIPHASH"
#define PERL_HASH_SEED_BYTES 16

/* This is SipHash by Jean-Philippe Aumasson and Daniel J. Bernstein.
 * The authors claim it is relatively secure compared to the alternatives
 * and that performance wise it is a suitable hash for languages like Perl.
 * See:
 *
 * https://www.131002.net/siphash/
 *
 * This implementation seems to perform slightly slower than one-at-a-time for
 * short keys, but degrades slower for longer keys. Murmur Hash outperforms it
 * regardless of keys size.
 *
 * It is 64 bit only.
 */

#define PERL_HASH_NEEDS_TWO_SEEDS

#ifndef U64
#define U64 uint64_t
#endif

#define ROTL(x,b) (U64)( ((x) << (b)) | ( (x) >> (64 - (b))) )

#define U32TO8_LE(p, v)         \
    (p)[0] = (U8)((v)      ); (p)[1] = (U8)((v) >>  8); \
    (p)[2] = (U8)((v) >> 16); (p)[3] = (U8)((v) >> 24);

#define U64TO8_LE(p, v)         \
  U32TO8_LE((p),     (U32)((v)      ));   \
  U32TO8_LE((p) + 4, (U32)((v) >> 32));

#define U8TO64_LE(p) \
  (((U64)((p)[0])      ) | \
   ((U64)((p)[1]) <<  8) | \
   ((U64)((p)[2]) << 16) | \
   ((U64)((p)[3]) << 24) | \
   ((U64)((p)[4]) << 32) | \
   ((U64)((p)[5]) << 40) | \
   ((U64)((p)[6]) << 48) | \
   ((U64)((p)[7]) << 56))

#define SIPROUND            \
  do {              \
    v0_PeRlHaSh += v1_PeRlHaSh; v1_PeRlHaSh=ROTL(v1_PeRlHaSh,13); v1_PeRlHaSh ^= v0_PeRlHaSh; v0_PeRlHaSh=ROTL(v0_PeRlHaSh,32); \
    v2_PeRlHaSh += v3_PeRlHaSh; v3_PeRlHaSh=ROTL(v3_PeRlHaSh,16); v3_PeRlHaSh ^= v2_PeRlHaSh;     \
    v0_PeRlHaSh += v3_PeRlHaSh; v3_PeRlHaSh=ROTL(v3_PeRlHaSh,21); v3_PeRlHaSh ^= v0_PeRlHaSh;     \
    v2_PeRlHaSh += v1_PeRlHaSh; v1_PeRlHaSh=ROTL(v1_PeRlHaSh,17); v1_PeRlHaSh ^= v2_PeRlHaSh; v2_PeRlHaSh=ROTL(v2_PeRlHaSh,32); \
  } while(0)

/* SipHash-2-4 */
#define PERL_HASH(hash,str,len) STMT_START { \
  const char * const strtmp_PeRlHaSh = (str); \
  const unsigned char *in_PeRlHaSh = (const unsigned char *)strtmp_PeRlHaSh; \
  const U32 inlen_PeRlHaSh = (len); \
  /* "somepseudorandomlygeneratedbytes" */ \
  U64 v0_PeRlHaSh = 0x736f6d6570736575ULL; \
  U64 v1_PeRlHaSh = 0x646f72616e646f6dULL; \
  U64 v2_PeRlHaSh = 0x6c7967656e657261ULL; \
  U64 v3_PeRlHaSh = 0x7465646279746573ULL; \
\
  U64 b_PeRlHaSh;                           \
  U64 k0_PeRlHaSh = PERL_HASH_SEED_U64_1;   \
  U64 k1_PeRlHaSh = PERL_HASH_SEED_U64_2;   \
  U64 m_PeRlHaSh;                           \
  const int left_PeRlHaSh = inlen_PeRlHaSh & 7; \
  const U8 *end_PeRlHaSh = in_PeRlHaSh + inlen_PeRlHaSh - left_PeRlHaSh; \
\
  b_PeRlHaSh = ( ( U64 )(len) ) << 56; \
  v3_PeRlHaSh ^= k1_PeRlHaSh; \
  v2_PeRlHaSh ^= k0_PeRlHaSh; \
  v1_PeRlHaSh ^= k1_PeRlHaSh; \
  v0_PeRlHaSh ^= k0_PeRlHaSh; \
\
  for ( ; in_PeRlHaSh != end_PeRlHaSh; in_PeRlHaSh += 8 ) \
  { \
    m_PeRlHaSh = U8TO64_LE( in_PeRlHaSh ); \
    v3_PeRlHaSh ^= m_PeRlHaSh; \
    SIPROUND; \
    SIPROUND; \
    v0_PeRlHaSh ^= m_PeRlHaSh; \
  } \
\
  switch( left_PeRlHaSh ) \
  { \
  case 7: b_PeRlHaSh |= ( ( U64 )in_PeRlHaSh[ 6] )  << 48; \
  case 6: b_PeRlHaSh |= ( ( U64 )in_PeRlHaSh[ 5] )  << 40; \
  case 5: b_PeRlHaSh |= ( ( U64 )in_PeRlHaSh[ 4] )  << 32; \
  case 4: b_PeRlHaSh |= ( ( U64 )in_PeRlHaSh[ 3] )  << 24; \
  case 3: b_PeRlHaSh |= ( ( U64 )in_PeRlHaSh[ 2] )  << 16; \
  case 2: b_PeRlHaSh |= ( ( U64 )in_PeRlHaSh[ 1] )  <<  8; \
  case 1: b_PeRlHaSh |= ( ( U64 )in_PeRlHaSh[ 0] ); break; \
  case 0: break; \
  } \
\
  v3_PeRlHaSh ^= b_PeRlHaSh; \
  SIPROUND; \
  SIPROUND; \
  v0_PeRlHaSh ^= b_PeRlHaSh; \
\
  v2_PeRlHaSh ^= 0xff; \
  SIPROUND; \
  SIPROUND; \
  SIPROUND; \
  SIPROUND; \
  b_PeRlHaSh = v0_PeRlHaSh ^ v1_PeRlHaSh ^ v2_PeRlHaSh  ^ v3_PeRlHaSh; \
  (hash)= (U32)(b_PeRlHaSh & U32_MAX); \
} STMT_END

#elif defined(PERL_HASH_FUNC_SUPERFAST)
#define PERL_HASH_FUNC "SUPERFAST"
#define PERL_HASH_SEED_BYTES 4
/* FYI: This is the "Super-Fast" algorithm mentioned by Bob Jenkins in
 * (http://burtleburtle.net/bob/hash/doobs.html)
 * It is by Paul Hsieh (c) 2004 and is analysed here
 * http://www.azillionmonkeys.com/qed/hash.html
 * license terms are here:
 * http://www.azillionmonkeys.com/qed/weblicense.html
 */
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const U16 *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((const U8 *)(d))[1] << UINT32_C(8))\
                      +((const U8 *)(d))[0])
#endif
#define PERL_HASH(hash,str,len) \
      STMT_START        { \
        const char * const strtmp_PeRlHaSh = (str); \
        const unsigned char *str_PeRlHaSh = (const unsigned char *)strtmp_PeRlHaSh; \
        U32 len_PeRlHaSh = (len); \
        U32 hash_PeRlHaSh = PERL_HASH_SEED_U32 ^ len; \
        U32 tmp_PeRlHaSh; \
        int rem_PeRlHaSh= len_PeRlHaSh & 3; \
        len_PeRlHaSh >>= 2; \
                            \
        for (;len_PeRlHaSh > 0; len_PeRlHaSh--) { \
            hash_PeRlHaSh  += get16bits (str_PeRlHaSh); \
            tmp_PeRlHaSh    = (get16bits (str_PeRlHaSh+2) << 11) ^ hash_PeRlHaSh; \
            hash_PeRlHaSh   = (hash_PeRlHaSh << 16) ^ tmp_PeRlHaSh; \
            str_PeRlHaSh   += 2 * sizeof (U16); \
            hash_PeRlHaSh  += hash_PeRlHaSh >> 11; \
        } \
        \
        /* Handle end cases */ \
        switch (rem_PeRlHaSh) { \
            case 3: hash_PeRlHaSh += get16bits (str_PeRlHaSh); \
                    hash_PeRlHaSh ^= hash_PeRlHaSh << 16; \
                    hash_PeRlHaSh ^= str_PeRlHaSh[sizeof (U16)] << 18; \
                    hash_PeRlHaSh += hash_PeRlHaSh >> 11; \
                    break; \
            case 2: hash_PeRlHaSh += get16bits (str_PeRlHaSh); \
                    hash_PeRlHaSh ^= hash_PeRlHaSh << 11; \
                    hash_PeRlHaSh += hash_PeRlHaSh >> 17; \
                    break; \
            case 1: hash_PeRlHaSh += *str_PeRlHaSh; \
                    hash_PeRlHaSh ^= hash_PeRlHaSh << 10; \
                    hash_PeRlHaSh += hash_PeRlHaSh >> 1; \
        } \
        \
        /* Force "avalanching" of final 127 bits */ \
        hash_PeRlHaSh ^= hash_PeRlHaSh << 3; \
        hash_PeRlHaSh += hash_PeRlHaSh >> 5; \
        hash_PeRlHaSh ^= hash_PeRlHaSh << 4; \
        hash_PeRlHaSh += hash_PeRlHaSh >> 17; \
        hash_PeRlHaSh ^= hash_PeRlHaSh << 25; \
        (hash) = (hash_PeRlHaSh + (hash_PeRlHaSh >> 6)); \
    } STMT_END

#elif defined(PERL_HASH_FUNC_MURMUR3)
#define PERL_HASH_FUNC "MURMUR3"
#define PERL_HASH_SEED_BYTES 4

/*-----------------------------------------------------------------------------
 * MurmurHash3 was written by Austin Appleby, and is placed in the public
 * domain.
 *
 * This implementation was originally written by Shane Day, and is also public domain,
 * and was modified to function as a macro similar to other perl hash functions by
 * Yves Orton.
 *
 * This is a portable ANSI C implementation of MurmurHash3_x86_32 (Murmur3A)
 * with support for progressive processing.
 *
 * If you want to understand the MurmurHash algorithm you would be much better
 * off reading the original source. Just point your browser at:
 * http://code.google.com/p/smhasher/source/browse/trunk/MurmurHash3.cpp
 *
 * How does it work?
 *
 * We can only process entire 32 bit chunks of input, except for the very end
 * that may be shorter.
 *
 * To handle endianess I simply use a macro that reads a U32 and define
 * that macro to be a direct read on little endian machines, a read and swap
 * on big endian machines, or a byte-by-byte read if the endianess is unknown.
 */


/*-----------------------------------------------------------------------------
 * Endianess, misalignment capabilities and util macros
 *
 * The following 3 macros are defined in this section. The other macros defined
 * are only needed to help derive these 3.
 *
 * MURMUR_READ_UINT32(x)   Read a little endian unsigned 32-bit int
 * MURMUR_UNALIGNED_SAFE   Defined if READ_UINT32 works on non-word boundaries
 * MURMUR_ROTL32(x,r)      Rotate x left by r bits
 */

/* Now find best way we can to READ_UINT32 */
#if (BYTEORDER == 0x1234 || BYTEORDER == 0x12345678) && U32SIZE == 4
  /* CPU endian matches murmurhash algorithm, so read 32-bit word directly */
  #define MURMUR_READ_UINT32(ptr)   (*((U32*)(ptr)))
#elif BYTEORDER == 0x4321 || BYTEORDER == 0x87654321
  /* TODO: Add additional cases below where a compiler provided bswap32 is available */
  #if defined(__GNUC__) && (__GNUC__>4 || (__GNUC__==4 && __GNUC_MINOR__>=3))
    #define MURMUR_READ_UINT32(ptr)   (__builtin_bswap32(*((U32*)(ptr))))
  #else
    /* Without a known fast bswap32 we're just as well off doing this */
    #define MURMUR_READ_UINT32(ptr)   (ptr[0]|ptr[1]<<8|ptr[2]<<16|ptr[3]<<24)
    #define MURMUR_UNALIGNED_SAFE
  #endif
#else
  /* Unknown endianess so last resort is to read individual bytes */
  #define MURMUR_READ_UINT32(ptr)   (ptr[0]|ptr[1]<<8|ptr[2]<<16|ptr[3]<<24)

  /* Since we're not doing word-reads we can skip the messing about with realignment */
  #define MURMUR_UNALIGNED_SAFE
#endif

/* Find best way to ROTL32 */
#if defined(_MSC_VER)
  #include <stdlib.h>  /* Microsoft put _rotl declaration in here */
  #define MURMUR_ROTL32(x,r)  _rotl(x,r)
#else
  /* gcc recognises this code and generates a rotate instruction for CPUs with one */
  #define MURMUR_ROTL32(x,r)  (((U32)x << r) | ((U32)x >> (32 - r)))
#endif


/*-----------------------------------------------------------------------------
 * Core murmurhash algorithm macros */

#define MURMUR_C1  (0xcc9e2d51)
#define MURMUR_C2  (0x1b873593)
#define MURMUR_C3  (0xe6546b64)
#define MURMUR_C4  (0x85ebca6b)
#define MURMUR_C5  (0xc2b2ae35)

/* This is the main processing body of the algorithm. It operates
 * on each full 32-bits of input. */
#define MURMUR_DOBLOCK(h1, k1) STMT_START { \
    k1 *= MURMUR_C1; \
    k1 = MURMUR_ROTL32(k1,15); \
    k1 *= MURMUR_C2; \
    \
    h1 ^= k1; \
    h1 = MURMUR_ROTL32(h1,13); \
    h1 = h1 * 5 + MURMUR_C3; \
} STMT_END


/* Append unaligned bytes to carry, forcing hash churn if we have 4 bytes */
/* cnt=bytes to process, h1=name of h1 var, c=carry, n=bytes in c, ptr/len=payload */
#define MURMUR_DOBYTES(cnt, h1, c, n, ptr, len) STMT_START { \
    int MURMUR_DOBYTES_i = cnt; \
    while(MURMUR_DOBYTES_i--) { \
        c = c>>8 | *ptr++<<24; \
        n++; len--; \
        if(n==4) { \
            MURMUR_DOBLOCK(h1, c); \
            n = 0; \
        } \
    } \
} STMT_END

/* process the last 1..3 bytes and finalize */
#define MURMUR_FINALIZE(hash, PeRlHaSh_len, PeRlHaSh_k1, PeRlHaSh_h1, PeRlHaSh_carry, PeRlHaSh_bytes_in_carry, PeRlHaSh_ptr, PeRlHaSh_total_length) STMT_START { \
    /* Advance over whole 32-bit chunks, possibly leaving 1..3 bytes */\
    PeRlHaSh_len -= PeRlHaSh_len/4*4;                           \
                                                                \
    /* Append any remaining bytes into carry */                 \
    MURMUR_DOBYTES(PeRlHaSh_len, PeRlHaSh_h1, PeRlHaSh_carry, PeRlHaSh_bytes_in_carry, PeRlHaSh_ptr, PeRlHaSh_len); \
                                                                \
    if (PeRlHaSh_bytes_in_carry) {                                           \
        PeRlHaSh_k1 = PeRlHaSh_carry >> ( 4 - PeRlHaSh_bytes_in_carry ) * 8; \
        PeRlHaSh_k1 *= MURMUR_C1;                               \
        PeRlHaSh_k1 = MURMUR_ROTL32(PeRlHaSh_k1,15);                   \
        PeRlHaSh_k1 *= MURMUR_C2;                               \
        PeRlHaSh_h1 ^= PeRlHaSh_k1;                             \
    }                                                           \
    PeRlHaSh_h1 ^= PeRlHaSh_total_length;                       \
                                                                \
    /* fmix */                                                  \
    PeRlHaSh_h1 ^= PeRlHaSh_h1 >> 16;                           \
    PeRlHaSh_h1 *= MURMUR_C4;                                   \
    PeRlHaSh_h1 ^= PeRlHaSh_h1 >> 13;                           \
    PeRlHaSh_h1 *= MURMUR_C5;                                   \
    PeRlHaSh_h1 ^= PeRlHaSh_h1 >> 16;                           \
    (hash)= PeRlHaSh_h1;                                        \
} STMT_END

/* now we create the hash function */

#if defined(UNALIGNED_SAFE)
#define PERL_HASH(hash,str,len) STMT_START { \
        const char * const s_PeRlHaSh_tmp = (str); \
        const unsigned char *PeRlHaSh_ptr = (const unsigned char *)s_PeRlHaSh_tmp; \
        I32 PeRlHaSh_len = len;    \
                                            \
        U32 PeRlHaSh_h1 = PERL_HASH_SEED_U32;   \
        U32 PeRlHaSh_k1;                    \
        U32 PeRlHaSh_carry = 0;             \
                                            \
        const unsigned char *PeRlHaSh_end;  \
                                            \
        int PeRlHaSh_bytes_in_carry = 0; /* bytes in carry */ \
        I32 PeRlHaSh_total_length= PeRlHaSh_len; \
                                            \
        /* This CPU handles unaligned word access */            \
        /* Process 32-bit chunks */                             \
        PeRlHaSh_end = PeRlHaSh_ptr + PeRlHaSh_len/4*4;         \
        for( ; PeRlHaSh_ptr < PeRlHaSh_end ; PeRlHaSh_ptr+=4) { \
            PeRlHaSh_k1 = MURMUR_READ_UINT32(PeRlHaSh_ptr);        \
            MURMUR_DOBLOCK(PeRlHaSh_h1, PeRlHaSh_k1);                  \
        }                                                       \
        \
        MURMUR_FINALIZE(hash, PeRlHaSh_len, PeRlHaSh_k1, PeRlHaSh_h1, PeRlHaSh_carry, PeRlHaSh_bytes_in_carry, PeRlHaSh_ptr, PeRlHaSh_total_length);\
    } STMT_END
#else
#define PERL_HASH(hash,str,len) STMT_START { \
        const char * const s_PeRlHaSh_tmp = (str); \
        const unsigned char *PeRlHaSh_ptr = (const unsigned char *)s_PeRlHaSh_tmp; \
        I32 PeRlHaSh_len = len;    \
                                            \
        U32 PeRlHaSh_h1 = PERL_HASH_SEED_U32;   \
        U32 PeRlHaSh_k1;                    \
        U32 PeRlHaSh_carry = 0;             \
                                            \
        const unsigned char *PeRlHaSh_end;  \
                                            \
        int PeRlHaSh_bytes_in_carry = 0; /* bytes in carry */ \
        I32 PeRlHaSh_total_length= PeRlHaSh_len; \
                                            \
        /* This CPU does not handle unaligned word access */    \
                                                                \
        /* Consume enough so that the next data byte is word aligned */ \
        int PeRlHaSh_i = -(long)PeRlHaSh_ptr & 3;                       \
        if(PeRlHaSh_i && PeRlHaSh_i <= PeRlHaSh_len) {                  \
          MURMUR_DOBYTES(PeRlHaSh_i, PeRlHaSh_h1, PeRlHaSh_carry, PeRlHaSh_bytes_in_carry, PeRlHaSh_ptr, PeRlHaSh_len);\
        }                                                               \
        \
        /* We're now aligned. Process in aligned blocks. Specialise for each possible carry count */ \
        PeRlHaSh_end = PeRlHaSh_ptr + PeRlHaSh_len/4*4;                 \
        switch(PeRlHaSh_bytes_in_carry) { /* how many bytes in carry */                  \
            case 0: /* c=[----]  w=[3210]  b=[3210]=w            c'=[----] */ \
            for( ; PeRlHaSh_ptr < PeRlHaSh_end ; PeRlHaSh_ptr+=4) { \
                PeRlHaSh_k1 = MURMUR_READ_UINT32(PeRlHaSh_ptr);        \
                MURMUR_DOBLOCK(PeRlHaSh_h1, PeRlHaSh_k1);                  \
            }                                                       \
            break;                                                  \
            case 1: /* c=[0---]  w=[4321]  b=[3210]=c>>24|w<<8   c'=[4---] */   \
            for( ; PeRlHaSh_ptr < PeRlHaSh_end ; PeRlHaSh_ptr+=4) { \
                PeRlHaSh_k1 = PeRlHaSh_carry>>24;                   \
                PeRlHaSh_carry = MURMUR_READ_UINT32(PeRlHaSh_ptr);             \
                PeRlHaSh_k1 |= PeRlHaSh_carry<<8;                       \
                MURMUR_DOBLOCK(PeRlHaSh_h1, PeRlHaSh_k1);                  \
            }                                                       \
            break;                                                  \
            case 2: /* c=[10--]  w=[5432]  b=[3210]=c>>16|w<<16  c'=[54--] */   \
            for( ; PeRlHaSh_ptr < PeRlHaSh_end ; PeRlHaSh_ptr+=4) { \
                PeRlHaSh_k1 = PeRlHaSh_carry>>16;                   \
                PeRlHaSh_carry = MURMUR_READ_UINT32(PeRlHaSh_ptr);             \
                PeRlHaSh_k1 |= PeRlHaSh_carry<<16;                      \
                MURMUR_DOBLOCK(PeRlHaSh_h1, PeRlHaSh_k1);                  \
            }                                                       \
            break;                                                  \
            case 3: /* c=[210-]  w=[6543]  b=[3210]=c>>8|w<<24   c'=[654-] */   \
            for( ; PeRlHaSh_ptr < PeRlHaSh_end ; PeRlHaSh_ptr+=4) { \
                PeRlHaSh_k1 = PeRlHaSh_carry>>8;                    \
                PeRlHaSh_carry = MURMUR_READ_UINT32(PeRlHaSh_ptr);             \
                PeRlHaSh_k1 |= PeRlHaSh_carry<<24;                      \
                MURMUR_DOBLOCK(PeRlHaSh_h1, PeRlHaSh_k1);                  \
            }                                                       \
        }                                                           \
                                                                    \
        MURMUR_FINALIZE(hash, PeRlHaSh_len, PeRlHaSh_k1, PeRlHaSh_h1, PeRlHaSh_carry, PeRlHaSh_bytes_in_carry, PeRlHaSh_ptr, PeRlHaSh_total_length);\
    } STMT_END
#endif

#elif defined(PERL_HASH_FUNC_DJB2)
#define PERL_HASH_FUNC "DJB2"
#define PERL_HASH_SEED_BYTES 4
#define PERL_HASH(hash,str,len) \
     STMT_START        { \
        const char * const s_PeRlHaSh_tmp = (str); \
        const unsigned char *s_PeRlHaSh = (const unsigned char *)s_PeRlHaSh_tmp; \
        I32 i_PeRlHaSh = len; \
        U32 hash_PeRlHaSh = PERL_HASH_SEED_U32 ^ len; \
        while (i_PeRlHaSh--) { \
            hash_PeRlHaSh = ((hash_PeRlHaSh << 5) + hash_PeRlHaSh) + *s_PeRlHaSh++; \
        } \
        (hash) = hash_PeRlHaSh;\
    } STMT_END

#elif defined(PERL_HASH_FUNC_SDBM)
#define PERL_HASH_FUNC "SDBM"
#define PERL_HASH_SEED_BYTES 4
#define PERL_HASH(hash,str,len) \
     STMT_START        { \
        const char * const s_PeRlHaSh_tmp = (str); \
        const unsigned char *s_PeRlHaSh = (const unsigned char *)s_PeRlHaSh_tmp; \
        I32 i_PeRlHaSh = len; \
        U32 hash_PeRlHaSh = PERL_HASH_SEED_U32 ^ len; \
        while (i_PeRlHaSh--) { \
            hash_PeRlHaSh = (hash_PeRlHaSh << 6) + (hash_PeRlHaSh << 16) - hash_PeRlHaSh + *s_PeRlHaSh++; \
        } \
        (hash) = hash_PeRlHaSh;\
    } STMT_END

#elif defined(PERL_HASH_FUNC_ONE_AT_A_TIME)
/* DEFAULT/HISTORIC HASH FUNCTION */
#define PERL_HASH_FUNC "ONE_AT_A_TIME"
#define PERL_HASH_SEED_BYTES 4

/* FYI: This is the "One-at-a-Time" algorithm by Bob Jenkins
 * from requirements by Colin Plumb.
 * (http://burtleburtle.net/bob/hash/doobs.html) */
#define PERL_HASH(hash,str,len) \
     STMT_START	{ \
        const char * const s_PeRlHaSh_tmp = (str); \
        const unsigned char *s_PeRlHaSh = (const unsigned char *)s_PeRlHaSh_tmp; \
        I32 i_PeRlHaSh = len; \
        U32 hash_PeRlHaSh = PERL_HASH_SEED_U32 ^ len; \
	while (i_PeRlHaSh--) { \
            hash_PeRlHaSh += (U8)*s_PeRlHaSh++; \
	    hash_PeRlHaSh += (hash_PeRlHaSh << 10); \
	    hash_PeRlHaSh ^= (hash_PeRlHaSh >> 6); \
	} \
	hash_PeRlHaSh += (hash_PeRlHaSh << 3); \
	hash_PeRlHaSh ^= (hash_PeRlHaSh >> 11); \
	(hash) = (hash_PeRlHaSh + (hash_PeRlHaSh << 15)); \
    } STMT_END
#endif
#ifndef PERL_HASH
#error "No hash function defined!"
#endif
/*
=head1 Hash Manipulation Functions

=for apidoc AmU||HEf_SVKEY
This flag, used in the length slot of hash entries and magic structures,
specifies the structure contains an C<SV*> pointer where a C<char*> pointer
is to be expected. (For information only--not to be used).

=head1 Handy Values

=for apidoc AmU||Nullhv
Null HV pointer.

(deprecated - use C<(HV *)NULL> instead)

=head1 Hash Manipulation Functions

=for apidoc Am|char*|HvNAME|HV* stash
Returns the package name of a stash, or NULL if C<stash> isn't a stash.
See C<SvSTASH>, C<CvSTASH>.

=for apidoc Am|STRLEN|HvNAMELEN|HV *stash
Returns the length of the stash's name.

=for apidoc Am|unsigned char|HvNAMEUTF8|HV *stash
Returns true if the name is in UTF8 encoding.

=for apidoc Am|char*|HvENAME|HV* stash
Returns the effective name of a stash, or NULL if there is none. The
effective name represents a location in the symbol table where this stash
resides. It is updated automatically when packages are aliased or deleted.
A stash that is no longer in the symbol table has no effective name. This
name is preferable to C<HvNAME> for use in MRO linearisations and isa
caches.

=for apidoc Am|STRLEN|HvENAMELEN|HV *stash
Returns the length of the stash's effective name.

=for apidoc Am|unsigned char|HvENAMEUTF8|HV *stash
Returns true if the effective name is in UTF8 encoding.

=for apidoc Am|void*|HeKEY|HE* he
Returns the actual pointer stored in the key slot of the hash entry. The
pointer may be either C<char*> or C<SV*>, depending on the value of
C<HeKLEN()>.  Can be assigned to.  The C<HePV()> or C<HeSVKEY()> macros are
usually preferable for finding the value of a key.

=for apidoc Am|STRLEN|HeKLEN|HE* he
If this is negative, and amounts to C<HEf_SVKEY>, it indicates the entry
holds an C<SV*> key.  Otherwise, holds the actual length of the key.  Can
be assigned to. The C<HePV()> macro is usually preferable for finding key
lengths.

=for apidoc Am|SV*|HeVAL|HE* he
Returns the value slot (type C<SV*>) stored in the hash entry. Can be assigned
to.

  SV *foo= HeVAL(hv);
  HeVAL(hv)= sv;


=for apidoc Am|U32|HeHASH|HE* he
Returns the computed hash stored in the hash entry.

=for apidoc Am|char*|HePV|HE* he|STRLEN len
Returns the key slot of the hash entry as a C<char*> value, doing any
necessary dereferencing of possibly C<SV*> keys.  The length of the string
is placed in C<len> (this is a macro, so do I<not> use C<&len>).  If you do
not care about what the length of the key is, you may use the global
variable C<PL_na>, though this is rather less efficient than using a local
variable.  Remember though, that hash keys in perl are free to contain
embedded nulls, so using C<strlen()> or similar is not a good way to find
the length of hash keys. This is very similar to the C<SvPV()> macro
described elsewhere in this document. See also C<HeUTF8>.

If you are using C<HePV> to get values to pass to C<newSVpvn()> to create a
new SV, you should consider using C<newSVhek(HeKEY_hek(he))> as it is more
efficient.

=for apidoc Am|char*|HeUTF8|HE* he
Returns whether the C<char *> value returned by C<HePV> is encoded in UTF-8,
doing any necessary dereferencing of possibly C<SV*> keys.  The value returned
will be 0 or non-0, not necessarily 1 (or even a value with any low bits set),
so B<do not> blindly assign this to a C<bool> variable, as C<bool> may be a
typedef for C<char>.

=for apidoc Am|SV*|HeSVKEY|HE* he
Returns the key as an C<SV*>, or C<NULL> if the hash entry does not
contain an C<SV*> key.

=for apidoc Am|SV*|HeSVKEY_force|HE* he
Returns the key as an C<SV*>.  Will create and return a temporary mortal
C<SV*> if the hash entry contains only a C<char*> key.

=for apidoc Am|SV*|HeSVKEY_set|HE* he|SV* sv
Sets the key to a given C<SV*>, taking care to set the appropriate flags to
indicate the presence of an C<SV*> key, and returns the same
C<SV*>.

=cut
*/

/* these hash entry flags ride on hent_klen (for use only in magic/tied HVs) */
#define HEf_SVKEY	-2	/* hent_key is an SV* */

#ifndef PERL_CORE
#  define Nullhv Null(HV*)
#endif
#define HvARRAY(hv)	((hv)->sv_u.svu_hash)
#define HvFILL(hv)	Perl_hv_fill(aTHX_ (const HV *)(hv))
#define HvMAX(hv)	((XPVHV*)  SvANY(hv))->xhv_max
/* This quite intentionally does no flag checking first. That's your
   responsibility.  */
#define HvAUX(hv)	((struct xpvhv_aux*)&(HvARRAY(hv)[HvMAX(hv)+1]))
#define HvRITER(hv)	(*Perl_hv_riter_p(aTHX_ MUTABLE_HV(hv)))
#define HvEITER(hv)	(*Perl_hv_eiter_p(aTHX_ MUTABLE_HV(hv)))
#define HvRITER_set(hv,r)	Perl_hv_riter_set(aTHX_ MUTABLE_HV(hv), r)
#define HvEITER_set(hv,e)	Perl_hv_eiter_set(aTHX_ MUTABLE_HV(hv), e)
#define HvRITER_get(hv)	(SvOOK(hv) ? HvAUX(hv)->xhv_riter : -1)
#define HvEITER_get(hv)	(SvOOK(hv) ? HvAUX(hv)->xhv_eiter : NULL)
#define HvNAME(hv)	HvNAME_get(hv)
#define HvNAMELEN(hv)   HvNAMELEN_get(hv)
#define HvENAME(hv)	HvENAME_get(hv)
#define HvENAMELEN(hv)  HvENAMELEN_get(hv)

/* Checking that hv is a valid package stash is the
   caller's responsibility */
#define HvMROMETA(hv) (HvAUX(hv)->xhv_mro_meta \
                       ? HvAUX(hv)->xhv_mro_meta \
                       : Perl_mro_meta_init(aTHX_ hv))

#define HvNAME_HEK_NN(hv)			  \
 (						  \
  HvAUX(hv)->xhv_name_count			  \
  ? *HvAUX(hv)->xhv_name_u.xhvnameu_names	  \
  : HvAUX(hv)->xhv_name_u.xhvnameu_name		  \
 )
/* This macro may go away without notice.  */
#define HvNAME_HEK(hv) \
	(SvOOK(hv) && HvAUX(hv)->xhv_name_u.xhvnameu_name ? HvNAME_HEK_NN(hv) : NULL)
#define HvNAME_get(hv) \
	((SvOOK(hv) && HvAUX(hv)->xhv_name_u.xhvnameu_name && HvNAME_HEK_NN(hv)) \
			 ? HEK_KEY(HvNAME_HEK_NN(hv)) : NULL)
#define HvNAMELEN_get(hv) \
	((SvOOK(hv) && HvAUX(hv)->xhv_name_u.xhvnameu_name && HvNAME_HEK_NN(hv)) \
				 ? HEK_LEN(HvNAME_HEK_NN(hv)) : 0)
#define HvNAMEUTF8(hv) \
	((SvOOK(hv) && HvAUX(hv)->xhv_name_u.xhvnameu_name && HvNAME_HEK_NN(hv)) \
				 ? HEK_UTF8(HvNAME_HEK_NN(hv)) : 0)
#define HvENAME_HEK_NN(hv)                                             \
 (                                                                      \
  HvAUX(hv)->xhv_name_count > 0   ? HvAUX(hv)->xhv_name_u.xhvnameu_names[0] : \
  HvAUX(hv)->xhv_name_count < -1  ? HvAUX(hv)->xhv_name_u.xhvnameu_names[1] : \
  HvAUX(hv)->xhv_name_count == -1 ? NULL                              : \
                                    HvAUX(hv)->xhv_name_u.xhvnameu_name \
 )
#define HvENAME_HEK(hv) \
	(SvOOK(hv) && HvAUX(hv)->xhv_name_u.xhvnameu_name ? HvENAME_HEK_NN(hv) : NULL)
#define HvENAME_get(hv) \
   ((SvOOK(hv) && HvAUX(hv)->xhv_name_u.xhvnameu_name && HvAUX(hv)->xhv_name_count != -1) \
			 ? HEK_KEY(HvENAME_HEK_NN(hv)) : NULL)
#define HvENAMELEN_get(hv) \
   ((SvOOK(hv) && HvAUX(hv)->xhv_name_u.xhvnameu_name && HvAUX(hv)->xhv_name_count != -1) \
				 ? HEK_LEN(HvENAME_HEK_NN(hv)) : 0)
#define HvENAMEUTF8(hv) \
   ((SvOOK(hv) && HvAUX(hv)->xhv_name_u.xhvnameu_name && HvAUX(hv)->xhv_name_count != -1) \
				 ? HEK_UTF8(HvENAME_HEK_NN(hv)) : 0)

/* the number of keys (including any placeholders) */
#define XHvTOTALKEYS(xhv)	((xhv)->xhv_keys)

/*
 * HvKEYS gets the number of keys that actually exist(), and is provided
 * for backwards compatibility with old XS code. The core uses HvUSEDKEYS
 * (keys, excluding placeholders) and HvTOTALKEYS (including placeholders)
 */
#define HvKEYS(hv)		HvUSEDKEYS(hv)
#define HvUSEDKEYS(hv)		(HvTOTALKEYS(hv) - HvPLACEHOLDERS_get(hv))
#define HvTOTALKEYS(hv)		XHvTOTALKEYS((XPVHV*)  SvANY(hv))
#define HvPLACEHOLDERS(hv)	(*Perl_hv_placeholders_p(aTHX_ MUTABLE_HV(hv)))
#define HvPLACEHOLDERS_get(hv)	(SvMAGIC(hv) ? Perl_hv_placeholders_get(aTHX_ (const HV *)hv) : 0)
#define HvPLACEHOLDERS_set(hv,p)	Perl_hv_placeholders_set(aTHX_ MUTABLE_HV(hv), p)

#define HvSHAREKEYS(hv)		(SvFLAGS(hv) & SVphv_SHAREKEYS)
#define HvSHAREKEYS_on(hv)	(SvFLAGS(hv) |= SVphv_SHAREKEYS)
#define HvSHAREKEYS_off(hv)	(SvFLAGS(hv) &= ~SVphv_SHAREKEYS)

/* This is an optimisation flag. It won't be set if all hash keys have a 0
 * flag. Currently the only flags relate to utf8.
 * Hence it won't be set if all keys are 8 bit only. It will be set if any key
 * is utf8 (including 8 bit keys that were entered as utf8, and need upgrading
 * when retrieved during iteration. It may still be set when there are no longer
 * any utf8 keys.
 * See HVhek_ENABLEHVKFLAGS for the trigger.
 */
#define HvHASKFLAGS(hv)		(SvFLAGS(hv) & SVphv_HASKFLAGS)
#define HvHASKFLAGS_on(hv)	(SvFLAGS(hv) |= SVphv_HASKFLAGS)
#define HvHASKFLAGS_off(hv)	(SvFLAGS(hv) &= ~SVphv_HASKFLAGS)

#define HvLAZYDEL(hv)		(SvFLAGS(hv) & SVphv_LAZYDEL)
#define HvLAZYDEL_on(hv)	(SvFLAGS(hv) |= SVphv_LAZYDEL)
#define HvLAZYDEL_off(hv)	(SvFLAGS(hv) &= ~SVphv_LAZYDEL)

#ifndef PERL_CORE
#  define Nullhe Null(HE*)
#endif
#define HeNEXT(he)		(he)->hent_next
#define HeKEY_hek(he)		(he)->hent_hek
#define HeKEY(he)		HEK_KEY(HeKEY_hek(he))
#define HeKEY_sv(he)		(*(SV**)HeKEY(he))
#define HeKLEN(he)		HEK_LEN(HeKEY_hek(he))
#define HeKUTF8(he)  HEK_UTF8(HeKEY_hek(he))
#define HeKWASUTF8(he)  HEK_WASUTF8(HeKEY_hek(he))
#define HeKLEN_UTF8(he)  (HeKUTF8(he) ? -HeKLEN(he) : HeKLEN(he))
#define HeKFLAGS(he)  HEK_FLAGS(HeKEY_hek(he))
#define HeVAL(he)		(he)->he_valu.hent_val
#define HeHASH(he)		HEK_HASH(HeKEY_hek(he))
#define HePV(he,lp)		((HeKLEN(he) == HEf_SVKEY) ?		\
				 SvPV(HeKEY_sv(he),lp) :		\
				 ((lp = HeKLEN(he)), HeKEY(he)))
#define HeUTF8(he)		((HeKLEN(he) == HEf_SVKEY) ?		\
				 SvUTF8(HeKEY_sv(he)) :			\
				 (U32)HeKUTF8(he))

#define HeSVKEY(he)		((HeKEY(he) && 				\
				  HeKLEN(he) == HEf_SVKEY) ?		\
				 HeKEY_sv(he) : NULL)

#define HeSVKEY_force(he)	(HeKEY(he) ?				\
				 ((HeKLEN(he) == HEf_SVKEY) ?		\
				  HeKEY_sv(he) :			\
				  newSVpvn_flags(HeKEY(he),		\
						 HeKLEN(he), SVs_TEMP)) : \
				 &PL_sv_undef)
#define HeSVKEY_set(he,sv)	((HeKLEN(he) = HEf_SVKEY), (HeKEY_sv(he) = sv))

#ifndef PERL_CORE
#  define Nullhek Null(HEK*)
#endif
#define HEK_BASESIZE		STRUCT_OFFSET(HEK, hek_key[0])
#define HEK_HASH(hek)		(hek)->hek_hash
#define HEK_LEN(hek)		(hek)->hek_len
#define HEK_KEY(hek)		(hek)->hek_key
#define HEK_FLAGS(hek)	(*((unsigned char *)(HEK_KEY(hek))+HEK_LEN(hek)+1))

#define HVhek_UTF8	0x01 /* Key is utf8 encoded. */
#define HVhek_WASUTF8	0x02 /* Key is bytes here, but was supplied as utf8. */
#define HVhek_UNSHARED	0x08 /* This key isn't a shared hash key. */
#define HVhek_FREEKEY	0x100 /* Internal flag to say key is malloc()ed.  */
#define HVhek_PLACEHOLD	0x200 /* Internal flag to create placeholder.
                               * (may change, but Storable is a core module) */
#define HVhek_KEYCANONICAL 0x400 /* Internal flag - key is in canonical form.
				    If the string is UTF-8, it cannot be
				    converted to bytes. */
#define HVhek_MASK	0xFF

#define HVhek_ENABLEHVKFLAGS        (HVhek_MASK & ~(HVhek_UNSHARED))

#define HEK_UTF8(hek)		(HEK_FLAGS(hek) & HVhek_UTF8)
#define HEK_UTF8_on(hek)	(HEK_FLAGS(hek) |= HVhek_UTF8)
#define HEK_UTF8_off(hek)	(HEK_FLAGS(hek) &= ~HVhek_UTF8)
#define HEK_WASUTF8(hek)	(HEK_FLAGS(hek) & HVhek_WASUTF8)
#define HEK_WASUTF8_on(hek)	(HEK_FLAGS(hek) |= HVhek_WASUTF8)
#define HEK_WASUTF8_off(hek)	(HEK_FLAGS(hek) &= ~HVhek_WASUTF8)

/* calculate HV array allocation */
#ifndef PERL_USE_LARGE_HV_ALLOC
/* Default to allocating the correct size - default to assuming that malloc()
   is not broken and is efficient at allocating blocks sized at powers-of-two.
*/   
#  define PERL_HV_ARRAY_ALLOC_BYTES(size) ((size) * sizeof(HE*))
#else
#  define MALLOC_OVERHEAD 16
#  define PERL_HV_ARRAY_ALLOC_BYTES(size) \
			(((size) < 64)					\
			 ? (size) * sizeof(HE*)				\
			 : (size) * sizeof(HE*) * 2 - MALLOC_OVERHEAD)
#endif

/* Flags for hv_iternext_flags.  */
#define HV_ITERNEXT_WANTPLACEHOLDERS	0x01	/* Don't skip placeholders.  */

#define hv_iternext(hv)	hv_iternext_flags(hv, 0)
#define hv_magic(hv, gv, how) sv_magic(MUTABLE_SV(hv), MUTABLE_SV(gv), how, NULL, 0)
#define hv_undef(hv) Perl_hv_undef_flags(aTHX_ hv, 0)

#define Perl_sharepvn(pv, len, hash) HEK_KEY(share_hek(pv, len, hash))
#define sharepvn(pv, len, hash)	     Perl_sharepvn(pv, len, hash)

#define share_hek_hek(hek)						\
    (++(((struct shared_he *)(((char *)hek)				\
			      - STRUCT_OFFSET(struct shared_he,		\
					      shared_he_hek)))		\
	->shared_he_he.he_valu.hent_refcount),				\
     hek)

#define hv_store_ent(hv, keysv, val, hash)				\
    ((HE *) hv_common((hv), (keysv), NULL, 0, 0, HV_FETCH_ISSTORE,	\
		      (val), (hash)))

#define hv_exists_ent(hv, keysv, hash)					\
    (hv_common((hv), (keysv), NULL, 0, 0, HV_FETCH_ISEXISTS, 0, (hash))	\
     ? TRUE : FALSE)
#define hv_fetch_ent(hv, keysv, lval, hash)				\
    ((HE *) hv_common((hv), (keysv), NULL, 0, 0,			\
		      ((lval) ? HV_FETCH_LVALUE : 0), NULL, (hash)))
#define hv_delete_ent(hv, key, flags, hash)				\
    (MUTABLE_SV(hv_common((hv), (key), NULL, 0, 0, (flags) | HV_DELETE,	\
			  NULL, (hash))))

#define hv_store_flags(hv, key, klen, val, hash, flags)			\
    ((SV**) hv_common((hv), NULL, (key), (klen), (flags),		\
		      (HV_FETCH_ISSTORE|HV_FETCH_JUST_SV), (val),	\
		      (hash)))

#define hv_store(hv, key, klen, val, hash)				\
    ((SV**) hv_common_key_len((hv), (key), (klen),			\
			      (HV_FETCH_ISSTORE|HV_FETCH_JUST_SV),	\
			      (val), (hash)))

#define hv_exists(hv, key, klen)					\
    (hv_common_key_len((hv), (key), (klen), HV_FETCH_ISEXISTS, NULL, 0) \
     ? TRUE : FALSE)

#define hv_fetch(hv, key, klen, lval)					\
    ((SV**) hv_common_key_len((hv), (key), (klen), (lval)		\
			      ? (HV_FETCH_JUST_SV | HV_FETCH_LVALUE)	\
			      : HV_FETCH_JUST_SV, NULL, 0))

#define hv_delete(hv, key, klen, flags)					\
    (MUTABLE_SV(hv_common_key_len((hv), (key), (klen),			\
				  (flags) | HV_DELETE, NULL, 0)))

/* This refcounted he structure is used for storing the hints used for lexical
   pragmas. Without threads, it's basically struct he + refcount.
   With threads, life gets more complex as the structure needs to be shared
   between threads (because it hangs from OPs, which are shared), hence the
   alternate definition and mutex.  */

struct refcounted_he;

/* flags for the refcounted_he API */
#define REFCOUNTED_HE_KEY_UTF8		0x00000001
#ifdef PERL_CORE
# define REFCOUNTED_HE_EXISTS		0x00000002
#endif

#ifdef PERL_CORE

/* Gosh. This really isn't a good name any longer.  */
struct refcounted_he {
    struct refcounted_he *refcounted_he_next;	/* next entry in chain */
#ifdef USE_ITHREADS
    U32                   refcounted_he_hash;
    U32                   refcounted_he_keylen;
#else
    HEK                  *refcounted_he_hek;	/* hint key */
#endif
    union {
	IV                refcounted_he_u_iv;
	UV                refcounted_he_u_uv;
	STRLEN            refcounted_he_u_len;
	void		 *refcounted_he_u_ptr;	/* Might be useful in future */
    } refcounted_he_val;
    U32	                  refcounted_he_refcnt;	/* reference count */
    /* First byte is flags. Then NUL-terminated value. Then for ithreads,
       non-NUL terminated key.  */
    char                  refcounted_he_data[1];
};

/*
=for apidoc m|SV *|refcounted_he_fetch_pvs|const struct refcounted_he *chain|const char *key|U32 flags

Like L</refcounted_he_fetch_pvn>, but takes a literal string instead of
a string/length pair, and no precomputed hash.

=cut
*/

#define refcounted_he_fetch_pvs(chain, key, flags) \
    Perl_refcounted_he_fetch_pvn(aTHX_ chain, STR_WITH_LEN(key), 0, flags)

/*
=for apidoc m|struct refcounted_he *|refcounted_he_new_pvs|struct refcounted_he *parent|const char *key|SV *value|U32 flags

Like L</refcounted_he_new_pvn>, but takes a literal string instead of
a string/length pair, and no precomputed hash.

=cut
*/

#define refcounted_he_new_pvs(parent, key, value, flags) \
    Perl_refcounted_he_new_pvn(aTHX_ parent, STR_WITH_LEN(key), 0, value, flags)

/* Flag bits are HVhek_UTF8, HVhek_WASUTF8, then */
#define HVrhek_undef	0x00 /* Value is undef. */
#define HVrhek_delete	0x10 /* Value is placeholder - signifies delete. */
#define HVrhek_IV	0x20 /* Value is IV. */
#define HVrhek_UV	0x30 /* Value is UV. */
#define HVrhek_PV	0x40 /* Value is a (byte) string. */
#define HVrhek_PV_UTF8	0x50 /* Value is a (utf8) string. */
/* Two spare. As these have to live in the optree, you can't store anything
   interpreter specific, such as SVs. :-( */
#define HVrhek_typemask 0x70

#ifdef USE_ITHREADS
/* A big expression to find the key offset */
#define REF_HE_KEY(chain)						\
	((((chain->refcounted_he_data[0] & 0x60) == 0x40)		\
	    ? chain->refcounted_he_val.refcounted_he_u_len + 1 : 0)	\
	 + 1 + chain->refcounted_he_data)
#endif

#  ifdef USE_ITHREADS
#    define HINTS_REFCNT_LOCK		MUTEX_LOCK(&PL_hints_mutex)
#    define HINTS_REFCNT_UNLOCK		MUTEX_UNLOCK(&PL_hints_mutex)
#  else
#    define HINTS_REFCNT_LOCK		NOOP
#    define HINTS_REFCNT_UNLOCK		NOOP
#  endif
#endif

#ifdef USE_ITHREADS
#  define HINTS_REFCNT_INIT		MUTEX_INIT(&PL_hints_mutex)
#  define HINTS_REFCNT_TERM		MUTEX_DESTROY(&PL_hints_mutex)
#else
#  define HINTS_REFCNT_INIT		NOOP
#  define HINTS_REFCNT_TERM		NOOP
#endif

/* Hash actions
 * Passed in PERL_MAGIC_uvar calls
 */
#define HV_DISABLE_UVAR_XKEY	0x01
/* We need to ensure that these don't clash with G_DISCARD, which is 2, as it
   is documented as being passed to hv_delete().  */
#define HV_FETCH_ISSTORE	0x04
#define HV_FETCH_ISEXISTS	0x08
#define HV_FETCH_LVALUE		0x10
#define HV_FETCH_JUST_SV	0x20
#define HV_DELETE		0x40
#define HV_FETCH_EMPTY_HE	0x80 /* Leave HeVAL null. */

/* Must not conflict with HVhek_UTF8 */
#define HV_NAME_SETALL		0x02

/*
=for apidoc newHV

Creates a new HV.  The reference count is set to 1.

=cut
*/

#define newHV()	MUTABLE_HV(newSV_type(SVt_PVHV))

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * ex: set ts=8 sts=4 sw=4 et:
 */
