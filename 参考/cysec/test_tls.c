#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <cysec.h>
#include "test_util.h"

#ifndef CYSEC_NO_TLS

/**
	��֤֤�����Ƿ������Ļص�
 */
static int tls_vcm_cb(X509CRT_PCTX crt, void *userdata){
	CERTMGR_PCTX cm = (CERTMGR_PCTX)userdata;
	int ret;

	if(!crt || !cm)
		return -1;

	printf("tls server certificate:\n");
	dumpcrt(crt);
	ret = cysec_certmgr_verify(cm, crt);
	s_assert((ret == 0), "Verify Certificate Chain Failure, ret = %08x\n", ret);
	
	return ret;
}

#if 0
/**
	����tls�ͻ����Ƿ�����tls���������ӳɹ�
 */
static void test_ssl_one(void) {
	const char* p[] = { "rsa", "sm2", "ecc" };
	char path[256];
	unsigned int n, m; 
	int ret;
	int num;
	PKEY_PCTX pvk = NULL;
	X509CRT_PCTX crt = NULL;
	const char* srvaddr = NULL;
	const char* req = "GET / HTTP/1.0\r\nConnection: close\r\n\r\n";		//http�����ַ���

	printf("tls client test ...\n");
	for (n = 0; n < sizeof(p)/sizeof(char*); n ++) {
		for (m = 0; m < 2; m ++) {
			TLS_CLIENT_PCTX ctx = tls_client_new();	

			tls_client_set_verify_callback(ctx, tls_vcb, NULL);
			if (m) {
				snprintf(path, sizeof(path), "%s/%s.ssl.pvk.pem", KPOOL_PATH, p[n]);
				pvk = FILE_getpvk(path);
				snprintf(path, sizeof(path), "%s/%s.ssl.crt.pem", KPOOL_PATH, p[n]);
				crt = FILE_getcrt(path);
				ret = tls_client_set_certificate(ctx, crt);
				s_assert((ret == 0), "ret=%08x", ret);
				ret = tls_client_set_private_key(ctx, pvk);
				s_assert((ret == 0), "ret=%08x", ret);
			}
			srvaddr = getenv("TLS_SERVER_ADDR") ? getenv("TLS_SERVER_ADDR") : "192.168.10.137";
			printf("connecting %s:%d \n", srvaddr, 443+n*2+m);
			ret = tls_client_connect(ctx, srvaddr, 443+n*2+m);
			s_assert((ret == 0), "ret=%08x", ret);
			num = tls_client_write(ctx, (const unsigned char *)req, strlen(req));
			s_assert((num == (int)strlen(req)), "num=%08x (%zu)", num, strlen(req));
			printf("connected.\n");
			tls_client_close(ctx);
			printf("closed. %d \n\n", 443+n*2+m);

			tls_client_free(ctx);
			
			if (m) {
				x509crt_free(crt);
				pkey_free(pvk);
			}
		}
	}
}
#endif

/**
	�ͻ�����������ķǰ�ȫ����benchmarkʱ����ԣ��ͻ��˲��Է������ݽ�����֤��
 */
static void test_ssl_benchmark_verify_none(const char *srvaddr, int port) {
	const char* req = "GET / HTTP/1.0\r\nConnection: close\r\n\r\n";
	int ret = 0 ;
	size_t reqlen = strlen(req);
	const char *addr = NULL;
	
	if(!srvaddr)
		addr = getenv("TLS_SERVER_ADDR") ? getenv("TLS_SERVER_ADDR") : "192.168.10.137";
	else
		addr = srvaddr;

	printf("using remote server %s:%d.\n", addr, port);
	
	//benchmark���Ӳ��ԣ�ÿ������������Ӷ�����һ���ͻ��ˣ�����֮�������Ͽ�����������3s�ڵĿͻ�����������������
	benchmark(1);
	while (_bm.loop) {
		//����tls�ͻ��˾��
		TLS_CLIENT_PCTX ctx = tls_client_new();
		//��tls�ͻ��ˣ����������������
		ret = tls_client_connect(ctx, addr, port);
		if (ret) {
			printf("connect failed. exit!(%08x)\n",ret);
			exit (-1);
		}
		//�ͻ��������˷�����������
		ret = tls_client_write(ctx, (const unsigned char *)req, reqlen);
		if (ret<=0) {
			printf("write req data failed. exit!\n");
			exit (-1);
		}
		//�ر������˵�����
		tls_client_close(ctx);
		//�ͷ�tls�ͻ���
		tls_client_free(ctx);
		_bm.round ++;
	}
	benchmark(0);

	printf("tls client\ttime=%fs round=%d  %.2f/s\n", _bm.e, _bm.round, _bm.round/_bm.e);
}

/**
	�ͻ�����������İ�ȫ����benchmarkʱ����ԣ��ͻ�����֤�������ݣ�������֤��
 */
static void test_ssl_benchmark_verify_required(const char *srvaddr, int port, CERTMGR_PCTX cm, X509CRT_PCTX localcrt, PKEY_PCTX localpvk) {
	const char* req = "GET / HTTP/1.0\r\nConnection: close\r\n\r\n";
	int ret = 0 ;
	size_t reqlen = strlen(req);
	const char *addr = NULL;

	if(!cm || !localcrt || !localpvk)
		return ;

	if(!srvaddr)
		addr = getenv("TLS_SERVER_ADDR") ? getenv("TLS_SERVER_ADDR") : "192.168.10.137";
	else
		addr = srvaddr;

	printf("using remote server %s:%d.\n", addr, port);
	
	benchmark(1);
	while (_bm.loop) {
		//����tls�ͻ��˾��
		TLS_CLIENT_PCTX ctx = tls_client_new();
		//�ͻ��˾��������֤�飨����������������֤�����Ҫ��
		ret = tls_client_set_certificate(ctx, localcrt);
		s_assert((ret == 0), "ret=%08x", ret);
		//�ͻ��˾����������Կ������������������֤�����Ҫ��
		ret = tls_client_set_private_key(ctx, localpvk);
		s_assert((ret == 0), "ret=%08x", ret);	
		//tls�ͻ��˻�÷�������֤��󣬽���������֤����ӽ�֤��������У�֮��ͻ��˵��ô˻ص���֤��������֤���֤����������
		tls_client_set_verify_callback(ctx, tls_vcm_cb, (void *)cm);
		//��tls�ͻ��ˣ����������������
		ret = tls_client_connect(ctx, addr, port);
		if (ret) {
			printf("connect failed. exit!(%08x)\n",ret);
			exit (-1);
		}
		//�ͻ��������˷�����������
		ret = tls_client_write(ctx, (const unsigned char *)req, reqlen);
		if (ret<=0) {
			printf("write req data failed. exit!\n");
			exit (-1);
		}
		//�ر������˵�����
		tls_client_close(ctx);
		//�ͷ�tls�ͻ���
		tls_client_free(ctx);
		_bm.round ++;
	}
	benchmark(0);

	printf("tls client\ttime=%fs round=%d  %.2f/s\n", _bm.e, _bm.round, _bm.round/_bm.e);

}

/**
	tls�ͻ��������ca������������Ϣ����������Լ������֤�飨�ӿͻ��������Լ������֤���˽Կ�²⣩
	������tls�ͻ��������������֮�������Ͽ�����û���������Ĺ�������������ͻ��˷���˽Կ����������Ӧ
	�ö���û��Ҫ��
 */
static int test_ssl_benchmark(void)
{
	const char* p[] = { "rsa", "sm2", "ecc" };
	char path[256];
	unsigned int n; 
	PKEY_PCTX pvk = NULL;
	X509CRT_PCTX crt = NULL;
	X509CRT_PCTX cacrt = NULL;
	X509CRT_PCTX rootcrt = NULL;
	CERTMGR_PCTX cm = NULL;
	const char* srvaddr = getenv("TLS_SERVER_ADDR") ? getenv("TLS_SERVER_ADDR") : "192.168.10.137";

	printf("tls client test ...\n");
	for (n = 0; n < sizeof(p)/sizeof(char*); n ++) {
		test_ssl_benchmark_verify_none(srvaddr, 443+n*2);
	}

	for (n = 0; n < sizeof(p)/sizeof(char*); n ++) {
		//ca����ǩ��֤��
		snprintf(path, sizeof(path), "%s/%s.ssl.rootcrt.pem", KPOOL_PATH, p[n] );
		rootcrt = FILE_getcrt(path);
		//����ca֤�飬������Ϊ�Ƿ�������֤��
		snprintf(path, sizeof(path), "%s/%s.ssl.cacrt.pem", KPOOL_PATH, p[n] );
		cacrt = FILE_getcrt(path);
		//tls�ͻ���˽Կ
		snprintf(path, sizeof(path), "%s/%s.ssl.pvk.pem", KPOOL_PATH, p[n]);
		pvk = FILE_getpvk(path);
		//tls�ͻ���˽Կ
		snprintf(path, sizeof(path), "%s/%s.ssl.crt.pem", KPOOL_PATH, p[n]);
		crt = FILE_getcrt(path);
		//����֤����
		cm = certmgr_new();
		certmgr_add_ca(cm, cacrt);
		certmgr_add_ca(cm, rootcrt);
		//����
		test_ssl_benchmark_verify_required(srvaddr, 443+n*2+1,cm,crt,pvk);	
		//�ͷ�
		pkey_free(pvk);
		x509crt_free(cacrt);
		x509crt_free(crt);
		x509crt_free(rootcrt);
		certmgr_free(cm);
	}

	return 0;
}

/**
	Bob��Alice����ͨ������ͨ��֮ǰBob��Ҫȷ��Alice���֤���Ƿ񱻵���������Bob��Alice�����֤�鷢�͸�ocsp����������Alice���������ȷ��.
	��ocsp�������᷵�ظ�Bobһ��ocsp��Ӧ���ݣ�֮��Bob��������Ļص���֤ocsp����ocsp��Ӧ�������֤
	�����Alice�Ƕ���ca֤����������Ǹ�Bobǩ�����֤��ķ�����
 */
/** for testing usage, don't use for product.*/
static int test_ocspstapling_callback(X509CRT_PCTX ctx, unsigned char *ocspresponse, int olen, void *userdata)
{
	CERTMGR_PCTX cm = (CERTMGR_PCTX)userdata;
	OCSP_RESPONSE_PCTX rsp_ctx = NULL;
	X509CRT_PCTX signer = NULL;	
	unsigned int ocspstatus = 0, ocsp_certstatus = 0;
	int ret = -1;

	if( !ctx || !ocspresponse || olen <= 0 )
		return -1;
	//Bob������Ӧ���ݹ���ocsp��Ӧ���
	ret = cysec_ocsprsp_decode(ocspresponse, olen, &rsp_ctx);
	s_assert((ret == 0), "recevice an invalid OCSP response, %08x\n",ret);
	if(ret != 0 ) goto err;;
	//��ȡocsp��Ӧ�����ǩ����֤�飬�����ǩ����֤�鼴Ϊocsp���������֤��
	//��ȡocsp��Ӧ�����ǩ����Ϣ��ocsp����������֤��Ϣʹ���Լ���˽Կǩ�����͸�Bob�������ǽ�ocsp�������Լ������֤�飨ǩ����֤�飩���͸�Bob��
	//�Դ�֤��Alice�����֤�����
	signer = cysec_ocsprsp_get_signer(rsp_ctx);
	//��Ϊ��ʱ�޷���ocsp���������ӣ�����ֻ��ͨ������ķ�������ȡ signer�����ǲ��Ե���Ҫ��ʵ���в�Ӧ������
	if(!signer)
		signer = cysec_certmgr_get_ocsprsp_signer(cm, rsp_ctx);	//kpool��Ҳû��ocsp��������֤�飬���Դ˴� test_tls.c ���޷���֤��
	if(!signer)
		goto err;
	//Bob�Ѿ���ocsp���������֤��Ĺ�Կ��ͨ���˹�Կ��֤��ǩ����֤��ȷ������ocsp����������ʱBobȷ��Alice֤���Ƿ�Ϸ������
	ret = cysec_ocsprsp_verify(rsp_ctx, signer);
	s_assert((ret == 0), "Verify Signature Failure, ret = %08x\n", ret);
	if(ret !=0 ) goto err;
	//ǩ����֤���֤�����Ƿ�����
	ret = cysec_certmgr_verify(cm, signer);
	s_assert((ret == 0), "Verify Certificate Chain Failure, ret = %08x\n", ret);
	if( ret != 0 ) goto err;
	//��ȡocsp��Ӧ��
	ret = cysec_ocsprsp_get_rspstatus(rsp_ctx, &ocspstatus);
	s_assert((ret == 0), "failed to get rsp status %08x\n",ret);
	printf("rspstatus is %d\n",ocspstatus);
	if(ret != 0 || ocspstatus != 0) goto err;
	//��ȡocsp��Ӧ֤��״̬��
	ret = cysec_ocsprsp_get_certstatus(rsp_ctx, ctx, cm, &ocsp_certstatus);
	s_assert((ret == 0), "failed to get cert status %08x\n", ret);
	printf("certstatus is %d\n", ocsp_certstatus);
	if(ret !=0 || ocsp_certstatus != 0) goto err;

	if(signer)
		x509crt_free(signer);
	if(rsp_ctx)
		cysec_ocsprsp_free(&rsp_ctx);
	return 0;
err:
	if(signer)
		x509crt_free(signer);
	if(rsp_ctx)
		cysec_ocsprsp_free(&rsp_ctx);

    return -(0x0000FFFF & ret);
}

/** ocsp stapling */
/**
	Bob��tls�ͻ��ˣ���Alice������ca֤�����������������Լ������֤�飬������ģʽ
 */
static void test_nonblocking_ssl_certstatus(void) {
	const char* p[] = { "rsa", "sm2", "ecc"};
	unsigned int n, m; 
	int ret;
	int num;
	const char* srvaddr = NULL;
	const char* req = "GET / HTTP/1.0\r\nConnection: close\r\n\r\n";

	printf("==================================\n");
	printf("tls client is testing (NONBLOCKING socket) ...\n");
	printf("==================================\n");
	for (n = 0; n < sizeof(p)/sizeof(char*);  n ++) {
		for (m = 0; m < 2; m ++) {
			CERTMGR_PCTX cm = NULL;
			X509CRT_PCTX cacrt = NULL;
			X509CRT_PCTX crt = NULL;
			X509CRT_PCTX rootcrt = NULL;
			PKEY_PCTX pvk = NULL;
			char path[256] = {0};
			TLS_CLIENT_PCTX ctx = NULL;
			
			printf("=========================================================\n");
			//��ȡtls�ͻ���֤��
			snprintf(path, sizeof(path), "./kpool/%s.ssl.crt.pem", p[n]);
			crt = FILE_getcrt(path);
			if(!crt)
				goto freebuffer;
			//��ȡca����ǩ��֤��
			printf("========(%s)========\n", path);
			snprintf(path, sizeof(path), "./kpool/%s.ssl.rootcrt.pem", p[n] );
			rootcrt = FILE_getcrt(path);
			if(!rootcrt)
				goto freebuffer;
			//��ȡ����ca֤��
			snprintf(path, sizeof(path), "./kpool/%s.ssl.cacrt.pem", p[n] );
			cacrt = FILE_getcrt(path);
			if(!cacrt)
				goto freebuffer;
			//��ȡ�ͻ���˽Կ
			snprintf(path, sizeof(path), "./kpool/%s.ssl.pvk.pem", p[n]);
			pvk = FILE_getpvk(path);
			if(!pvk)
				goto freebuffer;
			//����֤�������
			cm = certmgr_new();
			if(!cm)
				goto freebuffer;
			//��Ӷ���ca��������
			ret = certmgr_add_ca(cm, cacrt);
			s_assert((ret == 0), "ret=%08x\n", ret);
			if(ret)
				goto freebuffer;
			//��Ӹ�֤�鵽������
			ret = certmgr_add_ca(cm, rootcrt);
			s_assert((ret == 0), "ret=%08x\n", ret);
			if(ret)
				goto freebuffer;
			//����tls�ͻ���
			ctx = tls_client_new();
			if(!ctx)
				goto freebuffer;
			//Bob��ocsp����������һ����֤Alice֤��״̬��ocsp�����ocsp��������ocsp���������������󽫽����ocsp��Ӧ��ʽ���͸�Bob
			//Bob���ûص�����test_ocspstapling_callback�����ocsp��Ӧ
			cysec_tls_client_set_ocspstapling_callback(ctx, test_ocspstapling_callback, (void *)cm);
			//Bob��֤Alice���֤�����Ƿ�����
			tls_client_set_verify_callback(ctx, tls_vcm_cb, (void *)cm);
			//Bob���Լ������֤��
			ret = tls_client_set_certificate(ctx, crt);
			s_assert((ret == 0), "ret=%08x", ret);
			if(ret)
				goto freebuffer;
			//Bob���Լ���˽Կ
			ret = tls_client_set_private_key(ctx, pvk);
			s_assert((ret == 0), "ret=%08x", ret);
			if(ret)
				goto freebuffer;

			//Bob������Alice���ӣ�srvaddr��Alice��ַ
			srvaddr = getenv("TLS_SERVER_ADDR") ? getenv("TLS_SERVER_ADDR") : "192.168.10.137";
			printf("connecting %s:%d \n", srvaddr, 443+n*2+m);
			ret = tls_client_connect(ctx, srvaddr, 443+n*2+m);
			s_assert((ret == 0), "ret=%08x", ret);
			if(ret)
				goto freebuffer;
			//Bob��Alice��������
			num = tls_client_write(ctx, (const unsigned char *)req, strlen(req));
			s_assert((num == (int)strlen(req)), "num=%08x (%zu)", num, strlen(req));
			printf("connected.\n");
			//Bob�Ͽ���Alice������
			tls_client_close(ctx);
			printf("closed. %d \n\n", 443+n*2+m);

		freebuffer:
			if(ctx)
				tls_client_free(ctx);
			
			if(crt)
				x509crt_free(crt);

			if(pvk)
				pkey_free(pvk);

			if(cacrt)
				x509crt_free(cacrt);
			
			if(rootcrt)
				x509crt_free(rootcrt);
			
			if(cm)
				certmgr_free(cm);
			printf("=========================================================\n");

		}
	}
}

/** ocsp stapling */
/**
	Bob��tls�ͻ��ˣ���Alice������ca֤�����������������Լ������֤�飬����ģʽ
 */
static void test_blocking_ssl_certstatus(void) {
	const char* p[] = { "rsa", "sm2", "ecc"};
	unsigned int n, m; 
	int ret;
	int num;
	const char* srvaddr = NULL;
	const char* req = "GET / HTTP/1.0\r\nConnection: close\r\n\r\n";

	printf("==================================\n");
	printf("tls client is testing (BLOCKING socket) ...\n");
	printf("==================================\n");

	for (n = 0; n < sizeof(p)/sizeof(char*);  n ++) {
		for (m = 0; m < 2; m ++) {
			CERTMGR_PCTX cm = NULL;
			X509CRT_PCTX cacrt = NULL;
			X509CRT_PCTX crt = NULL;
			X509CRT_PCTX rootcrt = NULL;
			TLS_CLIENT_PCTX ctx =NULL;
			PKEY_PCTX pvk = NULL;
			char path[256] = {0};
			printf("=========================================================\n");
			snprintf(path, sizeof(path), "./kpool/%s.ssl.crt.pem", p[n]);
			crt = FILE_getcrt(path);
			if(!crt)
				goto freebuffer;
			printf("========(%s)========\n", path);
			snprintf(path, sizeof(path), "./kpool/%s.ssl.rootcrt.pem", p[n] );
			rootcrt = FILE_getcrt(path);
			if(!rootcrt)
				goto freebuffer;
			snprintf(path, sizeof(path), "./kpool/%s.ssl.cacrt.pem", p[n] );
			cacrt = FILE_getcrt(path);
			if(!cacrt)
				goto freebuffer;
			snprintf(path, sizeof(path), "./kpool/%s.ssl.pvk.pem", p[n]);
			pvk = FILE_getpvk(path);
			if(!pvk)
				goto freebuffer;

			cm = certmgr_new();
			if(!cm)
				goto freebuffer;
			ret = certmgr_add_ca(cm, cacrt);
			s_assert((ret == 0), "ret=%08x\n", ret);
			if(ret)
				goto freebuffer;
			ret = certmgr_add_ca(cm, rootcrt);
			s_assert((ret == 0), "ret=%08x\n", ret);
			if(ret)
				goto freebuffer;

			ctx = tls_client_new();
			if(!ctx)
				goto freebuffer;
			cysec_tls_client_set_ocspstapling_callback(ctx, test_ocspstapling_callback, (void *)cm);
			tls_client_set_verify_callback(ctx, tls_vcm_cb, (void *)cm);

			ret = tls_client_set_certificate(ctx, crt);
			s_assert((ret == 0), "ret=%08x", ret);
			if(ret)
				goto freebuffer;
			ret = tls_client_set_private_key(ctx, pvk);
			s_assert((ret == 0), "ret=%08x", ret);
			if(ret)
				goto freebuffer;
			//��������ģʽ����ʱʱ�����޷�����Alice������
			ret = cysec_tls_client_set_block_mode(ctx, CYSEC_TLS_CLIENT_BLOCK_MODE_BLOCK);
			if(ret)
				goto freebuffer;
			//������ʱ�趨
			ret = cysec_tls_client_set_rwtimeout(ctx, 5);
			if(ret)
				goto freebuffer;

			srvaddr = getenv("TLS_SERVER_ADDR") ? getenv("TLS_SERVER_ADDR") : "192.168.10.137";
			printf("connecting %s:%d \n", srvaddr, 443+n*2+m);
			ret = tls_client_connect(ctx, srvaddr, 443+n*2+m);
			s_assert((ret == 0), "ret=%08x", ret);
			if(ret)
				goto freebuffer;
			
			num = tls_client_write(ctx, (const unsigned char *)req, strlen(req));
			s_assert((num == (int)strlen(req)), "num=%08x (%zu)", num, strlen(req));
			printf("connected.\n");

			tls_client_close(ctx);
			printf("closed. %d \n\n", 443+n*2+m);

		freebuffer:
			if(ctx)
				tls_client_free(ctx);
			
			if(crt)
				x509crt_free(crt);

			if(pvk)
				pkey_free(pvk);

			if(cacrt)
				x509crt_free(cacrt);
			
			if(rootcrt)
				x509crt_free(rootcrt);
			
			if(cm)
				certmgr_free(cm);
			printf("=========================================================\n");

		}
	}
}

/**
	Bob��֤�鱻�����ˣ�Bob��tls�ͻ��ˣ���Alice������ca֤�����������������Լ������֤�飬������ģʽ
	ֻ��Bob��֤�鱻��������Alice��Ҫ����֤Bob֤���ǰ���£�Bob���������Լ��µ����֤�飻����Ͳ���
 */
static void test_ssl_revoked_nonblocking_certstatus(void) {
	const char* p[] = { "rsa", "sm2", "ecc"};
	unsigned int n, m; 
	int ret;
	int num;
	const char* srvaddr = NULL;
	const char* req = "GET / HTTP/1.0\r\nConnection: close\r\n\r\n";

	printf("==================================\n");
	printf("tls client is testing (NONBLOCKING socket)(certificate revoked) ...\n");
	printf("==================================\n");
	for (n = 0; n < sizeof(p)/sizeof(char*);  n ++) {
		for (m = 0; m < 2; m ++) {
			CERTMGR_PCTX cm = NULL;
			X509CRT_PCTX cacrt = NULL;
			X509CRT_PCTX crt = NULL;
			X509CRT_PCTX rootcrt = NULL;
			TLS_CLIENT_PCTX ctx=NULL;
			PKEY_PCTX pvk = NULL;
			char path[256] = {0};
			//Bob��ע����֤��
			printf("=========================================================\n");
			snprintf(path, sizeof(path), "./kpool/%s.ssl.revoke_crt.pem", p[n]);
			crt = FILE_getcrt(path);
			if(!crt)
				goto freebuffer;
			//ca��ǩ��֤��
			printf("========(%s)========\n", path);
			snprintf(path, sizeof(path), "./kpool/%s.ssl.rootcrt.pem", p[n] );
			rootcrt = FILE_getcrt(path);
			if(!rootcrt)
				goto freebuffer;
			//����ca֤��
			snprintf(path, sizeof(path), "./kpool/%s.ssl.cacrt.pem", p[n] );
			cacrt = FILE_getcrt(path);
			if(!cacrt)
				goto freebuffer;
			//Bob��ע����˽Կ
			snprintf(path, sizeof(path), "./kpool/%s.ssl.revoke_pvk.pem", p[n]);
			pvk = FILE_getpvk(path);
			if(!pvk)
				goto freebuffer;
			//����֤����
			cm = certmgr_new();
			if(!cm)
				goto freebuffer;
			ret = certmgr_add_ca(cm, cacrt);
			s_assert((ret == 0), "ret=%08x\n", ret);
			if(ret)
				goto freebuffer;
			ret = certmgr_add_ca(cm, rootcrt);
			s_assert((ret == 0), "ret=%08x\n", ret);
			if(ret)
				goto freebuffer;

			ctx = tls_client_new();
			if(!ctx)
				goto freebuffer;
			//ע�⣺������Bob��������ocsp��������֤Alice������Ƿ�Ϸ���Bob�����֤���Ƿ����޹��ڸò����ִ��
			cysec_tls_client_set_ocspstapling_callback(ctx, test_ocspstapling_callback, (void *)cm);
			//ע�⣺��������֤Alice��֤�����Ƿ���������Bob֤���Ƿ񱻳����޹�
			tls_client_set_verify_callback(ctx, tls_vcm_cb, (void *)cm);
			//��Ȼ֤�鱻̯������Bob����Ҫ���Լ���֤��
			ret = tls_client_set_certificate(ctx, crt);
			s_assert((ret == 0), "ret=%08x", ret);
			if(ret)
				goto freebuffer;
			ret = tls_client_set_private_key(ctx, pvk);
			s_assert((ret == 0), "ret=%08x", ret);
			if(ret)
				goto freebuffer;

			if(ret)
				goto freebuffer;
			//��Ȼ�Ƿ���֤��������󣬵����ų�Alice�û���Bob����ݽ�����֤����������£���ΪBob֤���ѱ��������޷�����Alice
			//��֤���������ʱ��AliceӦ�ò����Bob����ݽ�����֤
			srvaddr = getenv("TLS_SERVER_ADDR") ? getenv("TLS_SERVER_ADDR") : "192.168.10.137";
			printf("connecting %s:%d \n", srvaddr, 443+n*2+m);
			ret = tls_client_connect(ctx, srvaddr, 443+n*2+m);
			s_assert((ret == 0), "ret=%08x", ret);
			if(ret)
				goto freebuffer;
			
			num = tls_client_write(ctx, (const unsigned char *)req, strlen(req));
			s_assert((num == (int)strlen(req)), "num=%08x (%zu)", num, strlen(req));
			printf("connected.\n");

			tls_client_close(ctx);
			printf("closed. %d \n\n", 443+n*2+m);

		freebuffer:
			if(ctx)
				tls_client_free(ctx);
			
			if(crt)
				x509crt_free(crt);

			if(pvk)
				pkey_free(pvk);

			if(cacrt)
				x509crt_free(cacrt);
			
			if(rootcrt)
				x509crt_free(rootcrt);
			
			if(cm)
				certmgr_free(cm);
			printf("=========================================================\n");

		}
	}
}

/**
	Bob��֤�鱻�����ˣ�Bob��tls�ͻ��ˣ���Alice������ca֤�����������������Լ������֤�飬����ģʽ
	ֻ��Bob��֤�鱻��������Alice��Ҫ����֤Bob֤���ǰ���£�Bob���������Լ��µ����֤�飻����Ͳ���
 */
static void test_ssl_revoked_blocking_certstatus(void) {
	const char* p[] = { "rsa", "sm2", "ecc"};
	unsigned int n, m; 
	int ret;
	int num;
	const char* srvaddr = NULL;
	const char* req = "GET / HTTP/1.0\r\nConnection: close\r\n\r\n";

	printf("==================================\n");
	printf("tls client is testing (BLOCKING socket)(certificate revoked) ...\n");
	printf("==================================\n");
	for (n = 0; n < sizeof(p)/sizeof(char*);  n ++) {
		for (m = 0; m < 2; m ++) {
			CERTMGR_PCTX cm = NULL;
			X509CRT_PCTX cacrt = NULL;
			X509CRT_PCTX crt = NULL;
			PKEY_PCTX pvk = NULL;
			X509CRT_PCTX rootcrt = NULL;
			TLS_CLIENT_PCTX ctx = NULL;
			char path[256] = {0};
			
			printf("=========================================================\n");
			snprintf(path, sizeof(path), "./kpool/%s.ssl.revoke_crt.pem", p[n]);
			crt = FILE_getcrt(path);
			if(!crt)
				goto freebuffer;

			printf("========(%s)========\n", path);
			snprintf(path, sizeof(path), "./kpool/%s.ssl.rootcrt.pem", p[n] );
			rootcrt = FILE_getcrt(path);
			if(!rootcrt)
				goto freebuffer;

			snprintf(path, sizeof(path), "./kpool/%s.ssl.cacrt.pem", p[n] );
			cacrt = FILE_getcrt(path);
			if(!cacrt)
				goto freebuffer;

			snprintf(path, sizeof(path), "./kpool/%s.ssl.revoke_pvk.pem", p[n]);
			pvk = FILE_getpvk(path);
			if(!pvk)
				goto freebuffer;

			cm = certmgr_new();
			if(!cm)
				goto freebuffer;
			ret = certmgr_add_ca(cm, cacrt);
			s_assert((ret == 0), "ret=%08x\n", ret);
			if(ret)
				goto freebuffer;
			ret = certmgr_add_ca(cm, rootcrt);
			s_assert((ret == 0), "ret=%08x\n", ret);
			if(ret)
				goto freebuffer;


			ctx = tls_client_new();
			if(!ctx)
				goto freebuffer;
			cysec_tls_client_set_ocspstapling_callback(ctx, test_ocspstapling_callback, (void *)cm);
			tls_client_set_verify_callback(ctx, tls_vcm_cb, (void *)cm);

			ret = tls_client_set_certificate(ctx, crt);
			s_assert((ret == 0), "ret=%08x", ret);
			if(ret)
				goto freebuffer;
			ret = tls_client_set_private_key(ctx, pvk);
			s_assert((ret == 0), "ret=%08x", ret);
			if(ret)
				goto freebuffer;

			ret = cysec_tls_client_set_block_mode(ctx, CYSEC_TLS_CLIENT_BLOCK_MODE_BLOCK);
			if(ret)
				goto freebuffer;

			ret = cysec_tls_client_set_rwtimeout(ctx, 5);
			if(ret)
				goto freebuffer;

			srvaddr = getenv("TLS_SERVER_ADDR") ? getenv("TLS_SERVER_ADDR") : "192.168.10.137";
			printf("connecting %s:%d \n", srvaddr, 443+n*2+m);
			ret = tls_client_connect(ctx, srvaddr, 443+n*2+m);
			s_assert((ret == 0), "ret=%08x", ret);
			if(ret)
				goto freebuffer;
			
			num = tls_client_write(ctx, (const unsigned char *)req, strlen(req));
			s_assert((num == (int)strlen(req)), "num=%08x (%zu)", num, strlen(req));
			printf("connected.\n");

			tls_client_close(ctx);
			printf("closed. %d \n\n", 443+n*2+m);

		freebuffer:
			if(ctx)
				tls_client_free(ctx);
			
			if(crt)
				x509crt_free(crt);

			if(pvk)
				pkey_free(pvk);

			if(cacrt)
				x509crt_free(cacrt);
			
			if(rootcrt)
				x509crt_free(rootcrt);
			
			if(cm)
				certmgr_free(cm);
			printf("=========================================================\n");

		}
	}
}

/**
	Bob��֤������ˣ�Bob��tls�ͻ��ˣ���Alice������ca֤�����������������Լ������֤�飬������ģʽ
	ֻ��Bob��֤������ˣ���Alice��Ҫ����֤Bob֤���ǰ���£�Bob���������Լ��µ����֤�飻����Ͳ���
 */
static void test_ssl_expired_nonblocking_certstatus(void) {
	const char* p[] = { "rsa", "sm2", "ecc"};
	unsigned int n, m; 
	int ret;
	int num;
	const char* srvaddr = NULL;
	const char* req = "GET / HTTP/1.0\r\nConnection: close\r\n\r\n";

	printf("==================================\n");
	printf("tls client is testing (NONBLOCKING socket)(certificate expired) ...\n");
	printf("==================================\n");
	for (n = 0; n < sizeof(p)/sizeof(char*);  n ++) {
		for (m = 0; m < 2; m ++) {
			CERTMGR_PCTX cm = NULL;
			X509CRT_PCTX cacrt = NULL;
			X509CRT_PCTX crt = NULL;
			X509CRT_PCTX rootcrt = NULL;
			PKEY_PCTX pvk = NULL;
			char path[256] = {0};
			TLS_CLIENT_PCTX ctx = NULL;
			//Bob���ڵ����֤��
			printf("=========================================================\n");
			snprintf(path, sizeof(path), "./kpool/%s.ssl.expire_crt.pem", p[n]);
			crt = FILE_getcrt(path);
			if(!crt)
				goto freebuffer;

			printf("========(%s)========\n", path);
			snprintf(path, sizeof(path), "./kpool/%s.ssl.rootcrt.pem", p[n] );
			rootcrt = FILE_getcrt(path);
			if(!rootcrt)
				goto freebuffer;

			snprintf(path, sizeof(path), "./kpool/%s.ssl.cacrt.pem", p[n] );
			cacrt = FILE_getcrt(path);
			if(!cacrt)
				goto freebuffer;
			//Bob���ڵ���Կ
			snprintf(path, sizeof(path), "./kpool/%s.ssl.expire_pvk.pem", p[n]);
			pvk = FILE_getpvk(path);
			if(!pvk)
				goto freebuffer;

			cm = certmgr_new();
			if(!cm)
				goto freebuffer;
			ret = certmgr_add_ca(cm, cacrt);
			s_assert((ret == 0), "ret=%08x\n", ret);
			if(ret)
				goto freebuffer;
			ret = certmgr_add_ca(cm, rootcrt);
			s_assert((ret == 0), "ret=%08x\n", ret);
			if(ret)
				goto freebuffer;

			ctx = tls_client_new();
			if(!ctx)
				goto freebuffer;
			//ע�⣺������Bob��������ocsp��������֤Alice������Ƿ�Ϸ���Bob�����֤���Ƿ�����޹��ڸò����ִ��
			cysec_tls_client_set_ocspstapling_callback(ctx, test_ocspstapling_callback, (void *)cm);
			//ע�⣺��������֤Alice��֤�����Ƿ���������Bob֤���Ƿ�����޹�
			tls_client_set_verify_callback(ctx, tls_vcm_cb, (void *)cm);
			//��Ȼ֤����ڣ���Bob����Ҫ���Լ���֤��
			ret = tls_client_set_certificate(ctx, crt);
			s_assert((ret == 0), "ret=%08x", ret);
			if(ret)
				goto freebuffer;
			ret = tls_client_set_private_key(ctx, pvk);
			s_assert((ret == 0), "ret=%08x", ret);
			if(ret)
				goto freebuffer;

			if(ret)
				goto freebuffer;
			//��Ȼ�Ƿ���֤��������󣬵����ų�Alice�û���Bob����ݽ�����֤����������£���ΪBob����֤����ڶ��޷�����Alice
			//��֤���������ʱ��AliceӦ�ò����Bob����ݽ�����֤
			srvaddr = getenv("TLS_SERVER_ADDR") ? getenv("TLS_SERVER_ADDR") : "192.168.10.137";
			printf("connecting %s:%d \n", srvaddr, 443+n*2+m);
			ret = tls_client_connect(ctx, srvaddr, 443+n*2+m);
			s_assert((ret == 0), "ret=%08x", ret);
			if(ret)
				goto freebuffer;
			
			num = tls_client_write(ctx, (const unsigned char *)req, strlen(req));
			s_assert((num == (int)strlen(req)), "num=%08x (%zu)", num, strlen(req));
			printf("connected.\n");

			tls_client_close(ctx);
			printf("closed. %d \n\n", 443+n*2+m);

		freebuffer:

			if(ctx)
				tls_client_free(ctx);
			
			if(crt)
				x509crt_free(crt);

			if(pvk)
				pkey_free(pvk);

			if(cacrt)
				x509crt_free(cacrt);
			
			if(rootcrt)
				x509crt_free(rootcrt);
			
			if(cm)
				certmgr_free(cm);
			printf("=========================================================\n");

		}
	}
}

/**
	Bob��֤������ˣ�Bob��tls�ͻ��ˣ���Alice������ca֤�����������������Լ������֤�飬����ģʽ
	ֻ��Bob��֤������ˣ���Alice��Ҫ����֤Bob֤���ǰ���£�Bob���������Լ��µ����֤�飻����Ͳ���
 */
static void test_ssl_expired_blocking_certstatus(void) {
	const char* p[] = { "rsa", "sm2", "ecc"};
	unsigned int n, m; 
	int ret;
	int num;
	const char* srvaddr = NULL;
	const char* req = "GET / HTTP/1.0\r\nConnection: close\r\n\r\n";

	printf("==================================\n");
	printf("tls client is testing (BLOCKING socket)(certificate expired) ...\n");
	printf("==================================\n");
	for (n = 0; n < sizeof(p)/sizeof(char*);  n ++) {
		for (m = 0; m < 2; m ++) {
			CERTMGR_PCTX cm = NULL;
			X509CRT_PCTX cacrt = NULL;
			X509CRT_PCTX crt = NULL;
			X509CRT_PCTX rootcrt = NULL;
			PKEY_PCTX pvk = NULL;
			char path[256] = {0};
			TLS_CLIENT_PCTX ctx = NULL;
			
			printf("=========================================================\n");
			snprintf(path, sizeof(path), "./kpool/%s.ssl.expire_crt.pem", p[n]);
			crt = FILE_getcrt(path);
			if(!crt)
				goto freebuffer;

			printf("========(%s)========\n", path);
			snprintf(path, sizeof(path), "./kpool/%s.ssl.rootcrt.pem", p[n] );
			rootcrt = FILE_getcrt(path);
			if(!rootcrt)
				goto freebuffer;

			snprintf(path, sizeof(path), "./kpool/%s.ssl.cacrt.pem", p[n] );
			cacrt = FILE_getcrt(path);
			if(!cacrt)
				goto freebuffer;

			snprintf(path, sizeof(path), "./kpool/%s.ssl.expire_pvk.pem", p[n]);
			pvk = FILE_getpvk(path);
			if(!pvk)
				goto freebuffer;

			cm = certmgr_new();
			if(!cm)
				goto freebuffer;
			ret = certmgr_add_ca(cm, cacrt);
			s_assert((ret == 0), "ret=%08x\n", ret);
			if(ret)
				goto freebuffer;
			ret = certmgr_add_ca(cm, rootcrt);
			s_assert((ret == 0), "ret=%08x\n", ret);
			if(ret)
				goto freebuffer;


			ctx = tls_client_new();
			if(!ctx)
				goto freebuffer;
			cysec_tls_client_set_ocspstapling_callback(ctx, test_ocspstapling_callback, (void *)cm);
			tls_client_set_verify_callback(ctx, tls_vcm_cb, (void *)cm);

			ret = tls_client_set_certificate(ctx, crt);
			s_assert((ret == 0), "ret=%08x", ret);
			if(ret)
				goto freebuffer;
			ret = tls_client_set_private_key(ctx, pvk);
			s_assert((ret == 0), "ret=%08x", ret);
			if(ret)
				goto freebuffer;

			ret = cysec_tls_client_set_block_mode(ctx, CYSEC_TLS_CLIENT_BLOCK_MODE_BLOCK);
			if(ret)
				goto freebuffer;

			ret = cysec_tls_client_set_rwtimeout(ctx, 5);
			if(ret)
				goto freebuffer;

			srvaddr = getenv("TLS_SERVER_ADDR") ? getenv("TLS_SERVER_ADDR") : "192.168.10.137";
			printf("connecting %s:%d \n", srvaddr, 443+n*2+m);
			ret = tls_client_connect(ctx, srvaddr, 443+n*2+m);
			s_assert((ret == 0), "ret=%08x", ret);
			if(ret)
				goto freebuffer;
			
			num = tls_client_write(ctx, (const unsigned char *)req, strlen(req));
			s_assert((num == (int)strlen(req)), "num=%08x (%zu)", num, strlen(req));
			printf("connected.\n");

			tls_client_close(ctx);
			printf("closed. %d \n\n", 443+n*2+m);

		freebuffer:
			if(ctx)
				tls_client_free(ctx);
			
			if(crt)
				x509crt_free(crt);

			if(pvk)
				pkey_free(pvk);

			if(cacrt)
				x509crt_free(cacrt);
			
			if(rootcrt)
				x509crt_free(rootcrt);
			
			if(cm)
				certmgr_free(cm);
			printf("=========================================================\n");

		}
	}
}

int main(void)
{

	test_nonblocking_ssl_certstatus();
	
	test_blocking_ssl_certstatus();


	test_ssl_revoked_nonblocking_certstatus();
	test_ssl_revoked_blocking_certstatus();
	test_ssl_expired_nonblocking_certstatus();
	test_ssl_expired_blocking_certstatus();
	//test_ssl_one();
	test_ssl_benchmark();
	return 0;
}

#else
int  main()
{
	return 0;
}

#endif //CYSEC_NO_TLS