/******************************************************************************
 * Copyright © 2014-2019 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/
#include "komodo_defs.h"
#include "hex.h"
#include "key_io.h"
#include "cc/CCinclude.h"
#include <string.h>
#include "crc.h"
#include "komodo_config.h"

#ifdef _WIN32
#include <sodium.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#endif

#define SATOSHIDEN ((uint64_t)100000000L)
#define dstr(x) ((double)(x) / SATOSHIDEN)
#define portable_mutex_t pthread_mutex_t
#define portable_mutex_init(ptr) pthread_mutex_init(ptr,NULL)
#define portable_mutex_lock pthread_mutex_lock
#define portable_mutex_unlock pthread_mutex_unlock

extern void verus_hash(void *result, const void *data, size_t len);
int32_t komodo_baseid(char *origbase);

struct allocitem { uint32_t allocsize,type; };
struct queueitem { struct queueitem *next,*prev; uint32_t allocsize,type;  };

typedef struct queue
{
	struct queueitem *list;
	pthread_mutex_t mutex;
    char name[64],initflag;
} queue_t;

#include "mini-gmp.c"

#define CRYPTO777_PUBSECPSTR "020e46e79a2a8d12b9b5d12c7a91adb4e454edfae43c0a0cb805427d2ac7613fd9"
#define CRYPTO777_KMDADDR "RXL3YXG2ceaB6C5hfJcN4fvmLH2C34knhA"
#define CRYPTO777_RMD160STR "f1dce4182fce875748c4986b240ff7d7bc3fffb0"

#define KOMODO_PUBTYPE 60

struct sha256_vstate { uint64_t length; uint32_t state[8],curlen; uint8_t buf[64]; };
struct rmd160_vstate { uint64_t length; uint8_t buf[64]; uint32_t curlen, state[5]; };

// following is ported from libtom

#define STORE32L(x, y)                                                                     \
{ (y)[3] = (uint8_t)(((x)>>24)&255); (y)[2] = (uint8_t)(((x)>>16)&255);   \
(y)[1] = (uint8_t)(((x)>>8)&255); (y)[0] = (uint8_t)((x)&255); }

#define LOAD32L(x, y)                            \
{ x = (uint32_t)(((uint64_t)((y)[3] & 255)<<24) | \
((uint32_t)((y)[2] & 255)<<16) | \
((uint32_t)((y)[1] & 255)<<8)  | \
((uint32_t)((y)[0] & 255))); }

#define STORE64L(x, y)                                                                     \
{ (y)[7] = (uint8_t)(((x)>>56)&255); (y)[6] = (uint8_t)(((x)>>48)&255);   \
(y)[5] = (uint8_t)(((x)>>40)&255); (y)[4] = (uint8_t)(((x)>>32)&255);   \
(y)[3] = (uint8_t)(((x)>>24)&255); (y)[2] = (uint8_t)(((x)>>16)&255);   \
(y)[1] = (uint8_t)(((x)>>8)&255); (y)[0] = (uint8_t)((x)&255); }

#define LOAD64L(x, y)                                                       \
{ x = (((uint64_t)((y)[7] & 255))<<56)|(((uint64_t)((y)[6] & 255))<<48)| \
(((uint64_t)((y)[5] & 255))<<40)|(((uint64_t)((y)[4] & 255))<<32)| \
(((uint64_t)((y)[3] & 255))<<24)|(((uint64_t)((y)[2] & 255))<<16)| \
(((uint64_t)((y)[1] & 255))<<8)|(((uint64_t)((y)[0] & 255))); }

#define STORE32H(x, y)                                                                     \
{ (y)[0] = (uint8_t)(((x)>>24)&255); (y)[1] = (uint8_t)(((x)>>16)&255);   \
(y)[2] = (uint8_t)(((x)>>8)&255); (y)[3] = (uint8_t)((x)&255); }

#define LOAD32H(x, y)                            \
{ x = (uint32_t)(((uint64_t)((y)[0] & 255)<<24) | \
((uint32_t)((y)[1] & 255)<<16) | \
((uint32_t)((y)[2] & 255)<<8)  | \
((uint32_t)((y)[3] & 255))); }

#define STORE64H(x, y)                                                                     \
{ (y)[0] = (uint8_t)(((x)>>56)&255); (y)[1] = (uint8_t)(((x)>>48)&255);     \
(y)[2] = (uint8_t)(((x)>>40)&255); (y)[3] = (uint8_t)(((x)>>32)&255);     \
(y)[4] = (uint8_t)(((x)>>24)&255); (y)[5] = (uint8_t)(((x)>>16)&255);     \
(y)[6] = (uint8_t)(((x)>>8)&255); (y)[7] = (uint8_t)((x)&255); }

#define LOAD64H(x, y)                                                      \
{ x = (((uint64_t)((y)[0] & 255))<<56)|(((uint64_t)((y)[1] & 255))<<48) | \
(((uint64_t)((y)[2] & 255))<<40)|(((uint64_t)((y)[3] & 255))<<32) | \
(((uint64_t)((y)[4] & 255))<<24)|(((uint64_t)((y)[5] & 255))<<16) | \
(((uint64_t)((y)[6] & 255))<<8)|(((uint64_t)((y)[7] & 255))); }

// Various logical functions
#define RORc(x, y) ( ((((uint32_t)(x)&0xFFFFFFFFUL)>>(uint32_t)((y)&31)) | ((uint32_t)(x)<<(uint32_t)(32-((y)&31)))) & 0xFFFFFFFFUL)
#define Ch(x,y,z)       (z ^ (x & (y ^ z)))
#define Maj(x,y,z)      (((x | y) & z) | (x & y))
#define S(x, n)         RORc((x),(n))
#define R(x, n)         (((x)&0xFFFFFFFFUL)>>(n))
#define Sigma0(x)       (S(x, 2) ^ S(x, 13) ^ S(x, 22))
#define Sigma1(x)       (S(x, 6) ^ S(x, 11) ^ S(x, 25))
#define Gamma0(x)       (S(x, 7) ^ S(x, 18) ^ R(x, 3))
#define Gamma1(x)       (S(x, 17) ^ S(x, 19) ^ R(x, 10))
#define MIN(x, y) ( ((x)<(y))?(x):(y) )

static inline int32_t sha256_vcompress(struct sha256_vstate * md,uint8_t *buf)
{
    uint32_t S[8],W[64],t0,t1,i;
    for (i=0; i<8; i++) // copy state into S
        S[i] = md->state[i];
    for (i=0; i<16; i++) // copy the state into 512-bits into W[0..15]
        LOAD32H(W[i],buf + (4*i));
    for (i=16; i<64; i++) // fill W[16..63]
        W[i] = Gamma1(W[i - 2]) + W[i - 7] + Gamma0(W[i - 15]) + W[i - 16];

#define RND(a,b,c,d,e,f,g,h,i,ki)                    \
t0 = h + Sigma1(e) + Ch(e, f, g) + ki + W[i];   \
t1 = Sigma0(a) + Maj(a, b, c);                  \
d += t0;                                        \
h  = t0 + t1;

    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],0,0x428a2f98);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],1,0x71374491);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],2,0xb5c0fbcf);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],3,0xe9b5dba5);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],4,0x3956c25b);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],5,0x59f111f1);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],6,0x923f82a4);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],7,0xab1c5ed5);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],8,0xd807aa98);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],9,0x12835b01);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],10,0x243185be);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],11,0x550c7dc3);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],12,0x72be5d74);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],13,0x80deb1fe);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],14,0x9bdc06a7);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],15,0xc19bf174);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],16,0xe49b69c1);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],17,0xefbe4786);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],18,0x0fc19dc6);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],19,0x240ca1cc);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],20,0x2de92c6f);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],21,0x4a7484aa);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],22,0x5cb0a9dc);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],23,0x76f988da);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],24,0x983e5152);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],25,0xa831c66d);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],26,0xb00327c8);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],27,0xbf597fc7);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],28,0xc6e00bf3);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],29,0xd5a79147);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],30,0x06ca6351);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],31,0x14292967);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],32,0x27b70a85);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],33,0x2e1b2138);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],34,0x4d2c6dfc);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],35,0x53380d13);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],36,0x650a7354);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],37,0x766a0abb);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],38,0x81c2c92e);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],39,0x92722c85);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],40,0xa2bfe8a1);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],41,0xa81a664b);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],42,0xc24b8b70);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],43,0xc76c51a3);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],44,0xd192e819);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],45,0xd6990624);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],46,0xf40e3585);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],47,0x106aa070);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],48,0x19a4c116);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],49,0x1e376c08);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],50,0x2748774c);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],51,0x34b0bcb5);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],52,0x391c0cb3);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],53,0x4ed8aa4a);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],54,0x5b9cca4f);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],55,0x682e6ff3);
    RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],56,0x748f82ee);
    RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],57,0x78a5636f);
    RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],58,0x84c87814);
    RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],59,0x8cc70208);
    RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],60,0x90befffa);
    RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],61,0xa4506ceb);
    RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],62,0xbef9a3f7);
    RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],63,0xc67178f2);
#undef RND
    for (i=0; i<8; i++) // feedback
        md->state[i] = md->state[i] + S[i];
    return(0);
}

#undef RORc
#undef Ch
#undef Maj
#undef S
#undef R
#undef Sigma0
#undef Sigma1
#undef Gamma0
#undef Gamma1

static inline void sha256_vinit(struct sha256_vstate * md)
{
    md->curlen = 0;
    md->length = 0;
    md->state[0] = 0x6A09E667UL;
    md->state[1] = 0xBB67AE85UL;
    md->state[2] = 0x3C6EF372UL;
    md->state[3] = 0xA54FF53AUL;
    md->state[4] = 0x510E527FUL;
    md->state[5] = 0x9B05688CUL;
    md->state[6] = 0x1F83D9ABUL;
    md->state[7] = 0x5BE0CD19UL;
}

static inline int32_t sha256_vprocess(struct sha256_vstate *md,const uint8_t *in,uint64_t inlen)
{
    uint64_t n; int32_t err;
    if ( md->curlen > sizeof(md->buf) )
        return(-1);
    while ( inlen > 0 )
    {
        if ( md->curlen == 0 && inlen >= 64 )
        {
            if ( (err= sha256_vcompress(md,(uint8_t *)in)) != 0 )
                return(err);
            md->length += 64 * 8, in += 64, inlen -= 64;
        }
        else
        {
            n = MIN(inlen,64 - md->curlen);
            memcpy(md->buf + md->curlen,in,(size_t)n);
            md->curlen += n, in += n, inlen -= n;
            if ( md->curlen == 64 )
            {
                if ( (err= sha256_vcompress(md,md->buf)) != 0 )
                    return(err);
                md->length += 8*64;
                md->curlen = 0;
            }
        }
    }
    return(0);
}

static inline int32_t sha256_vdone(struct sha256_vstate *md,uint8_t *out)
{
    int32_t i;
    if ( md->curlen >= sizeof(md->buf) )
        return(-1);
    md->length += md->curlen * 8; // increase the length of the message
    md->buf[md->curlen++] = (uint8_t)0x80; // append the '1' bit
    // if len > 56 bytes we append zeros then compress.  Then we can fall back to padding zeros and length encoding like normal.
    if ( md->curlen > 56 )
    {
        while ( md->curlen < 64 )
            md->buf[md->curlen++] = (uint8_t)0;
        sha256_vcompress(md,md->buf);
        md->curlen = 0;
    }
    while ( md->curlen < 56 ) // pad upto 56 bytes of zeroes
        md->buf[md->curlen++] = (uint8_t)0;
    STORE64H(md->length,md->buf+56); // store length
    sha256_vcompress(md,md->buf);
    for (i=0; i<8; i++) // copy output
        STORE32H(md->state[i],out+(4*i));
    return(0);
}

void vcalc_sha256(char deprecated[(256 >> 3) * 2 + 1],uint8_t hash[256 >> 3],uint8_t *src,int32_t len)
{
    struct sha256_vstate md;
    sha256_vinit(&md);
    sha256_vprocess(&md,src,len);
    sha256_vdone(&md,hash);
}

bits256 bits256_doublesha256(char *deprecated,uint8_t *data,int32_t datalen)
{
    bits256 hash,hash2; int32_t i;
    vcalc_sha256(0,hash.bytes,data,datalen);
    vcalc_sha256(0,hash2.bytes,hash.bytes,sizeof(hash));
    for (i=0; i<sizeof(hash); i++)
        hash.bytes[i] = hash2.bytes[sizeof(hash) - 1 - i];
    return(hash);
}


// rmd160: the five basic functions F(), G() and H()
#define F(x, y, z)        ((x) ^ (y) ^ (z))
#define G(x, y, z)        (((x) & (y)) | (~(x) & (z)))
#define H(x, y, z)        (((x) | ~(y)) ^ (z))
#define I(x, y, z)        (((x) & (z)) | ((y) & ~(z)))
#define J(x, y, z)        ((x) ^ ((y) | ~(z)))
#define ROLc(x, y) ( (((unsigned long)(x)<<(unsigned long)((y)&31)) | (((unsigned long)(x)&0xFFFFFFFFUL)>>(unsigned long)(32-((y)&31)))) & 0xFFFFFFFFUL)

/* the ten basic operations FF() through III() */
#define FF(a, b, c, d, e, x, s)        \
(a) += F((b), (c), (d)) + (x);\
(a) = ROLc((a), (s)) + (e);\
(c) = ROLc((c), 10);

#define GG(a, b, c, d, e, x, s)        \
(a) += G((b), (c), (d)) + (x) + 0x5a827999UL;\
(a) = ROLc((a), (s)) + (e);\
(c) = ROLc((c), 10);

#define HH(a, b, c, d, e, x, s)        \
(a) += H((b), (c), (d)) + (x) + 0x6ed9eba1UL;\
(a) = ROLc((a), (s)) + (e);\
(c) = ROLc((c), 10);

#define II(a, b, c, d, e, x, s)        \
(a) += I((b), (c), (d)) + (x) + 0x8f1bbcdcUL;\
(a) = ROLc((a), (s)) + (e);\
(c) = ROLc((c), 10);

#define JJ(a, b, c, d, e, x, s)        \
(a) += J((b), (c), (d)) + (x) + 0xa953fd4eUL;\
(a) = ROLc((a), (s)) + (e);\
(c) = ROLc((c), 10);

#define FFF(a, b, c, d, e, x, s)        \
(a) += F((b), (c), (d)) + (x);\
(a) = ROLc((a), (s)) + (e);\
(c) = ROLc((c), 10);

#define GGG(a, b, c, d, e, x, s)        \
(a) += G((b), (c), (d)) + (x) + 0x7a6d76e9UL;\
(a) = ROLc((a), (s)) + (e);\
(c) = ROLc((c), 10);

#define HHH(a, b, c, d, e, x, s)        \
(a) += H((b), (c), (d)) + (x) + 0x6d703ef3UL;\
(a) = ROLc((a), (s)) + (e);\
(c) = ROLc((c), 10);

#define III(a, b, c, d, e, x, s)        \
(a) += I((b), (c), (d)) + (x) + 0x5c4dd124UL;\
(a) = ROLc((a), (s)) + (e);\
(c) = ROLc((c), 10);

#define JJJ(a, b, c, d, e, x, s)        \
(a) += J((b), (c), (d)) + (x) + 0x50a28be6UL;\
(a) = ROLc((a), (s)) + (e);\
(c) = ROLc((c), 10);

static int32_t rmd160_vcompress(struct rmd160_vstate *md,uint8_t *buf)
{
    uint32_t aa,bb,cc,dd,ee,aaa,bbb,ccc,ddd,eee,X[16];
    int i;

    /* load words X */
    for (i = 0; i < 16; i++){
        LOAD32L(X[i], buf + (4 * i));
    }

    /* load state */
    aa = aaa = md->state[0];
    bb = bbb = md->state[1];
    cc = ccc = md->state[2];
    dd = ddd = md->state[3];
    ee = eee = md->state[4];

    /* round 1 */
    FF(aa, bb, cc, dd, ee, X[ 0], 11);
    FF(ee, aa, bb, cc, dd, X[ 1], 14);
    FF(dd, ee, aa, bb, cc, X[ 2], 15);
    FF(cc, dd, ee, aa, bb, X[ 3], 12);
    FF(bb, cc, dd, ee, aa, X[ 4],  5);
    FF(aa, bb, cc, dd, ee, X[ 5],  8);
    FF(ee, aa, bb, cc, dd, X[ 6],  7);
    FF(dd, ee, aa, bb, cc, X[ 7],  9);
    FF(cc, dd, ee, aa, bb, X[ 8], 11);
    FF(bb, cc, dd, ee, aa, X[ 9], 13);
    FF(aa, bb, cc, dd, ee, X[10], 14);
    FF(ee, aa, bb, cc, dd, X[11], 15);
    FF(dd, ee, aa, bb, cc, X[12],  6);
    FF(cc, dd, ee, aa, bb, X[13],  7);
    FF(bb, cc, dd, ee, aa, X[14],  9);
    FF(aa, bb, cc, dd, ee, X[15],  8);

    /* round 2 */
    GG(ee, aa, bb, cc, dd, X[ 7],  7);
    GG(dd, ee, aa, bb, cc, X[ 4],  6);
    GG(cc, dd, ee, aa, bb, X[13],  8);
    GG(bb, cc, dd, ee, aa, X[ 1], 13);
    GG(aa, bb, cc, dd, ee, X[10], 11);
    GG(ee, aa, bb, cc, dd, X[ 6],  9);
    GG(dd, ee, aa, bb, cc, X[15],  7);
    GG(cc, dd, ee, aa, bb, X[ 3], 15);
    GG(bb, cc, dd, ee, aa, X[12],  7);
    GG(aa, bb, cc, dd, ee, X[ 0], 12);
    GG(ee, aa, bb, cc, dd, X[ 9], 15);
    GG(dd, ee, aa, bb, cc, X[ 5],  9);
    GG(cc, dd, ee, aa, bb, X[ 2], 11);
    GG(bb, cc, dd, ee, aa, X[14],  7);
    GG(aa, bb, cc, dd, ee, X[11], 13);
    GG(ee, aa, bb, cc, dd, X[ 8], 12);

    /* round 3 */
    HH(dd, ee, aa, bb, cc, X[ 3], 11);
    HH(cc, dd, ee, aa, bb, X[10], 13);
    HH(bb, cc, dd, ee, aa, X[14],  6);
    HH(aa, bb, cc, dd, ee, X[ 4],  7);
    HH(ee, aa, bb, cc, dd, X[ 9], 14);
    HH(dd, ee, aa, bb, cc, X[15],  9);
    HH(cc, dd, ee, aa, bb, X[ 8], 13);
    HH(bb, cc, dd, ee, aa, X[ 1], 15);
    HH(aa, bb, cc, dd, ee, X[ 2], 14);
    HH(ee, aa, bb, cc, dd, X[ 7],  8);
    HH(dd, ee, aa, bb, cc, X[ 0], 13);
    HH(cc, dd, ee, aa, bb, X[ 6],  6);
    HH(bb, cc, dd, ee, aa, X[13],  5);
    HH(aa, bb, cc, dd, ee, X[11], 12);
    HH(ee, aa, bb, cc, dd, X[ 5],  7);
    HH(dd, ee, aa, bb, cc, X[12],  5);

    /* round 4 */
    II(cc, dd, ee, aa, bb, X[ 1], 11);
    II(bb, cc, dd, ee, aa, X[ 9], 12);
    II(aa, bb, cc, dd, ee, X[11], 14);
    II(ee, aa, bb, cc, dd, X[10], 15);
    II(dd, ee, aa, bb, cc, X[ 0], 14);
    II(cc, dd, ee, aa, bb, X[ 8], 15);
    II(bb, cc, dd, ee, aa, X[12],  9);
    II(aa, bb, cc, dd, ee, X[ 4],  8);
    II(ee, aa, bb, cc, dd, X[13],  9);
    II(dd, ee, aa, bb, cc, X[ 3], 14);
    II(cc, dd, ee, aa, bb, X[ 7],  5);
    II(bb, cc, dd, ee, aa, X[15],  6);
    II(aa, bb, cc, dd, ee, X[14],  8);
    II(ee, aa, bb, cc, dd, X[ 5],  6);
    II(dd, ee, aa, bb, cc, X[ 6],  5);
    II(cc, dd, ee, aa, bb, X[ 2], 12);

    /* round 5 */
    JJ(bb, cc, dd, ee, aa, X[ 4],  9);
    JJ(aa, bb, cc, dd, ee, X[ 0], 15);
    JJ(ee, aa, bb, cc, dd, X[ 5],  5);
    JJ(dd, ee, aa, bb, cc, X[ 9], 11);
    JJ(cc, dd, ee, aa, bb, X[ 7],  6);
    JJ(bb, cc, dd, ee, aa, X[12],  8);
    JJ(aa, bb, cc, dd, ee, X[ 2], 13);
    JJ(ee, aa, bb, cc, dd, X[10], 12);
    JJ(dd, ee, aa, bb, cc, X[14],  5);
    JJ(cc, dd, ee, aa, bb, X[ 1], 12);
    JJ(bb, cc, dd, ee, aa, X[ 3], 13);
    JJ(aa, bb, cc, dd, ee, X[ 8], 14);
    JJ(ee, aa, bb, cc, dd, X[11], 11);
    JJ(dd, ee, aa, bb, cc, X[ 6],  8);
    JJ(cc, dd, ee, aa, bb, X[15],  5);
    JJ(bb, cc, dd, ee, aa, X[13],  6);

    /* parallel round 1 */
    JJJ(aaa, bbb, ccc, ddd, eee, X[ 5],  8);
    JJJ(eee, aaa, bbb, ccc, ddd, X[14],  9);
    JJJ(ddd, eee, aaa, bbb, ccc, X[ 7],  9);
    JJJ(ccc, ddd, eee, aaa, bbb, X[ 0], 11);
    JJJ(bbb, ccc, ddd, eee, aaa, X[ 9], 13);
    JJJ(aaa, bbb, ccc, ddd, eee, X[ 2], 15);
    JJJ(eee, aaa, bbb, ccc, ddd, X[11], 15);
    JJJ(ddd, eee, aaa, bbb, ccc, X[ 4],  5);
    JJJ(ccc, ddd, eee, aaa, bbb, X[13],  7);
    JJJ(bbb, ccc, ddd, eee, aaa, X[ 6],  7);
    JJJ(aaa, bbb, ccc, ddd, eee, X[15],  8);
    JJJ(eee, aaa, bbb, ccc, ddd, X[ 8], 11);
    JJJ(ddd, eee, aaa, bbb, ccc, X[ 1], 14);
    JJJ(ccc, ddd, eee, aaa, bbb, X[10], 14);
    JJJ(bbb, ccc, ddd, eee, aaa, X[ 3], 12);
    JJJ(aaa, bbb, ccc, ddd, eee, X[12],  6);

    /* parallel round 2 */
    III(eee, aaa, bbb, ccc, ddd, X[ 6],  9);
    III(ddd, eee, aaa, bbb, ccc, X[11], 13);
    III(ccc, ddd, eee, aaa, bbb, X[ 3], 15);
    III(bbb, ccc, ddd, eee, aaa, X[ 7],  7);
    III(aaa, bbb, ccc, ddd, eee, X[ 0], 12);
    III(eee, aaa, bbb, ccc, ddd, X[13],  8);
    III(ddd, eee, aaa, bbb, ccc, X[ 5],  9);
    III(ccc, ddd, eee, aaa, bbb, X[10], 11);
    III(bbb, ccc, ddd, eee, aaa, X[14],  7);
    III(aaa, bbb, ccc, ddd, eee, X[15],  7);
    III(eee, aaa, bbb, ccc, ddd, X[ 8], 12);
    III(ddd, eee, aaa, bbb, ccc, X[12],  7);
    III(ccc, ddd, eee, aaa, bbb, X[ 4],  6);
    III(bbb, ccc, ddd, eee, aaa, X[ 9], 15);
    III(aaa, bbb, ccc, ddd, eee, X[ 1], 13);
    III(eee, aaa, bbb, ccc, ddd, X[ 2], 11);

    /* parallel round 3 */
    HHH(ddd, eee, aaa, bbb, ccc, X[15],  9);
    HHH(ccc, ddd, eee, aaa, bbb, X[ 5],  7);
    HHH(bbb, ccc, ddd, eee, aaa, X[ 1], 15);
    HHH(aaa, bbb, ccc, ddd, eee, X[ 3], 11);
    HHH(eee, aaa, bbb, ccc, ddd, X[ 7],  8);
    HHH(ddd, eee, aaa, bbb, ccc, X[14],  6);
    HHH(ccc, ddd, eee, aaa, bbb, X[ 6],  6);
    HHH(bbb, ccc, ddd, eee, aaa, X[ 9], 14);
    HHH(aaa, bbb, ccc, ddd, eee, X[11], 12);
    HHH(eee, aaa, bbb, ccc, ddd, X[ 8], 13);
    HHH(ddd, eee, aaa, bbb, ccc, X[12],  5);
    HHH(ccc, ddd, eee, aaa, bbb, X[ 2], 14);
    HHH(bbb, ccc, ddd, eee, aaa, X[10], 13);
    HHH(aaa, bbb, ccc, ddd, eee, X[ 0], 13);
    HHH(eee, aaa, bbb, ccc, ddd, X[ 4],  7);
    HHH(ddd, eee, aaa, bbb, ccc, X[13],  5);

    /* parallel round 4 */
    GGG(ccc, ddd, eee, aaa, bbb, X[ 8], 15);
    GGG(bbb, ccc, ddd, eee, aaa, X[ 6],  5);
    GGG(aaa, bbb, ccc, ddd, eee, X[ 4],  8);
    GGG(eee, aaa, bbb, ccc, ddd, X[ 1], 11);
    GGG(ddd, eee, aaa, bbb, ccc, X[ 3], 14);
    GGG(ccc, ddd, eee, aaa, bbb, X[11], 14);
    GGG(bbb, ccc, ddd, eee, aaa, X[15],  6);
    GGG(aaa, bbb, ccc, ddd, eee, X[ 0], 14);
    GGG(eee, aaa, bbb, ccc, ddd, X[ 5],  6);
    GGG(ddd, eee, aaa, bbb, ccc, X[12],  9);
    GGG(ccc, ddd, eee, aaa, bbb, X[ 2], 12);
    GGG(bbb, ccc, ddd, eee, aaa, X[13],  9);
    GGG(aaa, bbb, ccc, ddd, eee, X[ 9], 12);
    GGG(eee, aaa, bbb, ccc, ddd, X[ 7],  5);
    GGG(ddd, eee, aaa, bbb, ccc, X[10], 15);
    GGG(ccc, ddd, eee, aaa, bbb, X[14],  8);

    /* parallel round 5 */
    FFF(bbb, ccc, ddd, eee, aaa, X[12] ,  8);
    FFF(aaa, bbb, ccc, ddd, eee, X[15] ,  5);
    FFF(eee, aaa, bbb, ccc, ddd, X[10] , 12);
    FFF(ddd, eee, aaa, bbb, ccc, X[ 4] ,  9);
    FFF(ccc, ddd, eee, aaa, bbb, X[ 1] , 12);
    FFF(bbb, ccc, ddd, eee, aaa, X[ 5] ,  5);
    FFF(aaa, bbb, ccc, ddd, eee, X[ 8] , 14);
    FFF(eee, aaa, bbb, ccc, ddd, X[ 7] ,  6);
    FFF(ddd, eee, aaa, bbb, ccc, X[ 6] ,  8);
    FFF(ccc, ddd, eee, aaa, bbb, X[ 2] , 13);
    FFF(bbb, ccc, ddd, eee, aaa, X[13] ,  6);
    FFF(aaa, bbb, ccc, ddd, eee, X[14] ,  5);
    FFF(eee, aaa, bbb, ccc, ddd, X[ 0] , 15);
    FFF(ddd, eee, aaa, bbb, ccc, X[ 3] , 13);
    FFF(ccc, ddd, eee, aaa, bbb, X[ 9] , 11);
    FFF(bbb, ccc, ddd, eee, aaa, X[11] , 11);

    /* combine results */
    ddd += cc + md->state[1];               /* final result for md->state[0] */
    md->state[1] = md->state[2] + dd + eee;
    md->state[2] = md->state[3] + ee + aaa;
    md->state[3] = md->state[4] + aa + bbb;
    md->state[4] = md->state[0] + bb + ccc;
    md->state[0] = ddd;

    return 0;
}

/**
 Initialize the hash state
 @param md   The hash state you wish to initialize
 @return 0 if successful
 */
int rmd160_vinit(struct rmd160_vstate * md)
{
    md->state[0] = 0x67452301UL;
    md->state[1] = 0xefcdab89UL;
    md->state[2] = 0x98badcfeUL;
    md->state[3] = 0x10325476UL;
    md->state[4] = 0xc3d2e1f0UL;
    md->curlen   = 0;
    md->length   = 0;
    return 0;
}
#define HASH_PROCESS(func_name, compress_name, state_var, block_size)                       \
int func_name (struct rmd160_vstate * md, const unsigned char *in, unsigned long inlen)               \
{                                                                                           \
unsigned long n;                                                                        \
int           err;                                                                      \
if (md->curlen > sizeof(md->buf)) {                             \
return -1;                                                            \
}                                                                                       \
while (inlen > 0) {                                                                     \
if (md->curlen == 0 && inlen >= block_size) {                           \
if ((err = compress_name (md, (unsigned char *)in)) != 0) {               \
return err;                                                                   \
}                                                                                \
md->length += block_size * 8;                                        \
in             += block_size;                                                    \
inlen          -= block_size;                                                    \
} else {                                                                            \
n = MIN(inlen, (block_size - md->curlen));                           \
memcpy(md->buf + md->curlen, in, (size_t)n);              \
md->curlen += n;                                                     \
in             += n;                                                             \
inlen          -= n;                                                             \
if (md->curlen == block_size) {                                      \
if ((err = compress_name (md, md->buf)) != 0) {            \
return err;                                                                \
}                                                                             \
md->length += 8*block_size;                                       \
md->curlen = 0;                                                   \
}                                                                                \
}                                                                                    \
}                                                                                       \
return 0;                                                                        \
}

/**
 Process a block of memory though the hash
 @param md     The hash state
 @param in     The data to hash
 @param inlen  The length of the data (octets)
 @return 0 if successful
 */
HASH_PROCESS(rmd160_vprocess, rmd160_vcompress, rmd160, 64)

/**
 Terminate the hash to get the digest
 @param md  The hash state
 @param out [out] The destination of the hash (20 bytes)
 @return 0 if successful
 */
int rmd160_vdone(struct rmd160_vstate * md, unsigned char *out)
{
    int i;
    if (md->curlen >= sizeof(md->buf)) {
        return -1;
    }
    /* increase the length of the message */
    md->length += md->curlen * 8;

    /* append the '1' bit */
    md->buf[md->curlen++] = (unsigned char)0x80;

    /* if the length is currently above 56 bytes we append zeros
     * then compress.  Then we can fall back to padding zeros and length
     * encoding like normal.
     */
    if (md->curlen > 56) {
        while (md->curlen < 64) {
            md->buf[md->curlen++] = (unsigned char)0;
        }
        rmd160_vcompress(md, md->buf);
        md->curlen = 0;
    }
    /* pad upto 56 bytes of zeroes */
    while (md->curlen < 56) {
        md->buf[md->curlen++] = (unsigned char)0;
    }
    /* store length */
    STORE64L(md->length, md->buf+56);
    rmd160_vcompress(md, md->buf);
    /* copy output */
    for (i = 0; i < 5; i++) {
        STORE32L(md->state[i], out+(4*i));
    }
    return 0;
}

void calc_rmd160(char deprecated[41],uint8_t buf[20],uint8_t *msg,int32_t len)
{
    struct rmd160_vstate md;
    rmd160_vinit(&md);
    rmd160_vprocess(&md,msg,len);
    rmd160_vdone(&md, buf);
}
#undef F
#undef G
#undef H
#undef I
#undef J
#undef ROLc
#undef FF
#undef GG
#undef HH
#undef II
#undef JJ
#undef FFF
#undef GGG
#undef HHH
#undef III
#undef JJJ

void calc_rmd160_sha256(uint8_t rmd160[20],uint8_t *data,int32_t datalen)
{
    bits256 hash;
    vcalc_sha256(0,hash.bytes,data,datalen);
    calc_rmd160(0,rmd160,hash.bytes,sizeof(hash));
}

int32_t bitcoin_addr2rmd160(uint8_t *addrtypep,uint8_t rmd160[20],char *coinaddr)
{
    bits256 hash; uint8_t *buf,_buf[25]; int32_t len;
    memset(rmd160,0,20);
    *addrtypep = 0;
    buf = _buf;
    if ( (len= bitcoin_base58decode(buf,coinaddr)) >= 4 )
    {
        // validate with trailing hash, then remove hash
        hash = bits256_doublesha256(0,buf,21);
        *addrtypep = *buf;
        memcpy(rmd160,buf+1,20);
        if ( (buf[21]&0xff) == hash.bytes[31] && (buf[22]&0xff) == hash.bytes[30] &&(buf[23]&0xff) == hash.bytes[29] && (buf[24]&0xff) == hash.bytes[28] )
        {
            //printf("coinaddr.(%s) valid checksum addrtype.%02x\n",coinaddr,*addrtypep);
            return(20);
        }
        else
        {
            int32_t i;
            if ( len > 20 )
            {
                hash = bits256_doublesha256(0,buf,len);
            }
            for (i=0; i<len; i++)
                printf("%02x ",buf[i]);
            printf("\nhex checkhash.(%s) len.%d mismatch %02x %02x %02x %02x vs %02x %02x %02x %02x\n",coinaddr,len,buf[len-1]&0xff,buf[len-2]&0xff,buf[len-3]&0xff,buf[len-4]&0xff,hash.bytes[31],hash.bytes[30],hash.bytes[29],hash.bytes[28]);
        }
    }
	return(0);
}

char *bitcoin_address(char *coinaddr,uint8_t addrtype,uint8_t *pubkey_or_rmd160,int32_t len)
{
    int32_t i; uint8_t data[25]; bits256 hash;// char checkaddr[65];
    if ( len != 20 )
        calc_rmd160_sha256(data+1,pubkey_or_rmd160,len);
    else memcpy(data+1,pubkey_or_rmd160,20);
    //btc_convrmd160(checkaddr,addrtype,data+1);
    data[0] = addrtype;
    hash = bits256_doublesha256(0,data,21);
    for (i=0; i<4; i++)
        data[21+i] = hash.bytes[31-i];
    if ( (coinaddr= bitcoin_base58encode(coinaddr,data,25)) != 0 )
    {
        //uint8_t checktype,rmd160[20];
        //bitcoin_addr2rmd160(&checktype,rmd160,coinaddr);
        //if ( strcmp(checkaddr,coinaddr) != 0 )
        //    printf("checkaddr.(%s) vs coinaddr.(%s) %02x vs [%02x] memcmp.%d\n",checkaddr,coinaddr,addrtype,checktype,memcmp(rmd160,data+1,20));
    }
    return(coinaddr);
}

int32_t komodo_is_issuer()
{
    if ( ASSETCHAINS_SYMBOL[0] != 0 && komodo_baseid(ASSETCHAINS_SYMBOL) >= 0 )
        return(1);
    else return(0);
}

int32_t bitweight(uint64_t x)
{
    int i,wt = 0;
    for (i=0; i<64; i++)
        if ( (1LL << i) & x )
            wt++;
    return(wt);
}

int32_t iguana_rwnum(int32_t rwflag,uint8_t *serialized,int32_t len,void *endianedp)
{
    int32_t i; uint64_t x;
    if ( rwflag == 0 )
    {
        x = 0;
        for (i=len-1; i>=0; i--)
        {
            x <<= 8;
            x |= serialized[i];
        }
        switch ( len )
        {
            case 1: *(uint8_t *)endianedp = (uint8_t)x; break;
            case 2: *(uint16_t *)endianedp = (uint16_t)x; break;
            case 4: *(uint32_t *)endianedp = (uint32_t)x; break;
            case 8: *(uint64_t *)endianedp = (uint64_t)x; break;
        }
    }
    else
    {
        x = 0;
        switch ( len )
        {
            case 1: x = *(uint8_t *)endianedp; break;
            case 2: x = *(uint16_t *)endianedp; break;
            case 4: x = *(uint32_t *)endianedp; break;
            case 8: x = *(uint64_t *)endianedp; break;
        }
        for (i=0; i<len; i++,x >>= 8)
            serialized[i] = (uint8_t)(x & 0xff);
    }
    return(len);
}

int32_t iguana_rwbignum(int32_t rwflag,uint8_t *serialized,int32_t len,uint8_t *endianedp)
{
    int32_t i;
    if ( rwflag == 0 )
    {
        for (i=0; i<len; i++)
            endianedp[i] = serialized[i];
    }
    else
    {
        for (i=0; i<len; i++)
            serialized[i] = endianedp[i];
    }
    return(len);
}

int32_t komodo_scriptitemlen(int32_t *opretlenp,uint8_t *script)
{
    int32_t opretlen,len = 0;
    if ( (opretlen= script[len++]) >= 0x4c )
    {
        if ( opretlen == 0x4c )
            opretlen = script[len++];
        else if ( opretlen == 0x4d )
        {
            opretlen = script[len] + (script[len+1] << 8);
            len += 2;
            //opretlen = script[len++];
            //opretlen = (opretlen << 8) | script[len++];
        }
    }
    *opretlenp = opretlen;
    return(len);
}

int32_t komodo_opreturnscript(uint8_t *script,uint8_t type,uint8_t *opret,int32_t opretlen)
{
    int32_t offset = 0;
    script[offset++] = 0x6a;
    opretlen++;
    if ( opretlen >= 0x4c )
    {
        if ( opretlen > 0xff )
        {
            script[offset++] = 0x4d;
            script[offset++] = opretlen & 0xff;
            script[offset++] = (opretlen >> 8) & 0xff;
        }
        else
        {
            script[offset++] = 0x4c;
            script[offset++] = opretlen;
        }
    } else script[offset++] = opretlen;
    script[offset++] = type; // covered by opretlen
    memcpy(&script[offset],opret,opretlen-1);
    return(offset + opretlen - 1);
}

// get a pseudo random number that is the same for each block individually at all times and different
// from all other blocks. the sequence is extremely likely, but not guaranteed to be unique for each block chain
uint64_t komodo_block_prg(uint32_t nHeight)
{
    if (strcmp(ASSETCHAINS_SYMBOL, "VRSC") != 0 || nHeight >= 12800)
    {
        uint64_t i, result = 0, hashSrc64 = ((uint64_t)ASSETCHAINS_MAGIC << 32) | (uint64_t)nHeight;
        uint8_t hashSrc[8];
        bits256 hashResult;

        for ( i = 0; i < sizeof(hashSrc); i++ )
        {
            uint64_t x = hashSrc64 >> (i * 8);
            hashSrc[i] = (uint8_t)(x & 0xff);
        }
        verus_hash(hashResult.bytes, hashSrc, sizeof(hashSrc));
        for ( i = 0; i < 8; i++ )
        {
            result = (result << 8) | hashResult.bytes[i];
        }
        return result;
    }
    else
    {
        int i;
        uint8_t hashSrc[8];
        uint64_t result=0, hashSrc64 = (uint64_t)ASSETCHAINS_MAGIC << 32 + nHeight;
        bits256 hashResult;

        for ( i = 0; i < sizeof(hashSrc); i++ )
        {
            hashSrc[i] = hashSrc64 & 0xff;
            hashSrc64 >>= 8;
            int8_t b = hashSrc[i];
        }

        vcalc_sha256(0, hashResult.bytes, hashSrc, sizeof(hashSrc));
        for ( i = 0; i < 8; i++ )
        {
            result = (result << 8) + hashResult.bytes[i];
        }
        return result;
    }
}

// given a block height, this returns the unlock time for that block height, derived from
// the ASSETCHAINS_MAGIC number as well as the block height, providing different random numbers
// for corresponding blocks across chains, but the same sequence in each chain
int64_t komodo_block_unlocktime(uint32_t nHeight)
{
    uint64_t fromTime, toTime, unlocktime;

    if ( ASSETCHAINS_TIMEUNLOCKFROM == ASSETCHAINS_TIMEUNLOCKTO )
        unlocktime = ASSETCHAINS_TIMEUNLOCKTO;
    else
    {
        if (strcmp(ASSETCHAINS_SYMBOL, "VRSC") != 0 || nHeight >= 12800)
        {
            unlocktime = komodo_block_prg(nHeight) % (ASSETCHAINS_TIMEUNLOCKTO - ASSETCHAINS_TIMEUNLOCKFROM);
            unlocktime += ASSETCHAINS_TIMEUNLOCKFROM;
        }
        else
        {
            unlocktime = komodo_block_prg(nHeight) / (0xffffffffffffffff / ((ASSETCHAINS_TIMEUNLOCKTO - ASSETCHAINS_TIMEUNLOCKFROM) + 1));
            // boundary and power of 2 can make it exceed to time by 1
            unlocktime = unlocktime + ASSETCHAINS_TIMEUNLOCKFROM;
            if (unlocktime > ASSETCHAINS_TIMEUNLOCKTO)
                unlocktime--;
        }
    }
    return ((int64_t)unlocktime);
}

long _stripwhite(char *buf,int accept)
{
    int32_t i,j,c;
    if ( buf == 0 || buf[0] == 0 )
        return(0);
    for (i=j=0; buf[i]!=0; i++)
    {
        buf[j] = c = buf[i];
        if ( c == accept || (c != ' ' && c != '\n' && c != '\r' && c != '\t' && c != '\b') )
            j++;
    }
    buf[j] = 0;
    return(j);
}

char *clonestr(char *str)
{
    char *clone;
    if ( str == 0 || str[0] == 0 )
    {
        printf("warning cloning nullstr.%p\n",str);
#ifdef __APPLE__
        while ( 1 ) sleep(1);
#endif
        str = (char *)"<nullstr>";
    }
    clone = (char *)malloc(strlen(str)+16);
    strcpy(clone,str);
    return(clone);
}

int32_t safecopy(char *dest, const char *src,long len)
{
    int32_t i = -1;
    if ( src != 0 && dest != 0 && src != dest )
    {
        if ( dest != 0 )
            memset(dest,0,len);
        for (i=0; i<len&&src[i]!=0; i++)
            dest[i] = src[i];
        if ( i == len )
        {
            printf("safecopy: %s too long %ld\n",src,len);
#ifdef __APPLE__
            //getchar();
#endif
            return(-1);
        }
        dest[i] = 0;
    }
    return(i);
}

char *parse_conf_line(char *line,char *field)
{
    line += strlen(field);
    for (; *line!='='&&*line!=0; line++)
        break;
    if ( *line == 0 )
        return(0);
    if ( *line == '=' )
        line++;
    while ( line[strlen(line)-1] == '\r' || line[strlen(line)-1] == '\n' || line[strlen(line)-1] == ' ' )
        line[strlen(line)-1] = 0;
    //printf("LINE.(%s)\n",line);
    _stripwhite(line,0);
    return(clonestr(line));
}

double OS_milliseconds()
{
    struct timeval tv; double millis;
    gettimeofday(&tv,NULL);
    millis = ((double)tv.tv_sec * 1000. + (double)tv.tv_usec / 1000.);
    //printf("tv_sec.%ld usec.%d %f\n",tv.tv_sec,tv.tv_usec,millis);
    return(millis);
}

#ifndef _WIN32
void OS_randombytes(unsigned char *x,long xlen)
{
    static int fd = -1;
    int32_t i;
    if (fd == -1) {
        for (;;) {
            fd = open("/dev/urandom",O_RDONLY);
            if (fd != -1) break;
            sleep(1);
        }
    }
    while (xlen > 0) {
        if (xlen < 1048576) i = (int32_t)xlen; else i = 1048576;
        i = (int32_t)read(fd,x,i);
        if (i < 1) {
            sleep(1);
            continue;
        }
        if ( 0 )
        {
            int32_t j;
            for (j=0; j<i; j++)
                printf("%02x ",x[j]);
            printf("-> %p\n",x);
        }
        x += i;
        xlen -= i;
    }
}
#endif

void lock_queue(queue_t *queue)
{
    if ( queue->initflag == 0 )
    {
        portable_mutex_init(&queue->mutex);
        queue->initflag = 1;
    }
	portable_mutex_lock(&queue->mutex);
}

void queue_enqueue(char *name,queue_t *queue,struct queueitem *item)
{
    if ( queue->name[0] == 0 && name != 0 && name[0] != 0 )
        strcpy(queue->name,name);
    if ( item == 0 )
    {
        printf("FATAL type error: queueing empty value\n");
        return;
    }
    lock_queue(queue);
    DL_APPEND(queue->list,item);
    portable_mutex_unlock(&queue->mutex);
}

struct queueitem *queue_dequeue(queue_t *queue)
{
    struct queueitem *item = 0;
    lock_queue(queue);
    if ( queue->list != 0 )
    {
        item = queue->list;
        DL_DELETE(queue->list,item);
    }
	portable_mutex_unlock(&queue->mutex);
    return(item);
}

void *queue_delete(queue_t *queue,struct queueitem *copy,int32_t copysize)
{
    struct queueitem *item = 0;
    lock_queue(queue);
    if ( queue->list != 0 )
    {
        DL_FOREACH(queue->list,item)
        {
						#ifdef _WIN32
						if ( item == copy || (item->allocsize == copysize && memcmp((void *)((intptr_t)item + sizeof(struct queueitem)),(void *)((intptr_t)copy + sizeof(struct queueitem)),copysize) == 0) )
						#else
            if ( item == copy || (item->allocsize == copysize && memcmp((void *)((long)item + sizeof(struct queueitem)),(void *)((long)copy + sizeof(struct queueitem)),copysize) == 0) )
						#endif
            {
                DL_DELETE(queue->list,item);
                portable_mutex_unlock(&queue->mutex);
                printf("name.(%s) deleted item.%p list.%p\n",queue->name,item,queue->list);
                return(item);
            }
        }
    }
	portable_mutex_unlock(&queue->mutex);
    return(0);
}

void *queue_free(queue_t *queue)
{
    struct queueitem *item = 0;
    lock_queue(queue);
    if ( queue->list != 0 )
    {
        DL_FOREACH(queue->list,item)
        {
            DL_DELETE(queue->list,item);
            free(item);
        }
        //printf("name.(%s) dequeue.%p list.%p\n",queue->name,item,queue->list);
    }
	portable_mutex_unlock(&queue->mutex);
    return(0);
}

void *queue_clone(queue_t *clone,queue_t *queue,int32_t size)
{
    struct queueitem *ptr,*item = 0;
    lock_queue(queue);
    if ( queue->list != 0 )
    {
        DL_FOREACH(queue->list,item)
        {
            ptr = (struct queueitem *)calloc(1,sizeof(*ptr));
            memcpy(ptr,item,size);
            queue_enqueue(queue->name,clone,ptr);
        }
        //printf("name.(%s) dequeue.%p list.%p\n",queue->name,item,queue->list);
    }
	portable_mutex_unlock(&queue->mutex);
    return(0);
}

int32_t queue_size(queue_t *queue)
{
    int32_t count = 0;
    struct queueitem *tmp;
    lock_queue(queue);
    DL_COUNT(queue->list,tmp,count);
    portable_mutex_unlock(&queue->mutex);
	return count;
}

void iguana_initQ(queue_t *Q,char *name)
{
    struct queueitem *item,*I;
    memset(Q,0,sizeof(*Q));
    I = (struct queueitem *)calloc(1,sizeof(*I));
    strcpy(Q->name,name);
    queue_enqueue(name,Q,I);
    if ( (item= queue_dequeue(Q)) != 0 )
        free(item);
}

/*****
 * @brief retrieve the full path of the file
 * @param symbol the symbol that forms part of the path
 * @param filename the file
 * @returns the full path of the file
 */
std::string komodo_statefname(const std::string& symbol, const std::string& filename)
{
    std::string fname = GetDataDir(false).string();
    
    size_t n;
    if ( (n= (int32_t)strlen(ASSETCHAINS_SYMBOL)) != 0 )
    {
        size_t len = fname.size();
        if ( !mapArgs.count("-datadir") && strcmp(ASSETCHAINS_SYMBOL,&fname[len - n]) == 0 )
            fname[len - n] = 0;
        else if(mapArgs.count("-datadir")) 
            printf("DEBUG - komodo_utils:1363: custom datadir\n");
        else
        {
            if ( symbol != "REGTEST" )
                printf("unexpected fname.(%s) vs %s [%s] n.%d len.%d (%s)\n",
                        fname.c_str(),symbol.c_str(),ASSETCHAINS_SYMBOL,(int)n,(int)len,&fname[len - n]);
            return fname;
        }
    }
    else
    {
        fname += boost::filesystem::path::preferred_separator;
    }

    if ( !symbol.empty() && "KMD" != symbol )
    {
        if(!mapArgs.count("-datadir")) 
            fname += symbol;
        fname += boost::filesystem::path::preferred_separator;
    }

    fname += filename;
    return fname;
}

/******
 * @brief read configuration file for username, password, and port
 * @param[out] userpass the username:password
 * @param[in] symbol the chain symbol
 * @returns the port or 0 if not found
 */
uint16_t komodo_userpass(char *userpass, const std::string& symbol)
{
    uint16_t port = 0;
    try
    {
        ConfigFile config_f(GetConfigFile(symbol));
        port = std::stoi(config_f.Value("rpcport"));
        strcpy(userpass, (config_f.Value("rpcuser") + ":" + config_f.Value("rpcpassword")).c_str());
    }
    catch(const ios_base::failure& ex)
    {
        LogPrintf("Unable to open config file: %s\n", ex.what() );
    }
    return port;
}

uint32_t komodo_assetmagic(char *symbol,uint64_t supply,uint8_t *extraptr,int32_t extralen)
{
    uint8_t buf[512]; uint32_t crc0=0; int32_t len = 0; bits256 hash;
    if ( strcmp(symbol,"KMD") == 0 )
        return(0x8de4eef9);
    len = iguana_rwnum(1,&buf[len],sizeof(supply),(void *)&supply);
    strcpy((char *)&buf[len],symbol);
    len += strlen(symbol);
    if ( extraptr != 0 && extralen != 0 )
    {
        vcalc_sha256(0,hash.bytes,extraptr,extralen);
        crc0 = hash.uints[0];
        int32_t i; for (i=0; i<extralen; i++)
            fprintf(stderr,"%02x",extraptr[i]);
        fprintf(stderr," extralen.%d crc0.%x\n",extralen,crc0);
    }
    return(calc_crc32(crc0,buf,len));
}

uint16_t komodo_assetport(uint32_t magic,int32_t extralen)
{
    if ( magic == 0x8de4eef9 )
        return(7770);
    else if ( extralen == 0 )
        return(8000 + (magic % 7777));
    else return(16000 + (magic % 49500));
}

uint16_t komodo_port(char *symbol,uint64_t supply,uint32_t *magicp,uint8_t *extraptr,int32_t extralen)
{
    if ( symbol == 0 || symbol[0] == 0 || strcmp("KMD",symbol) == 0 )
    {
        *magicp = 0x8de4eef9;
        return(7770);
    }
    *magicp = komodo_assetmagic(symbol,supply,extraptr,extralen);
    return(komodo_assetport(*magicp,extralen));
}

/*void komodo_ports(uint16_t ports[MAX_CURRENCIES])
{
    int32_t i; uint32_t magic;
    for (i=0; i<MAX_CURRENCIES; i++)
    {
        ports[i] = komodo_port(CURRENCIES[i],10,&magic);
        printf("%u ",ports[i]);
    }
    printf("ports\n");
}*/

char *iguanafmtstr = (char *)"curl --url \"http://127.0.0.1:7776\" --data \"{\\\"conf\\\":\\\"%s.conf\\\",\\\"path\\\":\\\"${HOME#\"/\"}/.komodo/%s\\\",\\\"unitval\\\":\\\"20\\\",\\\"zcash\\\":1,\\\"RELAY\\\":-1,\\\"VALIDATE\\\":0,\\\"prefetchlag\\\":-1,\\\"poll\\\":100,\\\"active\\\":1,\\\"agent\\\":\\\"iguana\\\",\\\"method\\\":\\\"addcoin\\\",\\\"startpend\\\":4,\\\"endpend\\\":4,\\\"services\\\":129,\\\"maxpeers\\\":8,\\\"newcoin\\\":\\\"%s\\\",\\\"name\\\":\\\"%s\\\",\\\"hasheaders\\\":1,\\\"useaddmultisig\\\":0,\\\"netmagic\\\":\\\"%s\\\",\\\"p2p\\\":%u,\\\"rpc\\\":%u,\\\"pubval\\\":60,\\\"p2shval\\\":85,\\\"wifval\\\":188,\\\"txfee_satoshis\\\":\\\"10000\\\",\\\"isPoS\\\":0,\\\"minoutput\\\":10000,\\\"minconfirms\\\":2,\\\"genesishash\\\":\\\"027e3758c3a65b12aa1046462b486d0a63bfa1beae327897f56c5cfb7daaae71\\\",\\\"protover\\\":170002,\\\"genesisblock\\\":\\\"0100000000000000000000000000000000000000000000000000000000000000000000003ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a000000000000000000000000000000000000000000000000000000000000000029ab5f490f0f0f200b00000000000000000000000000000000000000000000000000000000000000fd4005000d5ba7cda5d473947263bf194285317179d2b0d307119c2e7cc4bd8ac456f0774bd52b0cd9249be9d40718b6397a4c7bbd8f2b3272fed2823cd2af4bd1632200ba4bf796727d6347b225f670f292343274cc35099466f5fb5f0cd1c105121b28213d15db2ed7bdba490b4cedc69742a57b7c25af24485e523aadbb77a0144fc76f79ef73bd8530d42b9f3b9bed1c135ad1fe152923fafe98f95f76f1615e64c4abb1137f4c31b218ba2782bc15534788dda2cc08a0ee2987c8b27ff41bd4e31cd5fb5643dfe862c9a02ca9f90c8c51a6671d681d04ad47e4b53b1518d4befafefe8cadfb912f3d03051b1efbf1dfe37b56e93a741d8dfd80d576ca250bee55fab1311fc7b3255977558cdda6f7d6f875306e43a14413facdaed2f46093e0ef1e8f8a963e1632dcbeebd8e49fd16b57d49b08f9762de89157c65233f60c8e38a1f503a48c555f8ec45dedecd574a37601323c27be597b956343107f8bd80f3a925afaf30811df83c402116bb9c1e5231c70fff899a7c82f73c902ba54da53cc459b7bf1113db65cc8f6914d3618560ea69abd13658fa7b6af92d374d6eca9529f8bd565166e4fcbf2a8dfb3c9b69539d4d2ee2e9321b85b331925df195915f2757637c2805e1d4131e1ad9ef9bc1bb1c732d8dba4738716d351ab30c996c8657bab39567ee3b29c6d054b711495c0d52e1cd5d8e55b4f0f0325b97369280755b46a02afd54be4ddd9f77c22272b8bbb17ff5118fedbae2564524e797bd28b5f74f7079d532ccc059807989f94d267f47e724b3f1ecfe00ec9e6541c961080d8891251b84b4480bc292f6a180bea089fef5bbda56e1e41390d7c0e85ba0ef530f7177413481a226465a36ef6afe1e2bca69d2078712b3912bba1a99b1fbff0d355d6ffe726d2bb6fbc103c4ac5756e5bee6e47e17424ebcbf1b63d8cb90ce2e40198b4f4198689daea254307e52a25562f4c1455340f0ffeb10f9d8e914775e37d0edca019fb1b9c6ef81255ed86bc51c5391e0591480f66e2d88c5f4fd7277697968656a9b113ab97f874fdd5f2465e5559533e01ba13ef4a8f7a21d02c30c8ded68e8c54603ab9c8084ef6d9eb4e92c75b078539e2ae786ebab6dab73a09e0aa9ac575bcefb29e930ae656e58bcb513f7e3c17e079dce4f05b5dbc18c2a872b22509740ebe6a3903e00ad1abc55076441862643f93606e3dc35e8d9f2caef3ee6be14d513b2e062b21d0061de3bd56881713a1a5c17f5ace05e1ec09da53f99442df175a49bd154aa96e4949decd52fed79ccf7ccbce32941419c314e374e4a396ac553e17b5340336a1a25c22f9e42a243ba5404450b650acfc826a6e432971ace776e15719515e1634ceb9a4a35061b668c74998d3dfb5827f6238ec015377e6f9c94f38108768cf6e5c8b132e0303fb5a200368f845ad9d46343035a6ff94031df8d8309415bb3f6cd5ede9c135fdabcc030599858d803c0f85be7661c88984d88faa3d26fb0e9aac0056a53f1b5d0baed713c853c4a2726869a0a124a8a5bbc0fc0ef80c8ae4cb53636aa02503b86a1eb9836fcc259823e2692d921d88e1ffc1e6cb2bde43939ceb3f32a611686f539f8f7c9f0bf00381f743607d40960f06d347d1cd8ac8a51969c25e37150efdf7aa4c2037a2fd0516fb444525ab157a0ed0a7412b2fa69b217fe397263153782c0f64351fbdf2678fa0dc8569912dcd8e3ccad38f34f23bbbce14c6a26ac24911b308b82c7e43062d180baeac4ba7153858365c72c63dcf5f6a5b08070b730adb017aeae925b7d0439979e2679f45ed2f25a7edcfd2fb77a8794630285ccb0a071f5cce410b46dbf9750b0354aae8b65574501cc69efb5b6a43444074fee116641bb29da56c2b4a7f456991fc92b2\\\",\\\"debug\\\":0,\\\"seedipaddr\\\":\\\"%s\\\",\\\"sapling\\\":1,\\\"notarypay\\\":%i}\"";



int32_t komodo_whoami(char *pubkeystr,int32_t height,uint32_t timestamp)
{
    int32_t i,notaryid;
    for (i=0; i<33; i++)
        sprintf(&pubkeystr[i<<1],"%02x",NOTARY_PUBKEY33[i]);
    pubkeystr[66] = 0;
    komodo_chosennotary(&notaryid,height,NOTARY_PUBKEY33,timestamp);
    return(notaryid);
}

char *argv0suffix[] =
{
    (char *)"mnzd", (char *)"mnz-cli", (char *)"mnzd.exe", (char *)"mnz-cli.exe", (char *)"btchd", (char *)"btch-cli", (char *)"btchd.exe", (char *)"btch-cli.exe"
};

char *argv0names[] =
{
    (char *)"MNZ", (char *)"MNZ", (char *)"MNZ", (char *)"MNZ", (char *)"BTCH", (char *)"BTCH", (char *)"BTCH", (char *)"BTCH"
};

uint64_t komodo_max_money()
{
    return komodo_current_supply(10000000);
}

uint64_t komodo_ac_block_subsidy(int nHeight)
{
    // we have to find our era, start from beginning reward, and determine current subsidy
    int64_t numerator, denominator, subsidy = 0;
    int64_t subsidyDifference;
    int32_t numhalvings, curEra = 0, sign = 1;
    static uint64_t cached_subsidy; static int32_t cached_numhalvings; static int cached_era;

    // check for backwards compat, older chains with no explicit rewards had 0.0001 block reward
    if ( ASSETCHAINS_ENDSUBSIDY[0] == 0 && ASSETCHAINS_REWARD[0] == 0 )
        subsidy = 10000;
    else if ( (ASSETCHAINS_ENDSUBSIDY[0] == 0 && ASSETCHAINS_REWARD[0] != 0) || ASSETCHAINS_ENDSUBSIDY[0] != 0 )
    {
        // if we have an end block in the first era, find our current era
        if ( ASSETCHAINS_ENDSUBSIDY[0] != 0 )
        {
            for ( curEra = 0; curEra <= ASSETCHAINS_LASTERA; curEra++ )
            {
                if ( ASSETCHAINS_ENDSUBSIDY[curEra] > nHeight || ASSETCHAINS_ENDSUBSIDY[curEra] == 0 )
                    break;
            }
        }
        if ( curEra <= ASSETCHAINS_LASTERA )
        {
            int64_t nStart = curEra ? ASSETCHAINS_ENDSUBSIDY[curEra - 1] : 0;
            subsidy = (int64_t)ASSETCHAINS_REWARD[curEra];
            if ( subsidy || (curEra != ASSETCHAINS_LASTERA && ASSETCHAINS_REWARD[curEra + 1] != 0) )
            {
                if ( ASSETCHAINS_HALVING[curEra] != 0 )
                {
                    if ( (numhalvings = ((nHeight - nStart) / ASSETCHAINS_HALVING[curEra])) > 0 )
                    {
                        if ( ASSETCHAINS_DECAY[curEra] == 0 )
                            subsidy >>= numhalvings;
                        else if ( ASSETCHAINS_DECAY[curEra] == 100000000 && ASSETCHAINS_ENDSUBSIDY[curEra] != 0 )
                        {
                            if ( curEra == ASSETCHAINS_LASTERA )
                            {
                                subsidyDifference = subsidy;
                            }
                            else
                            {
                                // Ex: -ac_eras=3 -ac_reward=0,384,24 -ac_end=1440,260640,0 -ac_halving=1,1440,2103840 -ac_decay 100000000,97750000,0
                                subsidyDifference = subsidy - ASSETCHAINS_REWARD[curEra + 1];
                                if (subsidyDifference < 0)
                                {
                                    sign = -1;
                                    subsidyDifference *= sign;
                                }
                            }
                            denominator = ASSETCHAINS_ENDSUBSIDY[curEra] - nStart;
                            numerator = denominator - ((ASSETCHAINS_ENDSUBSIDY[curEra] - nHeight) + ((nHeight - nStart) % ASSETCHAINS_HALVING[curEra]));
                            subsidy = subsidy - sign * ((subsidyDifference * numerator) / denominator);
                        }
                        else
                        {
                            if ( cached_subsidy > 0 && cached_era == curEra && cached_numhalvings == numhalvings )
                                subsidy = cached_subsidy;
                            else
                            {
                                for (int i=0; i < numhalvings && subsidy != 0; i++)
                                    subsidy = (subsidy * ASSETCHAINS_DECAY[curEra]) / 100000000;
                                cached_subsidy = subsidy;
                                cached_numhalvings = numhalvings;
                                cached_era = curEra;
                            }
                        }
                    }
                }
            }
        }
    }
    uint32_t magicExtra = ASSETCHAINS_STAKED ? ASSETCHAINS_MAGIC : (ASSETCHAINS_MAGIC & 0xffffff);
    if ( ASSETCHAINS_SUPPLY > 10000000000 ) // over 10 billion?
    {
        if ( nHeight <= ASSETCHAINS_SUPPLY/1000000000 )
        {
            subsidy += (uint64_t)1000000000 * COIN;
            if ( nHeight == 1 )
                subsidy += (ASSETCHAINS_SUPPLY % 1000000000)*COIN + magicExtra;
        }
    }
    else if ( nHeight == 1 )
    {
        if ( ASSETCHAINS_LASTERA == 0 )
            subsidy = ASSETCHAINS_SUPPLY * SATOSHIDEN + magicExtra;
        else
            subsidy += ASSETCHAINS_SUPPLY * SATOSHIDEN + magicExtra;
    }
    else if ( is_STAKED(ASSETCHAINS_SYMBOL) == 2 )
        return(0);
    // LABS fungible chains, cannot have any block reward!
    return(subsidy);
}

extern int64_t MAX_MONEY;
void komodo_cbopretupdate(int32_t forceflag);
void SplitStr(const std::string& strVal, std::vector<std::string> &outVals);

int8_t equihash_params_possible(uint64_t n, uint64_t k)
{
    /* To add more of these you also need to edit:
    * equihash.cpp very end of file with the tempate to point to the new param numbers 
    * equihash.h
    *  line 210/217 (declaration of equihash class)
    * Add this object to the following functions: 
    *  EhInitialiseState 
    *  EhBasicSolve
    *  EhOptimisedSolve
    *  EhIsValidSolution
    * Alternatively change ASSETCHAINS_N and ASSETCHAINS_K in komodo_nk.h for fast testing.
    */
    if ( k == 9 && (n == 200 || n == 210) )
        return(0);
    if ( k == 5 && (n == 150 || n == 144 || n == 96 || n == 48) )
        return(0);
    if ( k == ASSETCHAINS_K && n == ASSETCHAINS_N)
        return(0);
    return(-1);
}

/***
 * Parse command line arguments
 * @param argv0 the arguments
 */
void komodo_args(char *argv0)
{
    uint8_t extrabuf[32756], *extraptr = 0;
    int32_t i;
    int32_t n;
    int32_t nonz=0;
    int32_t extralen = 0; 
    uint64_t ccEnablesHeight[512] = {0}; 

    std::string ntz_dest_path;
    ntz_dest_path = GetArg("-notary", "");
    IS_KOMODO_NOTARY = !ntz_dest_path.empty();

    STAKED_NOTARY_ID = GetArg("-stakednotary", -1);
    KOMODO_NSPV = GetArg("-nSPV",0);
    uint8_t disablebits[32];
    memset(disablebits,0,sizeof(disablebits));
    memset(ccEnablesHeight,0,sizeof(ccEnablesHeight));
    if ( GetBoolArg("-gen", false) != 0 )
    {
        KOMODO_MININGTHREADS = GetArg("-genproclimit",-1);
    }
    if ( (IS_MODE_EXCHANGEWALLET = GetBoolArg("-exchange", false)) )
        fprintf(stderr,"IS_MODE_EXCHANGEWALLET mode active\n");
    DONATION_PUBKEY = GetArg("-donation", "");
    NOTARY_PUBKEY = GetArg("-pubkey", "");
    IS_KOMODO_DEALERNODE = GetBoolArg("-dealer",false);
    IS_KOMODO_TESTNODE = GetArg("-testnode",0);
    ASSETCHAINS_STAKED_SPLIT_PERCENTAGE = GetArg("-splitperc",0);
    if ( NOTARY_PUBKEY.size() == 66 )
    {
        decode_hex(NOTARY_PUBKEY33,33,NOTARY_PUBKEY.c_str());
        USE_EXTERNAL_PUBKEY = 1;
        if ( !IS_KOMODO_NOTARY )
        {
            // We dont have any chain data yet, so use system clock to guess. 
            // I think on season change should reccomend notaries to use -notary to avoid needing this. 
            int32_t kmd_season = getacseason(time(NULL));
            for (int16_t i=0; i<64; i++)
            {
                if ( strcmp(NOTARY_PUBKEY.c_str(),notaries_elected[kmd_season-1][i][1]) == 0 )
                {
                    IS_KOMODO_NOTARY = true;
                    KOMODO_MININGTHREADS = 1;
                    mapArgs ["-genproclimit"] = itostr(KOMODO_MININGTHREADS);
                    STAKED_NOTARY_ID = -1;
                    fprintf(stderr,"running as notary.%d %s\n",i,notaries_elected[kmd_season-1][i][0]);
                    break;
                }
            }
        }
    }
    if ( STAKED_NOTARY_ID != -1 && IS_KOMODO_NOTARY ) {
        fprintf(stderr, "Cannot be STAKED and KMD notary at the same time!\n");
        StartShutdown();
    }
	std::string name = GetArg("-ac_name","");
    if ( argv0 != 0 )
    {
        size_t len = strlen(argv0);
        for (i=0; i<sizeof(argv0suffix)/sizeof(*argv0suffix); i++)
        {
            n = (int32_t)strlen(argv0suffix[i]);
            if ( strcmp(&argv0[len - n],argv0suffix[i]) == 0 )
            {
                name = argv0names[i];
                break;
            }
        }
    }
    KOMODO_STOPAT = GetArg("-stopat",0);
    MAX_REORG_LENGTH = GetArg("-maxreorg",MAX_REORG_LENGTH);
    WITNESS_CACHE_SIZE = MAX_REORG_LENGTH+10;
    ASSETCHAINS_CC = GetArg("-ac_cc",0);
    KOMODO_CCACTIVATE = GetArg("-ac_ccactivate",0);
    ASSETCHAINS_BLOCKTIME = GetArg("-ac_blocktime",60);
    ASSETCHAINS_PUBLIC = GetArg("-ac_public",0);
    ASSETCHAINS_PRIVATE = GetArg("-ac_private",0);
    KOMODO_SNAPSHOT_INTERVAL = GetArg("-ac_snapshot",0);
    Split(GetArg("-ac_nk",""), sizeof(ASSETCHAINS_NK)/sizeof(*ASSETCHAINS_NK), ASSETCHAINS_NK, 0);
    
    // -ac_ccactivateht=evalcode,height,evalcode,height,evalcode,height....
    Split(GetArg("-ac_ccactivateht",""), sizeof(ccEnablesHeight)/sizeof(*ccEnablesHeight), ccEnablesHeight, 0);
    // fill map with all eval codes and activation height of 0.
    for ( int i = 0; i < 256; i++ )
        mapHeightEvalActivate[i] = 0;
    for ( int i = 0; i < 512; i++ )
    {
        int32_t ecode = ccEnablesHeight[i];
        int32_t ht = ccEnablesHeight[i+1];
        if ( i > 1 && ccEnablesHeight[i-2] == ecode )
            break;
        if ( ecode > 255 || ecode < 0 )
            fprintf(stderr, "ac_ccactivateht: invalid evalcode.%i must be between 0 and 256.\n", ecode);
        else if ( ht > 0 )
        {
            // update global map. 
            mapHeightEvalActivate[ecode] = ht;
            fprintf(stderr, "ac_ccactivateht: ecode.%i activates at height.%i\n", ecode, mapHeightEvalActivate[ecode]);
        }
        i++;
    }
    
    if ( (KOMODO_REWIND= GetArg("-rewind",0)) != 0 )
    {
        printf("KOMODO_REWIND %d\n",KOMODO_REWIND);
    }
    KOMODO_EARLYTXID = Parseuint256(GetArg("-earlytxid","0").c_str());    
    ASSETCHAINS_EARLYTXIDCONTRACT = GetArg("-ac_earlytxidcontract",0);
    if ( name.c_str()[0] != 0 )
    {
        std::string selectedAlgo = GetArg("-ac_algo", std::string(ASSETCHAINS_ALGORITHMS[0]));
        int i;
        for (i = 0; i < ASSETCHAINS_NUMALGOS; i++ )
        {
            if (std::string(ASSETCHAINS_ALGORITHMS[i]) == selectedAlgo)
            {
                ASSETCHAINS_ALGO = i;
                STAKING_MIN_DIFF = ASSETCHAINS_MINDIFF[i];
                // only worth mentioning if it's not equihash
                if (ASSETCHAINS_ALGO != ASSETCHAINS_EQUIHASH)
                    printf("ASSETCHAINS_ALGO, algorithm set to %s\n", selectedAlgo.c_str());
                break;
            }
        }
        if ( ASSETCHAINS_ALGO == ASSETCHAINS_EQUIHASH && ASSETCHAINS_NK[0] != 0 && ASSETCHAINS_NK[1] != 0 )
        {
            if ( equihash_params_possible(ASSETCHAINS_NK[0], ASSETCHAINS_NK[1]) == -1 ) 
            {
                printf("equihash values N.%li and K.%li are not currently available\n", ASSETCHAINS_NK[0], ASSETCHAINS_NK[1]);
                exit(0);
            } else printf("ASSETCHAINS_ALGO, algorithm set to equihash with N.%li and K.%li\n", ASSETCHAINS_NK[0], ASSETCHAINS_NK[1]);
        }
        if (i == ASSETCHAINS_NUMALGOS)
        {
            printf("ASSETCHAINS_ALGO, %s not supported. using equihash\n", selectedAlgo.c_str());
        }

        ASSETCHAINS_LASTERA = GetArg("-ac_eras", 1);
        if ( ASSETCHAINS_LASTERA < 1 || ASSETCHAINS_LASTERA > ASSETCHAINS_MAX_ERAS )
        {
            ASSETCHAINS_LASTERA = 1;
            printf("ASSETCHAINS_LASTERA, if specified, must be between 1 and %u. ASSETCHAINS_LASTERA set to %lu\n", ASSETCHAINS_MAX_ERAS, ASSETCHAINS_LASTERA);
        }
        ASSETCHAINS_LASTERA -= 1;

        ASSETCHAINS_TIMELOCKGTE = (uint64_t)GetArg("-ac_timelockgte", _ASSETCHAINS_TIMELOCKOFF);
        ASSETCHAINS_TIMEUNLOCKFROM = GetArg("-ac_timeunlockfrom", 0);
        ASSETCHAINS_TIMEUNLOCKTO = GetArg("-ac_timeunlockto", 0);
        if ( ASSETCHAINS_TIMEUNLOCKFROM > ASSETCHAINS_TIMEUNLOCKTO )
        {
            printf("ASSETCHAINS_TIMELOCKGTE - must specify valid ac_timeunlockfrom and ac_timeunlockto\n");
            ASSETCHAINS_TIMELOCKGTE = _ASSETCHAINS_TIMELOCKOFF;
            ASSETCHAINS_TIMEUNLOCKFROM = ASSETCHAINS_TIMEUNLOCKTO = 0;
        }

        Split(GetArg("-ac_end",""), sizeof(ASSETCHAINS_ENDSUBSIDY)/sizeof(*ASSETCHAINS_ENDSUBSIDY),  ASSETCHAINS_ENDSUBSIDY, 0);
        Split(GetArg("-ac_reward",""), sizeof(ASSETCHAINS_REWARD)/sizeof(*ASSETCHAINS_REWARD),  ASSETCHAINS_REWARD, 0);
        Split(GetArg("-ac_halving",""), sizeof(ASSETCHAINS_HALVING)/sizeof(*ASSETCHAINS_HALVING),  ASSETCHAINS_HALVING, 0);
        Split(GetArg("-ac_decay",""), sizeof(ASSETCHAINS_DECAY)/sizeof(*ASSETCHAINS_DECAY),  ASSETCHAINS_DECAY, 0);
        Split(GetArg("-ac_notarypay",""), sizeof(ASSETCHAINS_NOTARY_PAY)/sizeof(*ASSETCHAINS_NOTARY_PAY),  ASSETCHAINS_NOTARY_PAY, 0);

        for ( int i = 0; i < ASSETCHAINS_MAX_ERAS; i++ )
        {
            if ( ASSETCHAINS_DECAY[i] == 100000000 && ASSETCHAINS_ENDSUBSIDY == 0 )
            {
                ASSETCHAINS_DECAY[i] = 0;
                printf("ERA%u: ASSETCHAINS_DECAY of 100000000 means linear and that needs ASSETCHAINS_ENDSUBSIDY\n", i);
            }
            else if ( ASSETCHAINS_DECAY[i] > 100000000 )
            {
                ASSETCHAINS_DECAY[i] = 0;
                printf("ERA%u: ASSETCHAINS_DECAY cant be more than 100000000\n", i);
            }
        }

        MAX_BLOCK_SIGOPS = 60000;
        ASSETCHAINS_TXPOW = GetArg("-ac_txpow",0) & 3;
        ASSETCHAINS_FOUNDERS = GetArg("-ac_founders",0);// & 1;
		ASSETCHAINS_FOUNDERS_REWARD = GetArg("-ac_founders_reward",0);
        ASSETCHAINS_SUPPLY = GetArg("-ac_supply",10);
        if ( ASSETCHAINS_SUPPLY > (uint64_t)90*1000*1000000 )
        {
            fprintf(stderr,"-ac_supply must be less than 90 billion\n");
            StartShutdown();
        }
        fprintf(stderr,"ASSETCHAINS_SUPPLY %llu\n",(long long)ASSETCHAINS_SUPPLY);
        
        ASSETCHAINS_COMMISSION = GetArg("-ac_perc",0);
        ASSETCHAINS_OVERRIDE_PUBKEY = GetArg("-ac_pubkey","");
        ASSETCHAINS_SCRIPTPUB = GetArg("-ac_script","");
        ASSETCHAINS_BEAMPORT = GetArg("-ac_beam",0);
        ASSETCHAINS_CODAPORT = GetArg("-ac_coda",0);
        ASSETCHAINS_MARMARA = GetArg("-ac_marmara",0);
        ASSETCHAINS_CBOPRET = GetArg("-ac_cbopret",0);
        ASSETCHAINS_CBMATURITY = GetArg("-ac_cbmaturity",0);
        ASSETCHAINS_ADAPTIVEPOW = GetArg("-ac_adaptivepow",0);
        if ( ASSETCHAINS_CBOPRET != 0 )
        {
            SplitStr(GetArg("-ac_prices",""),  ASSETCHAINS_PRICES);
            if ( ASSETCHAINS_PRICES.size() > 0 )
                ASSETCHAINS_CBOPRET |= 4;
            SplitStr(GetArg("-ac_stocks",""),  ASSETCHAINS_STOCKS);
            if ( ASSETCHAINS_STOCKS.size() > 0 )
                ASSETCHAINS_CBOPRET |= 8;
            for (i=0; i<ASSETCHAINS_PRICES.size(); i++)
                fprintf(stderr,"%s ",ASSETCHAINS_PRICES[i].c_str());
            fprintf(stderr,"%d -ac_prices\n",(int32_t)ASSETCHAINS_PRICES.size());
            for (i=0; i<ASSETCHAINS_STOCKS.size(); i++)
                fprintf(stderr,"%s ",ASSETCHAINS_STOCKS[i].c_str());
            fprintf(stderr,"%d -ac_stocks\n",(int32_t)ASSETCHAINS_STOCKS.size());
        }
        std::string hexstr = GetArg("-ac_mineropret","");
        if ( !hexstr.empty() )
        {
            Mineropret.resize(hexstr.size()/2);
            decode_hex(Mineropret.data(),hexstr.size()/2,hexstr.c_str());
            for (i=0; i<Mineropret.size(); i++)
                fprintf(stderr,"%02x",Mineropret[i]);
            fprintf(stderr," Mineropret\n");
        }
        if ( ASSETCHAINS_COMMISSION != 0 && ASSETCHAINS_FOUNDERS_REWARD != 0 )
        {
            fprintf(stderr,"cannot use founders reward and commission on the same chain.\n");
            StartShutdown();
        }
        if ( ASSETCHAINS_CC != 0 )
        {
            uint8_t prevCCi = 0;
            uint64_t ccenables[256];
            memset(ccenables,0,sizeof(ccenables));
            ASSETCHAINS_CCLIB = GetArg("-ac_cclib","");
            Split(GetArg("-ac_ccenable",""), sizeof(ccenables)/sizeof(*ccenables),  ccenables, 0);
            for (i=nonz=0; i<0x100; i++)
            {
                if ( ccenables[i] != prevCCi && ccenables[i] != 0 )
                {
                    nonz++;
                    prevCCi = ccenables[i];
                    fprintf(stderr,"%d ",(uint8_t)(ccenables[i] & 0xff));
                }
            }
            fprintf(stderr,"nonz.%d ccenables[]\n",nonz);
            if ( nonz > 0 )
            {
                for (i=0; i<256; i++)
                {
                    ASSETCHAINS_CCDISABLES[i] = 1;
                    SETBIT(disablebits,i);
                }
                for (i=0; i<nonz; i++)
                {
                    CLEARBIT(disablebits,(ccenables[i] & 0xff));
                    ASSETCHAINS_CCDISABLES[ccenables[i] & 0xff] = 0;
                }
                CLEARBIT(disablebits,0);
            }
        }
        if ( ASSETCHAINS_BEAMPORT != 0 )
        {
            fprintf(stderr,"can only have one of -ac_beam or -ac_coda\n");
            StartShutdown();
        }
        ASSETCHAINS_SELFIMPORT = GetArg("-ac_import",""); // BEAM, CODA, PUBKEY, GATEWAY
        if ( ASSETCHAINS_SELFIMPORT == "PUBKEY" )
        {
            if ( strlen(ASSETCHAINS_OVERRIDE_PUBKEY.c_str()) != 66 )
            {
                fprintf(stderr,"invalid -ac_pubkey for -ac_import=PUBKEY\n");
                StartShutdown();
            }
        }
        else if ( ASSETCHAINS_SELFIMPORT == "BEAM" )
        {
            if (ASSETCHAINS_BEAMPORT == 0)
            {
                fprintf(stderr,"missing -ac_beam for BEAM rpcport\n");
                StartShutdown();
            }
        }
        else if ( ASSETCHAINS_SELFIMPORT == "CODA" )
        {
            if (ASSETCHAINS_CODAPORT == 0)
            {
                fprintf(stderr,"missing -ac_coda for CODA rpcport\n");
                StartShutdown();
            }
        }
        else if ( ASSETCHAINS_SELFIMPORT == "PEGSCC")
        {
            Split(GetArg("-ac_pegsccparams",""), sizeof(ASSETCHAINS_PEGSCCPARAMS)/sizeof(*ASSETCHAINS_PEGSCCPARAMS), ASSETCHAINS_PEGSCCPARAMS, 0);
            if (ASSETCHAINS_ENDSUBSIDY[0]!=1 || ASSETCHAINS_COMMISSION!=0)
            {
                fprintf(stderr,"when using import for pegsCC these must be set: -ac_end=1 -ac_perc=0\n");
                StartShutdown();
            }
        }
        // else it can be gateway coin
        else if (!ASSETCHAINS_SELFIMPORT.empty() && (ASSETCHAINS_ENDSUBSIDY[0]!=1 || ASSETCHAINS_SUPPLY>0 || ASSETCHAINS_COMMISSION!=0))
        {
            fprintf(stderr,"when using gateway import these must be set: -ac_end=1 -ac_supply=0 -ac_perc=0\n");
            StartShutdown();
        }
        

        if ( (ASSETCHAINS_STAKED= GetArg("-ac_staked",0)) > 100 )
            ASSETCHAINS_STAKED = 100;

        // for now, we only support 50% PoS due to other parts of the algorithm needing adjustment for
        // other values
        if ( (ASSETCHAINS_LWMAPOS = GetArg("-ac_veruspos",0)) != 0 )
        {
            ASSETCHAINS_LWMAPOS = 50;
        }
        ASSETCHAINS_SAPLING = GetArg("-ac_sapling", -1);
        if (ASSETCHAINS_SAPLING == -1)
        {
            ASSETCHAINS_OVERWINTER = GetArg("-ac_overwinter", -1);
        }
        else
        {
            ASSETCHAINS_OVERWINTER = GetArg("-ac_overwinter", ASSETCHAINS_SAPLING);
        }
        if ( strlen(ASSETCHAINS_OVERRIDE_PUBKEY.c_str()) == 66 || ASSETCHAINS_SCRIPTPUB.size() > 1 )
        {
            if ( ASSETCHAINS_SUPPLY > 10000000000 )
            {
                printf("ac_pubkey or ac_script wont work with ac_supply over 10 billion\n");
                StartShutdown();
            }
            if ( ASSETCHAINS_NOTARY_PAY[0] != 0 )
            {
                printf("Assetchains NOTARY PAY cannot be used with ac_pubkey or ac_script.\n");
                StartShutdown();
            }
            if ( strlen(ASSETCHAINS_OVERRIDE_PUBKEY.c_str()) == 66 )
            {
                decode_hex(ASSETCHAINS_OVERRIDE_PUBKEY33,33,ASSETCHAINS_OVERRIDE_PUBKEY.c_str());
                calc_rmd160_sha256(ASSETCHAINS_OVERRIDE_PUBKEYHASH,ASSETCHAINS_OVERRIDE_PUBKEY33,33);
            }
            if ( ASSETCHAINS_COMMISSION == 0 && ASSETCHAINS_FOUNDERS != 0 )
            {
                if ( ASSETCHAINS_FOUNDERS_REWARD == 0 )
                {
                    ASSETCHAINS_COMMISSION = 53846154; // maps to 35%
                    printf("ASSETCHAINS_COMMISSION defaulted to 35%% when founders reward active\n");
                }
                else
                {
                    printf("ASSETCHAINS_FOUNDERS_REWARD set to %ld\n", ASSETCHAINS_FOUNDERS_REWARD);
                }
            }
        }
        else
        {
            if ( ASSETCHAINS_COMMISSION != 0 )
            {
                ASSETCHAINS_COMMISSION = 0;
                printf("ASSETCHAINS_COMMISSION needs an ASSETCHAINS_OVERRIDE_PUBKEY and cant be more than 100000000 (100%%)\n");
            }
            if ( ASSETCHAINS_FOUNDERS != 0 )
            {
                ASSETCHAINS_FOUNDERS = 0;
                printf("ASSETCHAINS_FOUNDERS needs an ASSETCHAINS_OVERRIDE_PUBKEY or ASSETCHAINS_SCRIPTPUB\n");
            }
        }
        if ( ASSETCHAINS_SCRIPTPUB.size() > 1 && ASSETCHAINS_MARMARA != 0 )
        {
            fprintf(stderr,"-ac_script and -ac_marmara are mutually exclusive\n");
            StartShutdown();
        }
        if ( ASSETCHAINS_ENDSUBSIDY[0] != 0 || ASSETCHAINS_REWARD[0] != 0 || ASSETCHAINS_HALVING[0] != 0 
                || ASSETCHAINS_DECAY[0] != 0 || ASSETCHAINS_COMMISSION != 0 || ASSETCHAINS_PUBLIC != 0 
                || ASSETCHAINS_PRIVATE != 0 || ASSETCHAINS_TXPOW != 0 || ASSETCHAINS_FOUNDERS != 0 
                || ASSETCHAINS_SCRIPTPUB.size() > 1 || ASSETCHAINS_SELFIMPORT.size() > 0 
                || ASSETCHAINS_OVERRIDE_PUBKEY33[0] != 0 || ASSETCHAINS_TIMELOCKGTE != _ASSETCHAINS_TIMELOCKOFF
                || ASSETCHAINS_ALGO != ASSETCHAINS_EQUIHASH || ASSETCHAINS_LWMAPOS != 0 || ASSETCHAINS_LASTERA > 0 
                || ASSETCHAINS_BEAMPORT != 0 || ASSETCHAINS_CODAPORT != 0 || ASSETCHAINS_MARMARA != 0 || nonz > 0 
                || ASSETCHAINS_CCLIB.size() > 0 || ASSETCHAINS_FOUNDERS_REWARD != 0 || ASSETCHAINS_NOTARY_PAY[0] != 0 
                || ASSETCHAINS_BLOCKTIME != 60 || ASSETCHAINS_CBOPRET != 0 || Mineropret.size() != 0 
                || (ASSETCHAINS_NK[0] != 0 && ASSETCHAINS_NK[1] != 0) || KOMODO_SNAPSHOT_INTERVAL != 0 
                || ASSETCHAINS_EARLYTXIDCONTRACT != 0 || ASSETCHAINS_CBMATURITY != 0 || ASSETCHAINS_ADAPTIVEPOW != 0 )
        {
            fprintf(stderr,"perc %.4f%% ac_pub=[%02x%02x%02x...] acsize.%d\n",dstr(ASSETCHAINS_COMMISSION)*100,
                    ASSETCHAINS_OVERRIDE_PUBKEY33[0],ASSETCHAINS_OVERRIDE_PUBKEY33[1],ASSETCHAINS_OVERRIDE_PUBKEY33[2],
                    (int32_t)ASSETCHAINS_SCRIPTPUB.size());
            extraptr = extrabuf;
            memcpy(extraptr,ASSETCHAINS_OVERRIDE_PUBKEY33,33), extralen = 33;

            // if we have one era, this should create the same data structure as it used to, same if we increase _MAX_ERAS
            for ( int i = 0; i <= ASSETCHAINS_LASTERA; i++ )
            {
                printf("ERA%u: end.%llu reward.%llu halving.%llu decay.%llu notarypay.%llu\n", i,
                       (long long)ASSETCHAINS_ENDSUBSIDY[i],
                       (long long)ASSETCHAINS_REWARD[i],
                       (long long)ASSETCHAINS_HALVING[i],
                       (long long)ASSETCHAINS_DECAY[i],
                       (long long)ASSETCHAINS_NOTARY_PAY[i]);

                // TODO: Verify that we don't overrun extrabuf here, which is a 256 byte buffer
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_ENDSUBSIDY[i]),(void *)&ASSETCHAINS_ENDSUBSIDY[i]);
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_REWARD[i]),(void *)&ASSETCHAINS_REWARD[i]);
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_HALVING[i]),(void *)&ASSETCHAINS_HALVING[i]);
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_DECAY[i]),(void *)&ASSETCHAINS_DECAY[i]);
                if ( ASSETCHAINS_NOTARY_PAY[0] != 0 )
                    extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_NOTARY_PAY[i]),(void *)&ASSETCHAINS_NOTARY_PAY[i]);
            }

            if (ASSETCHAINS_LASTERA > 0)
            {
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_LASTERA),(void *)&ASSETCHAINS_LASTERA);
            }

            // hash in lock above for time locked coinbase transactions above a certain reward value only if the lock above
            // param was specified, otherwise, for compatibility, do nothing
            if ( ASSETCHAINS_TIMELOCKGTE != _ASSETCHAINS_TIMELOCKOFF )
            {
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_TIMELOCKGTE),(void *)&ASSETCHAINS_TIMELOCKGTE);
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_TIMEUNLOCKFROM),(void *)&ASSETCHAINS_TIMEUNLOCKFROM);
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_TIMEUNLOCKTO),(void *)&ASSETCHAINS_TIMEUNLOCKTO);
            }

            if ( ASSETCHAINS_ALGO != ASSETCHAINS_EQUIHASH )
            {
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_ALGO),(void *)&ASSETCHAINS_ALGO);
            }

            if ( ASSETCHAINS_LWMAPOS != 0 )
            {
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_LWMAPOS),(void *)&ASSETCHAINS_LWMAPOS);
            }

            uint64_t val = ASSETCHAINS_COMMISSION | (((int64_t)ASSETCHAINS_STAKED & 0xff) << 32) | (((uint64_t)ASSETCHAINS_CC & 0xffff) << 40) | ((ASSETCHAINS_PUBLIC != 0) << 7) | ((ASSETCHAINS_PRIVATE != 0) << 6) | ASSETCHAINS_TXPOW;
            extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(val),(void *)&val);
            
            if ( ASSETCHAINS_FOUNDERS != 0 )
            {
                uint8_t tmp = 1;
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(tmp),(void *)&tmp);
                if ( ASSETCHAINS_FOUNDERS > 1 )
                    extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_FOUNDERS),(void *)&ASSETCHAINS_FOUNDERS);
                if ( ASSETCHAINS_FOUNDERS_REWARD != 0 )
                {
                    fprintf(stderr, "set founders reward.%lld\n",(long long)ASSETCHAINS_FOUNDERS_REWARD);
                    extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_FOUNDERS_REWARD),(void *)&ASSETCHAINS_FOUNDERS_REWARD);
                }
            }
            if ( ASSETCHAINS_SCRIPTPUB.size() > 1 )
            {
                decode_hex(&extraptr[extralen],ASSETCHAINS_SCRIPTPUB.size()/2,ASSETCHAINS_SCRIPTPUB.c_str());
                extralen += ASSETCHAINS_SCRIPTPUB.size()/2;
                fprintf(stderr,"append ac_script %s\n",ASSETCHAINS_SCRIPTPUB.c_str());
            }
            if ( ASSETCHAINS_SELFIMPORT.size() > 0 )
            {
                memcpy(&extraptr[extralen],(char *)ASSETCHAINS_SELFIMPORT.c_str(),ASSETCHAINS_SELFIMPORT.size());
                for (i=0; i<ASSETCHAINS_SELFIMPORT.size(); i++)
                    fprintf(stderr,"%c",extraptr[extralen+i]);
                fprintf(stderr," selfimport\n");
                extralen += ASSETCHAINS_SELFIMPORT.size();
            }
            if ( ASSETCHAINS_BEAMPORT != 0 )
                extraptr[extralen++] = 'b';
            if ( ASSETCHAINS_CODAPORT != 0 )
                extraptr[extralen++] = 'c';
            if ( ASSETCHAINS_MARMARA != 0 )
                extraptr[extralen++] = ASSETCHAINS_MARMARA;
            fprintf(stderr,"extralen.%d before disable bits\n",extralen);
            if ( nonz > 0 )
            {
                memcpy(&extraptr[extralen],disablebits,sizeof(disablebits));
                extralen += sizeof(disablebits);
            }
            if ( ASSETCHAINS_CCLIB.size() > 1 )
            {
                for (i=0; i<ASSETCHAINS_CCLIB.size(); i++)
                {
                    extraptr[extralen++] = ASSETCHAINS_CCLIB[i];
                    fprintf(stderr,"%c",ASSETCHAINS_CCLIB[i]);
                }
                fprintf(stderr," <- CCLIB name\n");
            }
            if ( ASSETCHAINS_BLOCKTIME != 60 )
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_BLOCKTIME),(void *)&ASSETCHAINS_BLOCKTIME);
            if ( Mineropret.size() != 0 )
            {
                for (i=0; i<Mineropret.size(); i++)
                    extraptr[extralen++] = Mineropret[i];
            }
            if ( ASSETCHAINS_CBOPRET != 0 )
            {
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_CBOPRET),(void *)&ASSETCHAINS_CBOPRET);
                if ( ASSETCHAINS_PRICES.size() != 0 )
                {
                    for (i=0; i<ASSETCHAINS_PRICES.size(); i++)
                    {
                        std::string symbol = ASSETCHAINS_PRICES[i];
                        memcpy(&extraptr[extralen],(char *)symbol.c_str(),symbol.size());
                        extralen += symbol.size();
                    }
                }
                if ( ASSETCHAINS_STOCKS.size() != 0 )
                {
                    for (i=0; i<ASSETCHAINS_STOCKS.size(); i++)
                    {
                        std::string symbol = ASSETCHAINS_STOCKS[i];
                        memcpy(&extraptr[extralen],(char *)symbol.c_str(),symbol.size());
                        extralen += symbol.size();
                    }
                }
                komodo_cbopretupdate(1); // will set Mineropret
                fprintf(stderr,"This blockchain uses data produced from CoinDesk Bitcoin Price Index\n");
            }
            if ( ASSETCHAINS_NK[0] != 0 && ASSETCHAINS_NK[1] != 0 )
            {
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_NK[0]),(void *)&ASSETCHAINS_NK[0]);
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_NK[1]),(void *)&ASSETCHAINS_NK[1]);
            }
            if ( KOMODO_SNAPSHOT_INTERVAL != 0 )
            {
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(KOMODO_SNAPSHOT_INTERVAL),(void *)&KOMODO_SNAPSHOT_INTERVAL);
            }
            if ( ASSETCHAINS_EARLYTXIDCONTRACT != 0 )
            {
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_EARLYTXIDCONTRACT),(void *)&ASSETCHAINS_EARLYTXIDCONTRACT);
            }
            if ( ASSETCHAINS_CBMATURITY != 0 )
            {
                extralen += iguana_rwnum(1,&extraptr[extralen],sizeof(ASSETCHAINS_CBMATURITY),(void *)&ASSETCHAINS_CBMATURITY);
            }
            if ( ASSETCHAINS_ADAPTIVEPOW != 0 )
                extraptr[extralen++] = ASSETCHAINS_ADAPTIVEPOW;
        }
        
        std::string addn = GetArg("-seednode","");
        if ( !addn.empty() )
            ASSETCHAINS_SEED = 1;

        strncpy(ASSETCHAINS_SYMBOL,name.c_str(),sizeof(ASSETCHAINS_SYMBOL)-1);

        MAX_MONEY = komodo_max_money();

        int32_t baseid;
        if ( (baseid = komodo_baseid(ASSETCHAINS_SYMBOL)) >= 0 && baseid < 32 )
        {
            printf("baseid.%d MAX_MONEY.%s %.8f\n",baseid,ASSETCHAINS_SYMBOL,(double)MAX_MONEY/SATOSHIDEN);
        }

        if ( ASSETCHAINS_CC >= KOMODO_FIRSTFUNGIBLEID && MAX_MONEY < 1000000LL*SATOSHIDEN )
            MAX_MONEY = 1000000LL*SATOSHIDEN;
        if ( KOMODO_BIT63SET(MAX_MONEY) != 0 )
            MAX_MONEY = KOMODO_MAXNVALUE;
        fprintf(stderr,"MAX_MONEY %llu %.8f\n",(long long)MAX_MONEY,(double)MAX_MONEY/SATOSHIDEN);
        uint16_t tmpport = komodo_port(ASSETCHAINS_SYMBOL,ASSETCHAINS_SUPPLY,&ASSETCHAINS_MAGIC,extraptr,extralen);
        if ( GetArg("-port",0) != 0 )
        {
            ASSETCHAINS_P2PPORT = GetArg("-port",0);
            fprintf(stderr,"set p2pport.%u\n",ASSETCHAINS_P2PPORT);
        } else ASSETCHAINS_P2PPORT = tmpport;

        char *dirname;
        while ( (dirname = (char *)GetDataDir(false).string().c_str()) == 0 || dirname[0] == 0 )
        {
            fprintf(stderr,"waiting for datadir (%s)\n",dirname);
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
        if ( ASSETCHAINS_SYMBOL[0] != 0 )
        {
            if ( strcmp(ASSETCHAINS_SYMBOL,"KMD") == 0 )
            {
                fprintf(stderr,"cant have assetchain named KMD\n");
                StartShutdown();
            }
            uint16_t port;
            if ( (port= komodo_userpass(ASSETCHAINS_USERPASS,ASSETCHAINS_SYMBOL)) != 0 )
                // config file found and read
                ASSETCHAINS_RPCPORT = port;
            else
            { 
                // config file was not found, generate default
                komodo_configfile(ASSETCHAINS_SYMBOL,ASSETCHAINS_P2PPORT + 1);
                BITCOIND_RPCPORT = ASSETCHAINS_P2PPORT + 1;
                // now try to load the komodo.conf
                try
                {
                    ConfigFile config_f(GetConfigFile("KMD"));
                    if (config_f.Has("rpcport"))
                        KMD_PORT = std::stoi(config_f.Value("rpcport"));
                    strcpy(KMDUSERPASS, (config_f.Value("rpcuser") + ":" + config_f.Value("rpcpassword")).c_str());
                }
                catch( const std::ios_base::failure& ex)
                {
                    LogPrintf("Unable to open komodo.conf: %s\n", ex.what() );
                }                
            }

            extern int COINBASE_MATURITY;
            if (ASSETCHAINS_CBMATURITY != 0)
                COINBASE_MATURITY = ASSETCHAINS_CBMATURITY;
            else if (ASSETCHAINS_LASTERA == 0 || is_STAKED(ASSETCHAINS_SYMBOL) != 0)
                COINBASE_MATURITY = 1;
            if (COINBASE_MATURITY < 1)
            {
                fprintf(stderr,"ac_cbmaturity must be >0, shutting down\n");
                StartShutdown();
            }
        }
        if ( ASSETCHAINS_RPCPORT == 0 )
            ASSETCHAINS_RPCPORT = ASSETCHAINS_P2PPORT + 1;
        uint8_t magic[4];
        iguana_rwnum(1,magic,sizeof(ASSETCHAINS_MAGIC),(void *)&ASSETCHAINS_MAGIC);
#ifndef FROM_CLI
        std::string fname = std::string(ASSETCHAINS_SYMBOL) + "_7776";
        FILE* fp;
        if ( (fp= fopen(fname.c_str(),"wb")) != 0 )
        {
            char magicstr[9];         
            for (i=0; i<4; i++)
                sprintf(&magicstr[i<<1],"%02x",magic[i]);
            magicstr[8] = 0;
            int8_t notarypay = 0;
            if ( ASSETCHAINS_NOTARY_PAY[0] != 0 )
                notarypay = 1;
            fprintf(fp,iguanafmtstr,name.c_str(),name.c_str(),name.c_str(),name.c_str(),magicstr,ASSETCHAINS_P2PPORT,ASSETCHAINS_RPCPORT,"78.47.196.146",notarypay);
            fclose(fp);
        } else printf("error creating (%s)\n",fname.c_str());
#endif
        if ( ASSETCHAINS_CC < 2 )
        {
            if ( KOMODO_CCACTIVATE != 0 )
            {
                ASSETCHAINS_CC = 2;
                fprintf(stderr,"smart utxo CC contracts will activate at height.%d\n",KOMODO_CCACTIVATE);
            }
            else if ( ccEnablesHeight[0] != 0 )
            {
                ASSETCHAINS_CC = 2;
                fprintf(stderr,"smart utxo CC contract %d will activate at height.%d\n",(int32_t)ccEnablesHeight[0],(int32_t)ccEnablesHeight[1]);
            }
        }
    }
    else // no -ac_name set, assume komodo chain
    {
        char fname[512],username[512],password[4096]; 
        int32_t iter; 
        FILE *fp;
        ASSETCHAINS_P2PPORT = 7770;
        ASSETCHAINS_RPCPORT = 7771;
        DEST_PORT = 0;
        try
        {
            ConfigFile config_f(GetConfigFile("KMD"));
            strcpy(KMDUSERPASS, (config_f.Value("rpcuser") + ":" + config_f.Value("rpcpassword")).c_str());
        }
        catch(const ios_base::failure& ex)
        {
            LogPrintf("Unable to open komodo.conf: %s\n", ex.what() );
        }
        if (IS_KOMODO_NOTARY)
        {
            try
            {
                ConfigFile config_f{ boost::filesystem::path(ntz_dest_path) };
                DEST_PORT = std::stoi(config_f.Value("rpcport"));
                strcpy(BTCUSERPASS, (config_f.Value("rpcuser") + ":" + config_f.Value("rpcpassword")).c_str());
            }
            catch(const ios_base::failure& ex)
            {
                LogPrintf("Could not open (%s). Will not validate dest notarizations. Reason: %s\n", 
                        ntz_dest_path.c_str(), ex.what());
            }
        }
    }
    int32_t dpowconfs = KOMODO_DPOWCONFS;
    if ( ASSETCHAINS_SYMBOL[0] != 0 )
    {
        BITCOIND_RPCPORT = GetArg("-rpcport", ASSETCHAINS_RPCPORT);
        if ( strcmp("PIRATE",ASSETCHAINS_SYMBOL) == 0 && ASSETCHAINS_HALVING[0] == 77777 )
        {
            ASSETCHAINS_HALVING[0] *= 5;
            fprintf(stderr,"PIRATE halving changed to %d %.1f days ASSETCHAINS_LASTERA.%llu\n",(int32_t)ASSETCHAINS_HALVING[0],(double)ASSETCHAINS_HALVING[0]/1440,(long long)ASSETCHAINS_LASTERA);
        }
        else if ( strcmp("VRSC",ASSETCHAINS_SYMBOL) == 0 )
            dpowconfs = 0;
        else if ( ASSETCHAINS_PRIVATE != 0 )
        {
            fprintf(stderr,"-ac_private for a non-PIRATE chain is not supported. The only reason to have an -ac_private chain is for total privacy and that is best achieved with the largest anon set. PIRATE has that and it is recommended to just use PIRATE\n");
            StartShutdown();
        }
        // Set cc enables for all existing ac_cc chains here. 
        if ( strcmp("AXO",ASSETCHAINS_SYMBOL) == 0 )
        {
            // No CCs used on this chain yet.
            CCDISABLEALL;
        }
        if ( strcmp("CCL",ASSETCHAINS_SYMBOL) == 0 )
        {
            // No CCs used on this chain yet. 
            CCDISABLEALL;
            CCENABLE(EVAL_TOKENS);
            CCENABLE(EVAL_HEIR);
        }
        if ( strcmp("COQUI",ASSETCHAINS_SYMBOL) == 0 )
        {
            CCDISABLEALL;
            CCENABLE(EVAL_DICE);
            CCENABLE(EVAL_CHANNELS);
            CCENABLE(EVAL_ORACLES);
            CCENABLE(EVAL_ASSETS);
            CCENABLE(EVAL_TOKENS);
        }
        if ( strcmp("DION",ASSETCHAINS_SYMBOL) == 0 )
        {
            // No CCs used on this chain yet. 
            CCDISABLEALL;
        }
        
        if ( strcmp("EQL",ASSETCHAINS_SYMBOL) == 0 )
        {
            // No CCs used on this chain yet. 
            CCDISABLEALL;
        }
        if ( strcmp("ILN",ASSETCHAINS_SYMBOL) == 0 )
        {
            // No CCs used on this chain yet. 
            CCDISABLEALL;
        }
        if ( strcmp("OUR",ASSETCHAINS_SYMBOL) == 0 )
        {
            // No CCs used on this chain yet. 
            CCDISABLEALL;
        }
        if ( strcmp("ZEXO",ASSETCHAINS_SYMBOL) == 0 )
        {
            // No CCs used on this chain yet. 
            CCDISABLEALL;
        }
        if ( strcmp("SEC",ASSETCHAINS_SYMBOL) == 0 )
        {
            CCDISABLEALL;
            CCENABLE(EVAL_ASSETS);
            CCENABLE(EVAL_TOKENS);
            CCENABLE(EVAL_ORACLES);
        }
        if ( strcmp("KMDICE",ASSETCHAINS_SYMBOL) == 0 )
        {
            CCDISABLEALL;
            CCENABLE(EVAL_FAUCET);
            CCENABLE(EVAL_DICE);
            CCENABLE(EVAL_ORACLES);
        }
    } 
    else 
        BITCOIND_RPCPORT = GetArg("-rpcport", BaseParams().RPCPort());
    KOMODO_DPOWCONFS = GetArg("-dpowconfs",dpowconfs);
    if ( ASSETCHAINS_SYMBOL[0] == 0 || strcmp(ASSETCHAINS_SYMBOL,"SUPERNET") == 0 
            || strcmp(ASSETCHAINS_SYMBOL,"DEX") == 0 || strcmp(ASSETCHAINS_SYMBOL,"COQUI") == 0 
            || strcmp(ASSETCHAINS_SYMBOL,"PIRATE") == 0 || strcmp(ASSETCHAINS_SYMBOL,"KMDICE") == 0 )
        KOMODO_EXTRASATOSHI = 1;
}

/****
 * Set the correct combination of symbol and dest based on source
 * @param[out] symbol the source, or KMD if source is empty
 * @param[out] dest the destination
 * @param[in] source the determinant
 */
void komodo_nameset(char *symbol,char *dest,const char *source)
{
    if ( source[0] == 0 )
    {
        strcpy(symbol,(char *)"KMD");
        strcpy(dest,(char *)"BTC");
    }
    else
    {
        strcpy(symbol,source);
        strcpy(dest,(char *)"KMD");
    }
}

struct komodo_state *komodo_stateptrget(char *base)
{
    int32_t baseid;
    if ( base == 0 || base[0] == 0 || strcmp(base,(char *)"KMD") == 0 )
        return(&KOMODO_STATES[33]);
    else if ( (baseid= komodo_baseid(base)) >= 0 )
        return(&KOMODO_STATES[baseid+1]);
    else 
        return(&KOMODO_STATES[0]);
}

struct komodo_state *komodo_stateptr(char *symbol,char *dest)
{
    int32_t baseid;
    komodo_nameset(symbol,dest,ASSETCHAINS_SYMBOL);
    return komodo_stateptrget(symbol);
}

void komodo_prefetch(FILE *fp)
{
    long fsize,fpos; int32_t incr = 16*1024*1024;
    fpos = ftell(fp);
    fseek(fp,0,SEEK_END);
    fsize = ftell(fp);
    if ( fsize > incr )
    {
        char *ignore = (char *)malloc(incr);
        if ( ignore != 0 )
        {
            rewind(fp);
            while ( fread(ignore,1,incr,fp) == incr ) // prefetch
                fprintf(stderr,".");
            free(ignore);
        }
    }
    fseek(fp,fpos,SEEK_SET);
}

// check if block timestamp is more than S5 activation time
// this function is to activate the ExtractDestination fix 
bool komodo_is_vSolutionsFixActive()
{
    return GetLatestTimestamp(komodo_currentheight()) > nS5Timestamp;
}
