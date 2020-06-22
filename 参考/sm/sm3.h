#ifndef __KOAL_SM3_H__
#define __KOAL_SM3_H__

#ifdef __cplusplus
extern "C" {
#endif

#define SM3_DIGEST_LENGTH 32

typedef unsigned int sm3_word_t;

typedef struct sm3_ctx_t {
	int m_size;							/*	�ܳ���(�ֽ�)			*/
	
	unsigned char remain[128];			/*	ʣ�������(�鳤�ȵ�2��)	*/
	unsigned int r_len;					/*	ʣ�����ݳ���(�ֽ�)		*/

	sm3_word_t iv[8];					/*	�м���̵�IVֵ			*/
	
} SM3_CTX;

int sm3_init(SM3_CTX *ctx);
int sm3_update(SM3_CTX *ctx, const unsigned char *m, unsigned int m_bytes);
int sm3_final(SM3_CTX *ctx, unsigned char *dgst);

int sm3_hash_simple(const unsigned char *m, int m_bytes, unsigned char *dgst);

int sm3_hash(const unsigned char* pBuffer, unsigned uLen, unsigned char *dgst);

int sm3(const unsigned char* pBuffer, unsigned uLen, unsigned char *dgst);

#ifdef __cplusplus
}
#endif

#endif /* __KOAL_SM3_H__	*/
