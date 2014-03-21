#ifndef _HMAC_MD5_H_
#define _HMAC_MD5_H_

typedef unsigned char md5_digest_t[16];

void hmac_md5(
    unsigned char*  text,                /* pointer to data stream */
    int             text_len,            /* length of data stream */
    unsigned char*  key,                 /* pointer to authentication key */
    int             key_len,             /* length of authentication key */
    md5_digest_t    digest);            /* caller digest to be filled in */

#endif
