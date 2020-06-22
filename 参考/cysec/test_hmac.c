#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <cysec.h>
#include "test_util.h"

/**
	���ܣ������ĵĹ�ϣ��ͨ����Կ�����γ���Ϣ��֤��
	1.����hmac�㷨���
	2.hmac�����ʼ��
	3.ִ��hmac����
	4.����hmac
	5.�ͷ�hamc�㷨���
 */
 
 	// HASH_ALG_AUTO = 0,     /** ����RSAǩ����ǩ�����ڲ�����㷨 */
	// HASH_ALG_SHA384 = 1,  /**< SHA384ժҪ�㷨 */
	// HASH_ALG_SM3    = 2,  /**< SM3ժҪ�㷨 */
	// HASH_ALG_SHA256 = 3,   /**< SHA256ժҪ�㷨 */
	// HASH_ALG_MD5 = 4,		/**< MD5ժҪ�㷨 */
	// HASH_ALG_ECDSA_SM2 = 5, /**< SM2ǩ��ժҪ�㷨��ֻ����SM2ǩ������ǩ���ã�������HMAC�� */
	// HASH_ALG_SHA1 = 6,		/**< SHA1ժҪ�㷨 */
	// HASH_ALG_SHA512 = 7 	/**< SHA512ժҪ�㷨 */
 
static void test_hmac(void) {
	int halgs[] = { HASH_ALG_SHA384, HASH_ALG_SM3, HASH_ALG_SHA256, HASH_ALG_MD5, HASH_ALG_ECDSA_SM2, HASH_ALG_SHA1, HASH_ALG_SHA512};	//��ϣ�㷨����
	const char* salgs[] = { "sha384", "sm3", "sha256", "md5", "sm2", "sha1", "sha512"};		
	int sizes[] = { 16, 64, 256, 1024, 4096 };		//���ĳ���
	HMAC_PCTX h;
	int n, m;
	unsigned char in[4096];			//��������
	unsigned char out[64];			//��ϣ�����Ӧ����16���ƣ�
	const char key[16];				//���ܹ�ϣʹ�õ���Կ��Ҳ��hashing-key

	for (n = 0; (unsigned int)n < sizeof(halgs)/sizeof(int); n ++) {
		for (m = 0; (unsigned int)m < sizeof(sizes)/sizeof(int); m ++) {
			printf("hmac(%d) name=[%s] buffersize=[%d] ", halgs[n], salgs[n], sizes[m]);
			int printflag = 1;
			benchmark(1);
			while (_bm.loop) {
				h = hmac_ctx_new(halgs[n]);
				if (printflag-- > 0)
					printf("hmacsize=[%d] ", hmac_size(h));
				hmac_init(h, (const unsigned char *)key, sizeof(key));
				hmac_update(h, in, sizes[m]);
				hmac_final(h, out);
				hmac_ctx_free(h);
				_bm.round ++;
			}
			benchmark(0);
			printf("round=[%d] time=[%fs] throughput=[%.2f]MB/s\n", _bm.round, _bm.e, _bm.round*sizes[m]/(_bm.e * 1000000));
		}
	}
}

int main(void) {
	test_hmac();
	return 0;
}
