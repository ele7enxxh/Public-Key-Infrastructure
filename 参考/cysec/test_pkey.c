#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <cysec.h>
#include "test_util.h"

/**
	����rsa˽Կ�Ƿ�ɹ�
	rsa����ʱ��benchmark����
 */
static void test_pkey_gen_rsa(void)
{
#ifdef CONFIG_X86_64
	int rsabits[] = {512, 1024, 2048, 4096};
#else
	int rsabits[] = {512, 1024};
#endif
	unsigned int n;
	PKEY_PCTX pkey;

	for (n = 0; n < sizeof(rsabits)/sizeof(int); n ++) {
		printf("generating rsa key %d...", rsabits[n]);
		pkey = cysec_pkey_gen_rsa(rsabits[n]);
		printf(" %s\n", pkey ? "ok":"failed");
		if (pkey) printf("keysize=[%d] ", cysec_pkey_get_bits(pkey));
		if(pkey)
			cysec_pkey_free(pkey);
		benchmark(1);
		while (_bm.loop) {
			pkey = cysec_pkey_gen_rsa(rsabits[n]);
			if(pkey)
				cysec_pkey_free(pkey);			
			_bm.round ++;
		}
		benchmark(0);
		printf("round=[%d] time=[%fs] [%.2f]/s\n", _bm.round, _bm.e, _bm.round/_bm.e);
		printf("\n");
	}
}

/**
	����ecc˽Կ�Ƿ�ɹ�
	ecc����ʱ��benchmark����
 */
static void test_pkey_gen_ecc(void)
{
	int curves[] = {ECC_CURVE_SECP256R1, ECC_CURVE_SECP384R1, ECC_CURVE_SECP521R1, ECC_CURVE_SM2};
	const char *curves_name[] = {"secp256r1","secp384r1","secp521r1","sm2"};
	unsigned int n;
	PKEY_PCTX pkey;

	for (n = 0; n < sizeof(curves)/sizeof(int); n ++) {
		printf("generating ecc key %s...", curves_name[n]);
		pkey = cysec_pkey_gen_ecc(curves[n]);
		printf(" %s\n", pkey ? "ok":"failed");
		if (pkey) printf("keysize=[%d] ", cysec_pkey_get_bits(pkey));
		if(pkey)
			cysec_pkey_free(pkey);

		benchmark(1);
		while (_bm.loop) {
			pkey = cysec_pkey_gen_ecc(curves[n]);
			if(pkey)
				cysec_pkey_free(pkey);			
			_bm.round ++;
		}
		benchmark(0);
		printf("round=[%d] time=[%fs] [%.2f]/s\n", _bm.round, _bm.e, _bm.round/_bm.e);
		printf("\n");
	}
}
/**
	����ecc˽Կ�Ƿ�ɹ�
	ecc����ʱ��benchmark����
 */
static void test_pkey_gen_ecc_by_name(void)
{
	int curves[] = {ECC_CURVE_SECP256R1, ECC_CURVE_SECP384R1, ECC_CURVE_SECP521R1, ECC_CURVE_SM2};
	const char *curves_name[] = {"secp256r1","secp384r1","secp521r1","sm2"};
	unsigned int n;
	PKEY_PCTX pkey;

	for (n = 0; n < sizeof(curves)/sizeof(int); n ++) {
		pkey = cysec_pkey_gen_ecc_by_name(curves_name[n]);
		printf(" %s\n", pkey ? "ok":"failed");
		printf("generated ecc key %s...ok\n", cysec_pkey_ecc_get_curve_name(pkey));
		if (pkey) printf("keysize=[%d] ", cysec_pkey_get_bits(pkey));
		if(pkey)
			cysec_pkey_free(pkey);

		benchmark(1);
		while (_bm.loop) {
			pkey = cysec_pkey_gen_ecc_by_name(curves_name[n]);
			if(pkey)
				cysec_pkey_free(pkey);			
			_bm.round ++;
		}
		benchmark(0);
		printf("round=[%d] time=[%fs] [%.2f]/s\n", _bm.round, _bm.e, _bm.round/_bm.e);
		printf("\n");
	}
}
/**
	���ܣ��ֱ�ʹ��rsa,ecc,sm2���ַǶԳƼ����㷨���мӽ��ܲ���
		1.��Կ���ܣ������Լ���ʱ��
		2.˽Կ���ܣ������Խ���ʱ��
 */
static void test_pkey_encdec_one(PKEY_PCTX pub, PKEY_PCTX pri)
{
	unsigned char plain[100];
	unsigned char enc[4096] = {0};
	unsigned char dec[4096] = {0};
	size_t elen = 4096;
	size_t dlen = 4096;
	int ret;
	struct timespec tpstart;
	struct timespec tpend;
	double timedif;

	fillbuf(plain, sizeof(plain));

	printf("public encrypt...\n");
	clock_gettime(CLOCK_REALTIME, &tpstart);
	ret = pkey_public_encrypt(pub, plain, sizeof(plain), enc, &elen);
	s_assert((elen > 0), "elen=%zu ret=%d", elen, ret);
	clock_gettime(CLOCK_REALTIME, &tpend);
	timedif = (tpend.tv_sec-tpstart.tv_sec)+(tpend.tv_nsec-tpstart.tv_nsec)/1000000000.0;
	printf("public encrypt time: %.12f\n", timedif);

	printf("private decrypt...\n");

	ret = pkey_private_decrypt(pri, enc, elen, dec, &dlen);
	s_assert((dlen > 0), "dlen=%zu ret=%08x", dlen, ret);
	clock_gettime(CLOCK_REALTIME, &tpstart);	
	s_assert((dlen == sizeof(plain)), "decrypt failed. %zu", dlen);
	s_assert((0 == memcmp(plain, dec, dlen)), "decrypt failed.");
	clock_gettime(CLOCK_REALTIME, &tpend);
	timedif = (tpend.tv_sec-tpstart.tv_sec)+(tpend.tv_nsec-tpstart.tv_nsec)/1000000000.0;
	printf("private decrypt time: %.12f\n", timedif);	
}
/**
	���ܣ��ֱ�ʹ��rsa,ecc,sm2���ֹ�Կ�����㷨�������ӦժҪ��ϣ�㷨�����ݽ�������ǩ��
	1.��ϣ���Ļ����ϢժҪ��˽Կ������ϢժҪ���������ǩ��
	2.ʹ�ù�Կ������ϢժҪ����ϣ���Ļ���µ���ϢժҪ���ȶ���ժҪ����ܵõ�����ϢժҪ�Ƿ���ͬ
	3.ֻ�ı�ԭ�����ݣ��ٽ���2�Ĳ��裬����֤ʧ��
	4.ֻ�ı�ǩ�����ٽ���2�Ĳ��裬����֤ʧ��
 */
static void test_pkey_sigvfy_one(PKEY_PCTX pub, PKEY_PCTX pri, HASH_ALG halgs)
{
	unsigned char plain[100] = {0};
	unsigned char sig[512] = {0};
	size_t slen = 512;
	int ret;
	struct timespec tpstart;
	struct timespec tpend;
	double timedif;

	fillbuf(plain, sizeof(plain));

	printf("sign.....\n");
	clock_gettime(CLOCK_REALTIME, &tpstart);
	//��ϣ���Ļ����ϢժҪ��˽Կ������ϢժҪ���������ǩ��
	ret = pkey_sign(pri, plain, sizeof(plain), halgs, sig, &slen);
	s_assert((slen > 0), "slen=%zu ret=%08x", slen, ret);
	clock_gettime(CLOCK_REALTIME, &tpend);
	timedif = (tpend.tv_sec-tpstart.tv_sec)+(tpend.tv_nsec-tpstart.tv_nsec)/1000000000.0;
	printf("sign: %.12f\n", timedif);

	printf("verify.....\n");
	clock_gettime(CLOCK_REALTIME, &tpstart);	
	//ʹ�ù�Կ������ϢժҪ����ϣ���Ļ���µ���ϢժҪ���ȶ���ժҪ����ܵõ�����ϢժҪ�Ƿ���ͬ
	ret = pkey_verify(pub, plain, sizeof(plain), halgs, sig, slen);
	s_assert((0 == ret), "ret=%08x", ret);
	clock_gettime(CLOCK_REALTIME, &tpend);
	timedif = (tpend.tv_sec-tpstart.tv_sec)+(tpend.tv_nsec-tpstart.tv_nsec)/1000000000.0;
	printf("verify: %.12f\n", timedif);	

	// change original data, verify should fail.
	plain[0] ++;
	ret = pkey_verify(pub, plain, sizeof(plain), halgs, sig, slen);
	s_assert((0 != ret), "ret=%08x", ret);
	
	// restore original data
	plain[0] --;
	// change signature, verify should fail.
	sig[0] ++;
	ret = pkey_verify(pub, plain, sizeof(plain), halgs, sig, slen);
	s_assert((0 != ret), "ret=%08x", ret);
}
/**
	����ϢժҪ��������ǩ�����ο�����
 */
static void test_pkey_digest_sigvfy_one(PKEY_PCTX pub, PKEY_PCTX pri, HASH_ALG halgs)
{
	unsigned char plain[100] = {0};
	unsigned char sig[512] = {0};
	size_t slen = 512;
	int ret;
	struct timespec tpstart;
	struct timespec tpend;
	double timedif;

	fillbuf(plain, sizeof(plain));

	printf("digest sign.....\n");
	clock_gettime(CLOCK_REALTIME, &tpstart);
	//����ϢժҪ��ϣ�����������ǩ��
	ret = cysec_pkey_digest_sign(pri, plain, sizeof(plain), halgs, sig, &slen);
	s_assert((slen > 0), "slen=%zu ret=%08x", slen, ret);
	clock_gettime(CLOCK_REALTIME, &tpend);
	timedif = (tpend.tv_sec-tpstart.tv_sec)+(tpend.tv_nsec-tpstart.tv_nsec)/1000000000.0;
	printf("digest sign: %.12f\n", timedif);

	printf("digest verify.....\n");
	clock_gettime(CLOCK_REALTIME, &tpstart);
	//����ǩ����֤
	ret = cysec_pkey_digest_verify(pub, plain, sizeof(plain), halgs, sig, slen);
	s_assert((0 == ret), "ret=%08x", ret);
	clock_gettime(CLOCK_REALTIME, &tpend);
	timedif = (tpend.tv_sec-tpstart.tv_sec)+(tpend.tv_nsec-tpstart.tv_nsec)/1000000000.0;
	printf("digest verify: %.12f\n", timedif);
	
	// change original data, verify should fail.
	plain[0] ++;
	ret = cysec_pkey_digest_verify(pub, plain, sizeof(plain), halgs, sig, slen);
	s_assert((0 != ret), "ret=%08x", ret);
	
	// restore original data
	plain[0] --;
	// change signature, verify should fail.
	sig[0] ++;
	ret = cysec_pkey_digest_verify(pub, plain, sizeof(plain), halgs, sig, slen);
	s_assert((0 != ret), "ret=%08x", ret);
}

static void test_pkey_encdec(void)
{
	const char *p[] = {"rsa", "ecc", "sm2"};
	char path[256] ={0} ;
	unsigned int n = 0;

	for(n = 0; n < sizeof(p)/sizeof(char *); n++ )
	{
		snprintf(path, sizeof(path), "%s/%s.pvk.pem", KPOOL_PATH, p[n]);
		PKEY_PCTX k = FILE_getpvk(path);
		snprintf(path, sizeof(path), "%s/%s.crt.pem", KPOOL_PATH, p[n]);
		X509CRT_PCTX c = FILE_getcrt(path);
		PKEY_PCTX u = x509crt_get_publickey(c);
		dumpkey(u);
		
		test_pkey_encdec_one(u, k);		

		pkey_free(u);
		pkey_free(k);
		x509crt_free(c);
		printf("\n");		
	} 
}

static void test_pkey_sigvfy(void)
{
	const char *p[] = {"rsa", "ecc", "sm2"};
	char path[256] ={0} ;
	int halgs[] = { HASH_ALG_SHA256, HASH_ALG_SHA384, HASH_ALG_ECDSA_SM2 };
	unsigned int n = 0;

	for(n = 0; n < sizeof(p)/sizeof(char *); n++ )
	{
		snprintf(path, sizeof(path), "%s/%s.pvk.pem", KPOOL_PATH, p[n]);
		PKEY_PCTX k = FILE_getpvk(path);
		snprintf(path, sizeof(path), "%s/%s.crt.pem", KPOOL_PATH, p[n]);
		X509CRT_PCTX c = FILE_getcrt(path);
		PKEY_PCTX u = x509crt_get_publickey(c);
		dumpkey(u);
		
		test_pkey_sigvfy_one(u, k, halgs[n]);		
		test_pkey_digest_sigvfy_one(u, k, halgs[n]);
		
		pkey_free(u);
		pkey_free(k);
		x509crt_free(c);
		printf("\n");		
	} 
}
/**
	�����ݽ���˽Կǩ������Կ��ǩ��benchmark����
	1.������ʹ��˽Կ����ǩ����������3s�ڿ���ǩ���Ĵ���
	2.������ʹ�ù�Կ������ǩ��������3s�ڿ�����ǩ�Ĵ���
 */
static void test_pkey_sigvfy_benchmark(void)
{
	const char* p[] = { "rsa", "ecc", "sm2" };
	int halgs[] = { HASH_ALG_SHA256, HASH_ALG_SHA384, HASH_ALG_ECDSA_SM2 };
	char path[256];
	unsigned int n;
	
	printf("sign/verify benchmark...\n");
	for (n = 0; n < sizeof(p)/sizeof(char *); n ++) {
		snprintf(path, sizeof(path), "%s/%s.pvk.pem", KPOOL_PATH, p[n]);
		PKEY_PCTX k = FILE_getpvk(path);
		snprintf(path, sizeof(path), "%s/%s.crt.pem", KPOOL_PATH, p[n]);
		X509CRT_PCTX c = FILE_getcrt(path);
		PKEY_PCTX u = x509crt_get_publickey(c);
		dumpkey(u);
		
		unsigned char plain[32];
		unsigned char sig[512];
		size_t slen;

		fillbuf(plain, sizeof(plain));
		benchmark(1);
		while (_bm.loop) {
			slen = sizeof(sig);
			pkey_sign(k, plain, sizeof(plain), halgs[n], sig, &slen);
			_bm.round ++;
		}
		benchmark(0);
		printf("sign\ttime=%fs round=%d  %.2f/s\n", _bm.e, _bm.round, _bm.round/_bm.e);
		
		benchmark(1);
		while (_bm.loop) {
			pkey_verify(u, plain, sizeof(plain), halgs[n], sig, slen);
			_bm.round ++;
		}
		benchmark(0);
		printf("verify\ttime=%fs round=%d  %.2f/s\n", _bm.e, _bm.round, _bm.round/_bm.e);

		pkey_free(u);
		pkey_free(k);
		x509crt_free(c);
		printf("\n");
	}	
}

/**
	��ϢժҪ��ǩ��ǩbenchmark���ԣ��ο�����
 */
static void test_pkey_digest_sigvfy_benchmark(void)
{
	const char* p[] = { "rsa", "ecc", "sm2" };
	int halgs[] = { HASH_ALG_SHA256, HASH_ALG_SHA384, HASH_ALG_ECDSA_SM2 };
	char path[256];
	unsigned int n;
	
	printf("digest sign/verify benchmark...\n");
	for (n = 0; n < sizeof(p)/sizeof(char *); n ++) {
		snprintf(path, sizeof(path), "%s/%s.pvk.pem", KPOOL_PATH, p[n]);
		PKEY_PCTX k = FILE_getpvk(path);
		snprintf(path, sizeof(path), "%s/%s.crt.pem", KPOOL_PATH, p[n]);
		X509CRT_PCTX c = FILE_getcrt(path);
		PKEY_PCTX u = x509crt_get_publickey(c);
		dumpkey(u);
		
		unsigned char plain[32];
		unsigned char sig[512];
		size_t slen;

		fillbuf(plain, sizeof(plain));
		benchmark(1);
		while (_bm.loop) {
			slen = sizeof(sig);
			cysec_pkey_digest_sign(k, plain, sizeof(plain), halgs[n], sig, &slen);
			_bm.round ++;
		}
		benchmark(0);
		printf("sign\ttime=%fs round=%d  %.2f/s\n", _bm.e, _bm.round, _bm.round/_bm.e);
		
		benchmark(1);
		while (_bm.loop) {
			cysec_pkey_digest_verify(u, plain, sizeof(plain), halgs[n], sig, slen);
			_bm.round ++;
		}
		benchmark(0);
		printf("verify\ttime=%fs round=%d  %.2f/s\n", _bm.e, _bm.round, _bm.round/_bm.e);

		pkey_free(u);
		pkey_free(k);
		x509crt_free(c);
		printf("\n");
	}	
}

/**
	�����ݽ��й�Կ���ܣ�˽Կ���ܵ�benchmark����
	1.������ʹ�ù�Կ���м��ܣ�������3s�ڿ��Լ��ܵĴ���
	2.������ʹ��˽Կ���н��ܣ�������3s�ڿ��Խ��ܵĴ���
 */
static void test_pkey_encdec_benchmark(void)
{
	const char* p[] = { "ecc", "sm2", "rsa" };
	char path[256];
	int ret = 0;
	unsigned int n = 0;

	printf("enc/dec benchmark...\n");
	for (n = 0; n < sizeof(p)/sizeof(char *); n ++) {
		snprintf(path, sizeof(path), "%s/%s.pvk.pem", KPOOL_PATH, p[n]);
		PKEY_PCTX k = FILE_getpvk(path);
		snprintf(path, sizeof(path), "%s/%s.crt.pem", KPOOL_PATH, p[n]);
		X509CRT_PCTX c = FILE_getcrt(path);
		PKEY_PCTX u = x509crt_get_publickey(c);
		dumpkey(u);
		
		unsigned char plain[128];
		unsigned char enc[512];
		unsigned char dec[512];
		size_t elen, dlen;
		elen = sizeof(enc);
		dlen = sizeof(dec);

		fillbuf(plain, sizeof(plain));
		benchmark(1);
		while (_bm.loop) {
			elen = sizeof(enc);
			ret = pkey_public_encrypt(u, plain, sizeof(plain), enc, &elen);
			if(ret){
				printf("public key encrypt error,%08x\n", ret);
				break;
			}
			_bm.round ++;
		}
		benchmark(0);
		printf("enc\ttime=%fs round=%d  %.2f/s\n", _bm.e, _bm.round, _bm.round/_bm.e);
		
		benchmark(1);
		while (_bm.loop) {
			dlen = sizeof(dec);
			ret = pkey_private_decrypt(k, enc, elen, dec, &dlen);
			if(ret){
				printf("private key decrypt error, %08x\n", ret);
				break;
			}
			_bm.round ++;
		}
		benchmark(0);
		printf("dec\ttime=%fs round=%d  %.2f/s\n", _bm.e, _bm.round, _bm.round/_bm.e);

		pkey_free(u);
		pkey_free(k);
		x509crt_free(c);
		printf("\n");
	}	
}

/**
	3sʱ����ʹ��rsa�㷨���ɵ�˽Կ�Ĵ���
	3sʱ����ʹ��ecc�㷨���ɵ�˽Կ�Ĵ���
	3sʱ����ʹ��sm2�㷨���ɵ�˽Կ�Ĵ���
 */
static void test_pkey_gen_benchmark(void)
{
	const char* p[] = { "rsa", "ecc", "sm2" };
#ifdef CONFIG_HUAWEI_ARMCORTEX_R4
	const unsigned int rsa_key_bits[] = {1024};
#else
	const unsigned int rsa_key_bits[] = {1024, 2048};
#endif
	const PKEY_ECC_CURVE ecc_curve_id[] = {ECC_CURVE_SECP256R1, ECC_CURVE_SECP384R1, ECC_CURVE_SECP521R1};
	const char *curves_name[] = {"secp256r1","secp384r1","secp512r1"};
	unsigned int n,m;
	
	printf("generate key benchmark...\n");
	for (n = 0; n < sizeof(p)/sizeof(char *); n ++) {
		if( strcmp(p[n],"rsa") == 0 )
		{
			for( m = 0; m < sizeof(rsa_key_bits)/sizeof(unsigned int); m++)
			{
				benchmark(1);
				while (_bm.loop) {
					PKEY_PCTX k = cysec_pkey_gen_rsa(rsa_key_bits[m]);
					if(!k)
						break;
					cysec_pkey_free(k);
					_bm.round ++;
				}
				benchmark(0);
				printf("generate rsa key(%d) \ttime=%fs round=%d  %.2f/s\n", rsa_key_bits[m], _bm.e, _bm.round, _bm.round/_bm.e);				
			}
		}

		if( strcmp(p[n],"ecc" ) == 0 )
		{
			for( m = 0; m < sizeof(ecc_curve_id)/sizeof(PKEY_ECC_CURVE); m++) 
			{
				benchmark(1);
				while (_bm.loop) {
					PKEY_PCTX k = cysec_pkey_gen_ecc(ecc_curve_id[m]);
					if(!k)
						break;
					cysec_pkey_free(k);
					_bm.round ++;
				}
				benchmark(0);
				printf("generate ecc key(%s) \ttime=%fs round=%d  %.2f/s\n", curves_name[m], _bm.e, _bm.round, _bm.round/_bm.e);				
			}
		}

		if( strcmp(p[n],"sm2" ) == 0 )
		{
			benchmark(1);
			while (_bm.loop) {
				PKEY_PCTX k = cysec_pkey_gen_sm2();
				if(!k)
					break;
				cysec_pkey_free(k);
				_bm.round ++;
			}
			benchmark(0);
			printf("generate sm2 key \ttime=%fs round=%d  %.2f/s\n", _bm.e, _bm.round, _bm.round/_bm.e);				
		}

		printf("\n");
	}	
}

/**
	1.����˽Կ��������Կ���ӻ��������칫Կ��˽Կ����
	2.�ӽ���ʱ�����
	3.����ǩ������
 */
static void test_pkey_import_export(void)
{
	const char* p[] = { "rsa", "sm2", "ecc" };
	int halgs[] = { HASH_ALG_SHA256, HASH_ALG_ECDSA_SM2, HASH_ALG_SHA384};
	int  ret = 0;
	unsigned int n = 0;

	for(n = 0; n < sizeof(p)/sizeof(char*); n ++){
		PKEY_PCTX pctx = NULL, pctx_pub = NULL, pctx_pri = NULL;
		unsigned char *pem_pub = NULL, *pem_prv = NULL;
		unsigned char *der_pub = NULL, *der_prv = NULL;
		size_t publen = 0, privlen =0;

		if( strcmp(p[n], "rsa") == 0 ){
			pctx = cysec_pkey_gen_rsa(1024);
			s_assert((pctx != NULL), "failure to generate rsa \n");
		}else if( strcmp(p[n], "sm2") == 0 ){
			pctx = cysec_pkey_gen_sm2();
			s_assert((pctx != NULL), "failure to generate sm2 \n");
		}else if (strcmp(p[n], "ecc") == 0 ){
			pctx = cysec_pkey_gen_ecc(ECC_CURVE_SECP256R1);
			s_assert((pctx != NULL), "failure to generate ecc \n");
		}

		ret = cysec_pkey_export_privatekey(pctx, &pem_prv, &privlen, PEM);
		s_assert((ret == 0),"ret =%d \n", ret);
		printf ("%d----------------------+++++%s\n",privlen, pem_prv);
		ret = cysec_pkey_export_publickey(pctx, &pem_pub, &publen, PEM);
		printf ("%d----------------------++++++00000000%s\n", publen,pem_pub);
		s_assert((ret == 0), "ret = %d \n", ret);

		pctx_pub = cysec_pkey_load_public(pem_pub, publen);
		s_assert((pctx_pub != NULL),"failure to load public key");

		pctx_pri = cysec_pkey_load_private(pem_prv, privlen, NULL);
		s_assert((pctx_pri != NULL)," failure to load private key");

		SAFE_FREE(pem_prv);
		SAFE_FREE(pem_pub);

		test_pkey_encdec_one(pctx_pub, pctx_pri);	
		test_pkey_sigvfy_one(pctx_pub, pctx_pri, halgs[n]);	

		ret = cysec_pkey_export_privatekey(pctx_pri, &der_prv, &privlen, DER);
		s_assert((ret == 0),"ret =%d \n", ret);

		ret = cysec_pkey_export_publickey(pctx_pub, &der_pub, &publen, DER);
		s_assert((ret == 0), "ret = %d \n", ret);	

		if(pctx_pri)
			pkey_free(pctx_pri);
		if(pctx_pub)
			pkey_free(pctx_pub);

		pctx_pub = cysec_pkey_load_public(der_pub, publen);
		s_assert((pctx_pub != NULL),"failure to load public key");

		pctx_pri = cysec_pkey_load_private(der_prv, privlen, NULL);
		s_assert((pctx_pri != NULL)," failure to load private key");

		SAFE_FREE(der_pub);
		SAFE_FREE(der_prv);
		test_pkey_encdec_one(pctx_pub, pctx_pri);	
		test_pkey_sigvfy_one(pctx_pub, pctx_pri, halgs[n]);		

		if(pctx_pri)
			pkey_free(pctx_pri);
		if(pctx_pub)
			pkey_free(pctx_pub);
		if(pctx)
			pkey_free(pctx);
	}
}

int main(void ) {
	test_pkey_encdec();
	test_pkey_sigvfy();
	test_pkey_import_export();
	test_pkey_encdec_benchmark();
	test_pkey_sigvfy_benchmark();
	test_pkey_digest_sigvfy_benchmark();
	test_pkey_gen_rsa();
	test_pkey_gen_ecc();
	test_pkey_gen_ecc_by_name();
	test_pkey_gen_benchmark();

	return 0;
}