#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <cysec.h>
#include "test_util.h"

static void test_detached_without_attrs(void)
{
	const char* p[] = { "rsa", "sm2", "ecc"};
	int ret = 0;
	unsigned int n=0;
	
	for(n = 0; n < sizeof(p)/sizeof(char*); n ++){
		char path[256] = {0};
		unsigned char *plain = NULL, *p7 = NULL;
		size_t plen = 0, p7len = 0;
		X509CRT_PCTX x509 = NULL;

		//��ȡpkcs7��ʽ�����֤�飨sm2�㷨�޷���ȡ�ɹ���
		/**
			��ȡ����(��rsa����Ϊ��)��
			openssl pkcs7 -inform der -in rsa.pkcs7.detached.without.attrs.der -outform pem -out pkcs7.raw.rsa.pem
			openssl pkcs7 -in pkcs7.raw.rsa.pem -print_certs -out pkcs7.rsa.cert.pem
			openssl x509 -in pkcs7.rsa.cert.pem -text -noout
		 */
		snprintf(path, sizeof(path), "./kpool/%s.pkcs7.detached.without.attrs.der", p[n]);		
		p7 = FILE_getcontent(path, &p7len);
		s_assert((p7 != NULL), "Failed to load pkcs7 (%s)", path);
		//��ȡ��������
		snprintf(path, sizeof(path), "./kpool/%s.pkcs7.data.der", p[n]);		
		plain = FILE_getcontent(path, &plen);
		s_assert((plain != NULL), "Failed to load data (%s)", path);

		/** remove '\n' */
		if(plain[plen-1] == '\n'){
			plen -- ;
		}
		//��֤pkcs7����ԭ��ǩ��
		ret = cysec_pkcs7_detached_verify(plain, plen, p7, p7len, &x509);
		s_assert((ret == 0), "verify error... %08X\n", ret);

		if(x509){
			dumpcrt(x509);
			x509crt_free(x509);
		}

		SAFE_FREE(plain);
		SAFE_FREE(p7);		
	}	
}

static void test_detached_with_attrs(void)
{
	const char* p[] = { "rsa", "sm2", "ecc"};
	int ret = 0;
	unsigned int n=0;

	for(n = 0; n < sizeof(p)/sizeof(char*); n ++){
		char path[256] = {0};
		unsigned char *plain = NULL, *p7 = NULL;
		size_t plen = 0, p7len = 0;
		X509CRT_PCTX x509 = NULL;

		/**
			��ȡ����(��rsa����Ϊ��)��
			openssl pkcs7 -inform der -in rsa.pkcs7.detached.with.attrs.der -outform pem -out pkcs7.raw.rsa.attrs.pem
			openssl pkcs7 -in pkcs7.raw.rsa.attrs.pem -print_certs -out pkcs7.rsa.cert.attrs.pem
			openssl x509 -in pkcs7.rsa.cert.attrs.pem -text -noout
		 */
		snprintf(path, sizeof(path), "./kpool/%s.pkcs7.detached.with.attrs.der", p[n]);		
		p7 = FILE_getcontent(path, &p7len);
		s_assert((p7 != NULL), "Failed to load pkcs7 (%s)", path);

		snprintf(path, sizeof(path), "./kpool/%s.pkcs7.data.der", p[n]);		
		plain = FILE_getcontent(path, &plen);
		s_assert((plain != NULL), "Failed to load data (%s)", path);

		/** remove '\n' */
		if(plain[plen-1] == '\n'){
			plen -- ;
		}

		ret = cysec_pkcs7_detached_verify(plain, plen, p7, p7len, &x509);
		s_assert((ret == 0), "verify error... %08X\n", ret);

		if(x509){
			dumpcrt(x509);
			x509crt_free(x509);
		}

		SAFE_FREE(plain);
		SAFE_FREE(p7);		
	}	
}

/**
	���ܣ���֤pkcs7��ʽ֤�鲻��ԭ��ǩ��
	1.��ȡpkcs7֤������
	2.��ȡԭ�����ݣ����Ǵ�pkcs7�л�ã�
	3.��֤pkcs7����ԭ��ǩ��
 */
static void test_detached(void)
{
	test_detached_with_attrs();
	test_detached_without_attrs();
}

static void test_attached_without_attrs(void)
{
	const char* p[] = { "rsa", "sm2", "ecc"};
	int ret = 0;
	unsigned int n=0;

	for(n = 0; n < sizeof(p)/sizeof(char*); n ++){
		char path[256] = {0};
		unsigned char *p7 = NULL;
		size_t p7len = 0;
		X509CRT_PCTX x509 = NULL;

		/**
			��ȡ����(��rsa����Ϊ��)��
			openssl pkcs7 -inform der -in rsa.pkcs7.attached.without.attrs.der -outform pem -out pkcs7.raw.rsa.attached.pem
			openssl pkcs7 -in pkcs7.raw.rsa.attached.pem -print_certs -out pkcs7.rsa.cert.attached.pem
			openssl x509 -in pkcs7.rsa.cert.attached.pem -text -noout
		 */
		snprintf(path, sizeof(path), "./kpool/%s.pkcs7.attached.without.attrs.der", p[n]);		
		p7 = FILE_getcontent(path, &p7len);
		s_assert((p7 != NULL), "Failed to load pkcs7 (%s)", path);
		//��֤pkcs7��ԭ��ǩ��
		ret = cysec_pkcs7_attached_verify(p7, p7len, &x509);
		s_assert((ret == 0), "verify error... %08X\n", ret);

		if(x509){
			dumpcrt(x509);
			x509crt_free(x509);
		}

		SAFE_FREE(p7);		
	}	
}

static void test_attached_with_attrs(void)
{
	const char* p[] = { "rsa", "sm2", "ecc"};
	int ret = 0;
	unsigned int n=0;

	for(n = 0; n < sizeof(p)/sizeof(char*); n ++){
		char path[256] = {0};
		unsigned char *p7 = NULL;
		size_t p7len = 0;
		X509CRT_PCTX x509 = NULL;

		snprintf(path, sizeof(path), "./kpool/%s.pkcs7.attached.with.attrs.der", p[n]);		
		p7 = FILE_getcontent(path, &p7len);
		s_assert((p7 != NULL), "Failed to load pkcs7 (%s)", path);

		ret = cysec_pkcs7_attached_verify(p7, p7len, &x509);
		s_assert((ret == 0), "verify error... %08X\n", ret);

		if(x509){
			dumpcrt(x509);
			x509crt_free(x509);
		}

		SAFE_FREE(p7);		
	}	
}

/**
	���ܣ���֤pkcs7��ʽ֤���ԭ��ǩ��
	1.��ȡpkcs7֤������
	2.��֤pkcs7����ԭ��ǩ��
 */
static void test_attached(void )
{
	test_attached_with_attrs();
	test_attached_without_attrs();
}

/**
	���ܣ�Alice���Լ������֤��ʹ������ǩ�����͸�Bob��������ͨ��˫��֮�䣬Alice��Bob�������ݣ�AliceΪ�˱�֤�Լ����͵����ݾ��б����ԣ������Ժ���ʵ�ԣ�
	Alice������ݽ�������ǩ�������������Ϊ����ǩ������Ϊ��ϢժҪ����Щ������ǩ������ϢժҪֻ�ǲ�ͬ�����µ�ͬһ��Ӧ��
	
	������Ĳ����У�Alice��signer��Bob��recipient
	
	��ʱ������Ϊ��
	����������ܣ��Ĺ�ϣ��֮��ʹ��˽Կ����������ǩ��
	����������ܣ��Ĺ�ϣ������ϢժҪ
 */
static void test_seal(void)
{
	const char *p[] = { "rsa", "sm2", "ecc" };
	int ret = 0;
	unsigned int n=0;

	for( n = 0; n < sizeof(p)/sizeof(char *); n ++ )
	{
		char path[256] = {0};
		X509CRT_PCTX recip_x509 = NULL;
		PKEY_PCTX signer_pkey = NULL;
		X509CRT_PCTX signer_x509 = NULL;
		unsigned char buf[4] = "123";
		size_t blen = sizeof(buf);
		unsigned char *seal = NULL;
		size_t slen = 0;
		//��ȡBob�����֤��
		snprintf(path, sizeof(path), "./kpool/%s.pkcs7.recipient.crt.pem", p[n]);
		recip_x509 = FILE_getcrt(path);
		s_assert((recip_x509 != NULL), "load recipient certificate %s\n error", path);
		if(!recip_x509)
			goto freebuffer;
		//��ȡAlice˽Կ
		memset(path, 0, sizeof(path));
		snprintf(path, sizeof(path), "./kpool/%s.pkcs7.signer.pvk.pem", p[n]);
		signer_pkey = FILE_getpvk(path);
		s_assert((signer_pkey != NULL), "load signer prviatekey %s\n error", path);
		if(!signer_pkey)
			goto freebuffer;
		//��ȡAlice���֤��
		memset(path, 0, sizeof(path));
		snprintf(path, sizeof(path), "./kpool/%s.pkcs7.signer.crt.pem", p[n]);
		signer_x509 = FILE_getcrt(path);
		s_assert((signer_x509 != NULL), "load signer certificate %s\n error", path);
		if(!signer_x509)
			goto freebuffer;
		/**
			(Alice��Bob��Ϊͨ��˫�����ƽ��)
			1.Alice�����ģ�buf��ʹ��Bob�Ĺ�Կ���ܣ���Կ������Bob�����֤���У�recip_x509��
			2.Alice���������ɹ�ϣ��Bob���Լ�֧�ֵĹ�ϣ�㷨�����֤��recip_x509�У�Alice����Bob�ṩ�Ĺ�ϣ�������й�ϣ��
			3.Aliceʹ���Լ���˽Կ���ܹ�ϣ�γ���ϢժҪ
			4.Alice���Լ������֤��+����+��ϢժҪ����һ���γ�pkcs7
			5.Alice����װ�õ�pkcs7��ʽ���ݷ��͸�Bob
			
			pkcs7��Ϊһ�ּ�����Ϣ���﷨��׼�����������������ݼ��ܣ�Ҳ����������ǩ��
			
			(Alice��ΪBob�ķ�֤�ߴ���)  ����������
			����Alice��һ��CA��Ҫ��Bob����һ�����֤�飬�������£�
			1.Bob���Լ��������Ϣʹ���Լ���˽Կ���ܺ󴫸�Alice
			2.Aliceȷ��Bob�����Ϣ�������Ĺ�ϣ
			3.Alice�����ȥ�Լ���Ԫ���ݣ�Ը��Ļ�����֮������ϣ��Ԫ���ݣ�һ��ʹ��base64��
			4.Aliceʹ���Լ���˽Կ���ܱ��룬�γ�����ǩ��
			5.Alice�������õĴ�������ǩ����֤�鷢�͸�Bob
		 */
		 /**
			��ȡ����(��rsa����Ϊ��)��
			openssl pkcs7 -in rsa.pkcs7.seal.pem -print_certs -out pkcs7.rsa.cert.pem
			openssl x509 -in pkcs7.rsa.cert.pem -text -noout
		 */
		ret = cysec_pkcs7_SignedAndEnveloped_seal(buf, blen, recip_x509, signer_pkey, signer_x509, &seal, &slen, PEM);
		if(ret)
			goto freebuffer;

		memset(path, 0, sizeof(path));
		snprintf(path, sizeof(path), "./kpool/%s.pkcs7.seal.pem", p[n]);
		ret = FILE_putcontent(seal,slen, path);
		if(ret)
			goto freebuffer;

		SAFE_FREE(seal);
		ret = cysec_pkcs7_SignedAndEnveloped_seal(buf, blen, recip_x509, signer_pkey, signer_x509, &seal, &slen, DER);
		if(ret)
			goto freebuffer;

		memset(path, 0, sizeof(path));
		snprintf(path, sizeof(path), "./kpool/%s.pkcs7.seal.der", p[n]);
		ret = FILE_putcontent(seal, slen, path);
		if(ret)
			goto freebuffer;

		SAFE_FREE(seal);
	freebuffer:
		SAFE_FREE(seal);
		if(recip_x509)
			cysec_x509crt_free(recip_x509);
		if(signer_x509)
			cysec_x509crt_free(signer_x509);
		if(signer_pkey)
			cysec_pkey_free(signer_pkey);
	}
}

static void test_open(void)
{
	const char *p[] = { "rsa", "sm2", "ecc" };
	int ret = 0;
	unsigned int n=0;

	for( n = 0; n < sizeof(p)/sizeof(char *); n ++ )
	{
		char path[256] = {0};
		X509CRT_PCTX recip_x509 = NULL;
		PKEY_PCTX recip_pkey = NULL;
		unsigned char *seal_pem = NULL, *seal_der = NULL;
		size_t seal_der_len = 0;
		unsigned char *plain_pem = NULL, *plain_der = NULL;
		size_t  plain_der_len = 0;
		CERTMGR_PCTX cm = NULL;
		X509CRT_PCTX signer_x509 = NULL,cacert = NULL;
		//��ȡBob�����֤��
		snprintf(path, sizeof(path), "./kpool/%s.pkcs7.recipient.crt.pem", p[n]);
		recip_x509 = FILE_getcrt(path);
		s_assert((recip_x509 != NULL), "load recipient certificate %s\n error", path);
		if(!recip_x509)
			goto freebuffer;
		//��ȡBob��˽Կ
		memset(path, 0, sizeof(path));
		snprintf(path, sizeof(path), "./kpool/%s.pkcs7.recipient.pvk.pem", p[n]);
		recip_pkey = FILE_getpvk(path);
		s_assert((recip_pkey != NULL), "load recipient prviatekey %s\n error", path);
		if(!recip_pkey)
			goto freebuffer;
		//��ȡ����ca�����֤��(Alice��Bob�����֤�鶼�ɶ���caǩ��)
		memset(path, 0, sizeof(path));
		snprintf(path, sizeof(path), "./kpool/%s.pkcs7.cacrt.pem", p[n]);
		cacert = FILE_getcrt(path);
		s_assert((cacert != NULL), "load signer certificate %s\n error", path);
		if(!cacert)
			goto freebuffer;			

		cm = cysec_certmgr_new();
		if(!cm)
			goto freebuffer;
		//����CA֤����ӵ�֤���������
		ret = cysec_certmgr_add_ca(cm, cacert);
		if(ret)
			goto freebuffer;

		/**
			��ȡ����(��rsa����Ϊ��)��
			openssl pkcs7 -inform der -in rsa.pkcs7.seal.der -print_certs -out pkcs7.rsa.seal.pem
			openssl x509 -in pkcs7.rsa.seal.pem -text -noout
		 */
		memset(path, 0, sizeof(path));
		snprintf(path, sizeof(path), "./kpool/%s.pkcs7.seal.der", p[n]);		//rsa.pkcs7.seal.der����Alice�����֤��
		seal_der = FILE_getcontent(path, &seal_der_len);
		if(!seal_der)
			goto freebuffer;
		/**
			1.Bob��pkcs7�л�ȡAlice�����֤��(signer_x509)
			2.Bobʹ��Alice�Ĺ�Կ������ϢժҪ����ԭ��ϣ
			3.Bob���¹�ϣ���ģ������¹�ϣ���뻹ԭ�Ĺ�ϣ�ȶԣ�2��3������ͬȷ����Ϣ�����Ժ���ʵ��
			4.Bobʹ���Լ���˽Կ��ԭ���ģ�ȷ����Ϣ�ı�����
		 */
		ret = cysec_pkcs7_SignedAndEnveloped_open(seal_der, seal_der_len, recip_x509, recip_pkey, &plain_der, &plain_der_len, &signer_x509);
		s_assert( (ret == 0),"open enveloped error %08x\n",ret);
		if(ret)
			goto freebuffer;

		if(!signer_x509){
			printf("can't found signer certificate.\n");
			goto freebuffer;
		}
		/**
			������ʵ�Ѿ���֤��Alice����ݣ�����֤�������Ӧ��ֻ����֤Alice��֤���Ƿ���Ч����������֤������
		 */
		ret = cysec_certmgr_verify(cm, signer_x509);
		s_assert(( ret == 0), "verify Chain error %08x\n",ret);
		if(ret)
			goto freebuffer;

		printf("plain_der (%s)\n", plain_der);
		SAFE_FREE(plain_der);

	freebuffer:
		SAFE_FREE(seal_pem);
		SAFE_FREE(seal_der);
		SAFE_FREE(plain_pem);
		SAFE_FREE(plain_der);
		if(cacert)
			cysec_x509crt_free(cacert);

		if(recip_x509)
			cysec_x509crt_free(recip_x509);

		if(signer_x509)
			cysec_x509crt_free(signer_x509);

		if(recip_pkey)
			cysec_pkey_free(recip_pkey);

		if(cm)
			cysec_certmgr_free(cm);		
	}
}

static void test_pkcs7_sign_ex(const char *save_file_path, int flags)
{
	const char *p[] = { "rsa", "sm2", "ecc" };
	int ret = 0;
	unsigned int n=0;

	for( n = 0; n < sizeof(p)/sizeof(char *); n ++ )
	{
		char path[256] = {0};
		PKEY_PCTX signer_pkey = NULL;
		X509CRT_PCTX signer_x509 = NULL;
		unsigned char buf[4] = "123";
		size_t blen = sizeof(buf);
		unsigned char *seal = NULL;
		size_t slen = 0;
		//��ȡAlice˽Կ
		memset(path, 0, sizeof(path));
		snprintf(path, sizeof(path), "./kpool/%s.pkcs7.signer.pvk.pem", p[n]);
		signer_pkey = FILE_getpvk(path);
		s_assert((signer_pkey != NULL), "load signer prviatekey %s\n error", path);
		if(!signer_pkey)
			goto freebuffer;
		//��ȡAlice���֤��
		memset(path, 0, sizeof(path));
		snprintf(path, sizeof(path), "./kpool/%s.pkcs7.signer.crt.pem", p[n]);
		signer_x509 = FILE_getcrt(path);
		s_assert((signer_x509 != NULL), "load signer certificate %s\n error", path);
		if(!signer_x509)
			goto freebuffer;			
		/**
			pkcs7��Ϊһ�ּ�����Ϣ���﷨��׼�����������������ݼ��ܣ�Ҳ����������ǩ��
			
			(Alice��ΪBob�ķ�֤�ߴ���)
			����Alice��һ��CA��Ҫ��Bob����һ�����֤�飬�������£�
			1.Bob���Լ��������Ϣʹ���Լ���˽Կ���ܺ󴫸�Alice
			2.Aliceȷ��Bob�����Ϣ�������Ĺ�ϣ
			3.Alice�����ȥ�Լ���Ԫ���ݣ�Ը��Ļ�����֮������ϣ��Ԫ���ݣ�һ��ʹ��base64��
			4.Aliceʹ���Լ���˽Կ���ܱ��룬�γ�����ǩ��
			5.Alice�������õĴ�������ǩ����֤�鷢�͸�Bob
			
			������û�ж����֤���������ǩ����ֻ�Ƕ����ݽ�������ǩ������Ϊ��ϢժҪ����Щ
			1.Aliceʹ���Լ���˽Կ�����ݣ�buf�����м����γ�α��ϢժҪ���ϸ���������˵�����ﲢ������ϢժҪ��
			2.Alice���Լ������֤���α��ϢժҪһ�𣬷�װ��pkcs7(seal)
		 */
		ret = cysec_pkcs7_sign(buf, blen, signer_pkey, signer_x509, flags, &seal, &slen, PEM);
		if(ret)
			goto freebuffer;
		
		memset(path, 0, sizeof(path));
		snprintf(path, sizeof(path), "./kpool/%s.pkcs7.sign.%s.pem", p[n], save_file_path);
		ret = FILE_putcontent(seal,slen, path);
		if(ret)
			goto freebuffer;
		
		SAFE_FREE(seal);
		ret = cysec_pkcs7_sign(buf, blen, signer_pkey, signer_x509, flags, &seal, &slen, DER);
		if(ret)
			goto freebuffer;

		memset(path, 0, sizeof(path));
		snprintf(path, sizeof(path), "./kpool/%s.pkcs7.sign.%s.der", p[n], save_file_path);
		ret = FILE_putcontent(seal, slen, path);
		if(ret)
			goto freebuffer;

		SAFE_FREE(seal);
	freebuffer:
		SAFE_FREE(seal);
		if(signer_x509)
			cysec_x509crt_free(signer_x509);
		if(signer_pkey)
			cysec_pkey_free(signer_pkey);
	}
}

static void test_pkcs7_sign_verify_ex(const char *save_file_path, int flags)
{
	const char *p[] = { "rsa", "sm2", "ecc" };
	int ret = 0;
	unsigned int n=0;

	for( n = 0; n < sizeof(p)/sizeof(char *); n ++ )
	{
		char path[256] = {0};
		X509CRT_PCTX cacert = NULL;
		unsigned char *seal_der = NULL;
		size_t seal_der_len = 0;
		CERTMGR_PCTX cm = NULL;
		X509CRT_PCTX signer_x509 = NULL;
		unsigned char buf[4] = "123";
		size_t blen = sizeof(buf);
		
		//��ȡ����ca֤��
		memset(path, 0, sizeof(path));
		snprintf(path, sizeof(path), "./kpool/%s.pkcs7.cacrt.pem", p[n]);
		cacert = FILE_getcrt(path);
		s_assert((cacert != NULL), "load signer certificate %s\n error", path);
		if(!cacert)
			goto freebuffer;			
		//����֤�������
		cm = cysec_certmgr_new();
		if(!cm)
			goto freebuffer;
		//����ca֤�����֤���������
		ret = cysec_certmgr_add_ca(cm, cacert);
		if(ret)
			goto freebuffer;
		//��ȡpkcs7��ʽ����
		memset(path, 0, sizeof(path));
		// snprintf(path, sizeof(path), "./kpool/%s.pkcs7.sign.der", p[n]);
		snprintf(path, sizeof(path), "./kpool/%s.pkcs7.sign.%s.pem", p[n], save_file_path);
		seal_der = FILE_getcontent(path, &seal_der_len);
		if(!seal_der)
			goto freebuffer;
		//��pkcs7��ʽ���ݽ�����֤��������ǩ���ߣ�Alice = signer_x509�����֤��
		if (flags & CYSEC_PKCS7_FLAG_DETACHED )
		{
			ret = cysec_pkcs7_detached_verify(buf, blen, seal_der, seal_der_len, &signer_x509);
		} else {
			ret = cysec_pkcs7_attached_verify(seal_der, seal_der_len, &signer_x509);
		}
		s_assert( (ret == 0),"open enveloped error %08x\n",ret);
		if(ret)
			goto freebuffer;

		if(!signer_x509){
			printf("can't found signer certificate.\n");
			goto freebuffer;
		}
		//��֤Alice�Ƿ����������֤�����Ƿ������ȵ�
		ret = cysec_certmgr_verify(cm, signer_x509);
		s_assert(( ret == 0), "verify Chain error %08x\n",ret);
		if(ret)
			goto freebuffer;

		printf("plain_der (%s)\n", buf);

	freebuffer:
		SAFE_FREE(seal_der);
		if(cacert)
			cysec_x509crt_free(cacert);

		if(signer_x509)
			cysec_x509crt_free(signer_x509);

		if(cm)
			cysec_certmgr_free(cm);		
	}
}

static void test_pkcs7_sign_detached_without_attrs(void)
{
	int flags = CYSEC_PKCS7_FLAG_DETACHED | CYSEC_PKCS7_FLAG_WITHOUT_ATTRIBUTES;
	const char *path = "detached_without_attrs";

	test_pkcs7_sign_ex(path, flags);
	test_pkcs7_sign_verify_ex(path, flags);
}

static void test_pkcs7_sign_detached_with_attrs(void)
{
	int flags = CYSEC_PKCS7_FLAG_DETACHED | CYSEC_PKCS7_FLAG_WITH_ATTRIBUTES;
	const char *path = "detached_with_attrs";

	test_pkcs7_sign_ex(path, flags);
	test_pkcs7_sign_verify_ex(path, flags);
}

static void test_pkcs7_sign_attached_without_attrs(void)
{
	int flags = CYSEC_PKCS7_FLAG_ATTACHED | CYSEC_PKCS7_FLAG_WITHOUT_ATTRIBUTES;
	const char *path = "attached_without_attrs";

	test_pkcs7_sign_ex(path, flags);
	test_pkcs7_sign_verify_ex(path, flags);
}

static void test_pkcs7_sign_attached_with_attrs(void)
{
	int flags = CYSEC_PKCS7_FLAG_ATTACHED | CYSEC_PKCS7_FLAG_WITH_ATTRIBUTES;
	const char *path = "attached_with_attrs";

	test_pkcs7_sign_ex(path, flags);
	test_pkcs7_sign_verify_ex(path, flags);
}

static void test_pkcs7_sign(void)
{
	test_pkcs7_sign_detached_without_attrs();
	test_pkcs7_sign_detached_with_attrs();
	test_pkcs7_sign_attached_without_attrs();
	test_pkcs7_sign_attached_with_attrs();
}

static void test(void)
{
	test_detached();
	test_attached();
	test_seal();
	test_open();
	test_pkcs7_sign();
}

int main(void)
{
	test();
	exit(0);
}