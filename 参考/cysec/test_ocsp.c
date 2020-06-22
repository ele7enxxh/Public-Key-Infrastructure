#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cysec.h>
#include "test_util.h"

#ifndef CYSEC_NO_OCSP

#define TEST_E_NETWORK  0x10000
#define TEST_E_GETHOSTNAME TEST_E_NETWORK + 1
#define TEST_E_SOCKET TEST_E_NETWORK + 2
#define TEST_E_CONNECT  TEST_E_NETWORK + 3
#define TEST_E_SEND   TEST_E_NETWORK + 4
#define TEST_E_RECEVICE   TEST_E_NETWORK + 5
#define TEST_E_ADD_HTTP_HEADER TEST_E_NETWORK + 6
#define TEST_E_REMOVE_RESPONSE TEST_E_NETWORK + 7 

/**
	ocsp�����Կ�����crl�����Ʒ
 */
 /**
	����http��Ӧ����
  */
static int recv_timeout(int socket_fd, int timeout, unsigned char **out)
{
	int size_recv, total_size = 0;
	char recv_buff[1024];
	double timediff;
	struct timeval begin, now;
	unsigned char  *newout = NULL;
	int pre_total_size = 0;

	if(!out)
		return 0;

	//make socket non blocking
	fcntl(socket_fd, F_SETFL, O_NONBLOCK);

	//beginning time
	gettimeofday(&begin, NULL);
	*out = NULL;

	while(1)
	{
		gettimeofday(&now, NULL);

		//time elapsed in seconds
        timediff = (now.tv_sec - begin.tv_sec) ;
         
        //if you got some data, then break after timeout
        if( total_size > 0 && timediff > timeout )
        {
            break;
        }
         
        //if you got no data at all, wait a little longer, twice the timeout
        else if( timediff > timeout * 2)
        {
            break;
        }
         
        memset(recv_buff ,0 , sizeof(recv_buff));  //clear the variable
        if((size_recv =  recv(socket_fd , recv_buff , sizeof(recv_buff) , 0) ) < 0)
        {
            //if nothing was received then we want to wait a little before trying again, 0.1 seconds
            usleep(100000);
        }
        else
        {
        	pre_total_size = total_size;
        	total_size += size_recv;
        	newout = realloc(*out, total_size);
        	if(!newout){
        		free(*out);
        		return 0;
        	}
        	memcpy(newout + pre_total_size, recv_buff, size_recv);
        	*out = newout;
            //reset beginning time
            gettimeofday(&begin , NULL);
        }
	}

	return total_size;
}

/**
	���ܣ�Ϊocsp�������http����ͷ
	@param	address		ocsp��������ַ
	@param	port		ocsp�������˿�
	@param	in			������ocsp��������
	@param	ilen		������ocsp�������ݳ���
	@param	out			��ocsp���������͵���������
 */
static size_t add_httpheader(const char *address, int port, const unsigned char *in, size_t ilen,  unsigned char **out)
{
    size_t header_size = 0, total_size = 0;
    char tmp_buf[16] = {0};

    if(!in || !out)
        return 0;

    total_size += ilen;

    header_size += strlen("POST / HTTP/1.1\r\nHost: :\r\n");
    header_size += strlen("User-Agent: PECL::HTTP/1.6.6 (PHP/4.4.9)\r\n");
    header_size += strlen("Accept: */*\r\n");
    header_size += strlen("Content-Length: \r\n");
    header_size += strlen("Content-Type: application/x-www-form-urlencoded\r\n\r\n");
    sprintf(tmp_buf, "%d", port);
    header_size += strlen(address) + strlen(tmp_buf);
    memset(tmp_buf, 0, sizeof(tmp_buf));
    sprintf(tmp_buf, "%zu", ilen);
    header_size += strlen(tmp_buf);
    total_size += header_size;

    *out = calloc(1, total_size);
    if(!*out){
        return 0;
    }

    sprintf((char *)*out,"POST / HTTP/1.1\r\nHost: %s:%d\r\n", address, port);
    strcat((char *)*out,"User-Agent: PECL::HTTP/1.6.6 (PHP/4.4.9)\r\n");
    strcat((char *)*out,"Accept: */*\r\n");
    sprintf((char *)*out + strlen((char *)*out),"Content-Length: %zu\r\n", ilen);
    strcat((char *)*out,"Content-Type: application/x-www-form-urlencoded\r\n\r\n");
    memcpy(*out+strlen((char *)*out), in, ilen);

    return total_size;
}

/**
	���ܣ���http��Ӧͷ����ɾȥ��ֻ����http��������
 */
static size_t remove_httpheader(const unsigned char *in ,size_t ilen, unsigned char **out)
{
    char *str_in = (char *)in;
    char *ocsp_body = NULL, *content_length = NULL,*p;
    char length_str[16] = {0};
    int i=0;
    size_t ret=0;

    if(!in || (ilen == 0) || !out)
        return 0;

    ocsp_body = strstr(str_in, "\r\n\r\n");
    if(!ocsp_body)
        return 0;
    else
        ocsp_body += 4;

    content_length = strstr(str_in, "Content-Length");
    if(!content_length)
        return 0;
    else
        content_length += 15;

    p = content_length;
    while( *p != '\r'){
        length_str[i] = *p;
        i++;
        p++;
    }

    ret = atoi(length_str);
    *out=calloc(1, ret+1);
    if(!(*out))
        return 0;

    memcpy(*out, ocsp_body, ret);
    return ret;
}

//only for testing....don't copy

/**
	@param	address		ocsp��������ַ
	@param	port		ocsp�������˿�
	@param	in			������ocsp����
	@param	ilen		������ocsp����ĳ���
	@param	out			ocsp��Ӧ����
	@param	olen		ocsp��Ӧ���ݳ��ȣ�http����Ӧbody��
 */
int test_http_post(const char *address, int port, const unsigned char *in, size_t ilen, unsigned char **out, size_t *olen)
{
	struct sockaddr_in server_info;
    struct hostent *he;
    int socket_fd;
    char send_buff[1024];
    size_t tlen = 0;
    int ret = 0;
    unsigned char *http_request = NULL, *ocsp_response = NULL;
    size_t http_req_len = 0, ocsp_rsp_len = 0;
    unsigned char *p=NULL;

	//��ȡocsp������������Ϣ
    he = gethostbyname(address);
    if(!he)
    	return TEST_E_GETHOSTNAME;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd == -1){
    	return TEST_E_SOCKET;
    }
	//Bob����Ҫ��ocsp��������ͨ��Э�鼰ocsp��ַ&�˿�
    memset(&server_info, 0 , sizeof(server_info));
    server_info.sin_family = AF_INET;
    server_info.sin_port = htons(port);
    server_info.sin_addr = *((struct in_addr *)he->h_addr);
	//Bob������ocsp��������������
    ret = connect(socket_fd, (struct sockaddr *)&server_info, sizeof(struct  sockaddr));
    if(ret < 0){
    	return TEST_E_CONNECT;
    }
	//�ڷ�������֮ǰ��Bob��http����ͷ���ݣ�������http_request��
    http_req_len = add_httpheader(address, port, in, ilen, &http_request);
    if(http_req_len == 0)
        return TEST_E_ADD_HTTP_HEADER;

    ilen = http_req_len;
    p = http_request;
    tlen = ilen;
	//Bob����http�����ocsp������
    while(1){
    	if(sizeof(send_buff) >= tlen ){
    		memset(send_buff, 0, sizeof(send_buff));
    		memcpy(send_buff, p, tlen);
    		if((send(socket_fd, send_buff, tlen, 0)) == -1){
    			printf("Failure sending message\n");
                free(http_request);
    			close(socket_fd);
    			return TEST_E_SEND;
    		}
    		break;
    	}else{
    		memset(send_buff, 0, sizeof(send_buff));
    		memcpy(send_buff, p, sizeof(send_buff));
    		p += sizeof(send_buff);
    		tlen -= sizeof(send_buff);
    		if((send(socket_fd, send_buff, sizeof(send_buff), 0))){
    			printf("Failure sending message\n");
                free(http_request);
    			close(socket_fd);
    			return TEST_E_SEND;
    		}
    	}
    }

    //printf("Sent successfully\n");
	//Bob����http��Ӧ�������ó�ʱ����
    ocsp_rsp_len = recv_timeout(socket_fd, 2, &ocsp_response);
    if(ocsp_rsp_len <=0 ){
    	printf("Failure recevicing message\n");
        free(http_request);
    	close(socket_fd);
    	return TEST_E_RECEVICE;
    }

    close(socket_fd);
    *olen = remove_httpheader(ocsp_response, ocsp_rsp_len, out);
    if(*olen == 0){
        free(http_request);
        return TEST_E_REMOVE_RESPONSE;
    }

    free(http_request);
    if(ocsp_response)
        free(ocsp_response);
    return 0;
}

/**
	��Ҫ�� test_tls.c ��Alice��Bob����
	���ܣ���������֤��״̬Э�飨OCSP����ȷ�����֤���Ƿ���Ч
	1.��֤�߸��ݴ���֤֤����Ϣ������һ��ocsp���������͸�ocsp������
	2.ocsp��������ȡ�������󣬽��д�������һ��ocsp��Ӧ��Ϣ����֤��
	3.��֤�߸���ocsp��Ӧ��Ϣ����ocsp��Ӧ�������������Ӧ������������Ƿ�ƥ��
	4.��ƥ�䣬��֤�߾Ϳ��Դ�ocsp��Ӧ����л�֤��������л�ȡǩ����֤�飬��ocsp�����������֤��
	5.��֤�߸���ǩ���߹�Կȷ��ǩ����֤�����ʵ��
	6.��֤�߼��ǩ����֤�����Ƿ�����
	7.��ȡocsp��Ӧ�뼰��Ӧ״̬���Ӷ��ж�Ҫ��֤֤����Ƿ���Ч
 */
void test_ocsp(void) {
	const char* p[] = { "rsa.n", "rsa.r", "sm2.n", "sm2.r", "ecc.n", "ecc.r" };
	char *address = NULL;
	int port = 0;
	int n, m, ret;

	for (n = 0; n < 2; n ++) {
		for ( m = 0; m < 2; m++ ){
			CERTMGR_PCTX ctx = NULL;
			X509CRT_PCTX cacrt = NULL;
			X509CRT_PCTX crt = NULL;
			X509CRT_PCTX signer = NULL;	
			char path[256] = {0};
			unsigned char* request = NULL, *response = NULL;
			size_t reqlen = 0, rsplen = 0;
			unsigned int ocspstatus = 0, ocsp_certstatus = 0;
            OCSP_REQUEST_PCTX req_ctx = NULL;
            OCSP_RESPONSE_PCTX rsp_ctx = NULL; 
			//��ȡBob��֤��
			snprintf(path, sizeof(path), "./kpool/%s.crt.pem", p[n*2 + m ]);
			crt = FILE_getcrt(path);
            if(!crt){
                printf("The certificate (%s) is invalid.\n",path);
                break;
            }
			//��ȡAlice��֤�飨����ca֤�飩
			snprintf(path, sizeof(path), "./kpool/%s.rootcrt.pem", p[n*2 + m] );
			cacrt = FILE_getcrt(path);
            if(!crt){
                printf("The CA certificate (%s) is invalid.\n",path);
                break;
            }
			//����֤�������
			ctx = certmgr_new();
			//���Alice֤��
			ret = certmgr_add_ca(ctx, cacrt);
			s_assert((ret == 0), "ret=%d\n", ret);
			//���ݴ���֤֤��crt����ocsp������
			//Bob��Ҫ��Alice����ͨ�ţ�ͨ��֮ǰ��Bob��ȷ���Լ������֤���Ƿ�Ϸ������Է���һ��ocsp���󣨾������ocsp������������֤
			req_ctx = cysec_ocspreq_new(crt, ctx);		
			s_assert((req_ctx != NULL), "Failure new ocspreq\n");
			//Bob��ocsp���������б���
			ret = cysec_ocspreq_encode(req_ctx, &request, &reqlen);
			s_assert((ret == 0),"ret = %d\n", ret);
			//Bob���л�ocsp������
            snprintf(path, sizeof(path), "./kpool/%s.ocspreq.der", p[n*2 + m]);
            FILE_putcontent(request, reqlen, path);

			//Bob��ȡocsp��������ַ�Ͷ˿�
			port = 2560 + n;
			address = getenv("OCSP_SERVER_ADDR") ? getenv("OCSP_SERVER_ADDR") : "192.168.20.165";
			printf("connecting %s:%d \n", address, port);
			//��test_tls.c��ͬ�����ﲢδ��ocsp��Ӧ�ŵ�һ���ص���ȥ��������test_http_post�ܷ�ɹ�����response����ֱ�ӹغ�������֤�ĳɰ�
			//����ʱҪע���test_http_post����ֵ�Ĺ۲�
			ret = test_http_post(address, port, request, reqlen, (unsigned char **)&response, &rsplen);
			s_assert((ret == 0), "ret = %d\n", ret);
			if(ret != 0)
				break;
			//Bob��ocsp���������ص�ocsp��Ӧ���ݽ��룬����ocsp��Ӧ���
			ret = cysec_ocsprsp_decode(response, rsplen, &rsp_ctx);
			s_assert((ret == 0), "recevice an invalid OCSP response, %08x\n",ret);
			//Bob���ocsp��������ocsp��Ӧ����Ƿ�ƥ��(Bob����ʹ��ocsp�������Ĺ�Կ��ocsp������������ǩ��������֤����ȷ�Ͼ���Ƿ�ƥ�䡣���������濴��Ӧ�ò���������֤����)
			ret = cysec_ocsprsp_check(req_ctx, rsp_ctx);
			s_assert((ret == 0), "Malware response, ret = %08x\n", ret);
			//Bob��ocsp��Ӧ����л�ȡǩ����֤��(��ocsp�����������֤��)
			signer = cysec_ocsprsp_get_signer(rsp_ctx);
			//���ocsp��Ӧ�����û���ṩǩ����֤�飬�͸���ocsp��Ӧ������ݴ�֤��������л�ȡǩ����֤��
			if(!signer)
				signer = cysec_certmgr_get_ocsprsp_signer(ctx, rsp_ctx);
            if(!signer){
                printf("Failed to get signer.\n");
                break;
            }
			//Bob�Ѿ���ocsp���������֤��Ĺ�Կ��ͨ���˹�Կ��֤��ǩ����֤��ȷ������ocsp����������ʱBobȷ���Լ�֤���Ƿ�Ϸ������
			ret = cysec_ocsprsp_verify(rsp_ctx,signer);
			s_assert((ret == 0), "Verify Signature Failure, ret = %08x\n", ret);
			//Bob���ǩ����֤�����Ƿ�����
			ret = cysec_certmgr_verify(ctx, signer);
			s_assert((ret == 0), "Verify Certificate Chain Failure, ret = %08x\n", ret);
			//��ȡocsp��Ӧ�루�����ȡocsp��Ӧʧ�ܿ��Ը�����Ӧ��ȷ��ԭ��
			ret = cysec_ocsprsp_get_rspstatus(rsp_ctx, &ocspstatus);
			s_assert((ret == 0), "failed to get rsp status %08x\n",ret);
			printf("rspstatus is %d\n",ocspstatus);
			//��ȡocsp��Ӧ���״̬�룬�Դ��ж�֤��crt��״̬��������ע����δ֪�ȴ���
			ret = cysec_ocsprsp_get_certstatus(rsp_ctx, crt, ctx, &ocsp_certstatus);
			s_assert((ret == 0), "failed to get cert status %08x\n", ret);
			printf("certstatus is %d\n", ocsp_certstatus);

			if(req_ctx)
				cysec_ocspreq_free(&req_ctx);
			if(rsp_ctx)
				cysec_ocsprsp_free(&rsp_ctx);

			if(ctx)
				certmgr_free(ctx);

			if(signer)
				x509crt_free(signer);

			if(cacrt)
				x509crt_free(cacrt);

			if(crt)
				x509crt_free(crt);
			
			if(request)
				SAFE_FREE(request);
			if(response)
				SAFE_FREE(response);			
		}

	}
}

int main(void)
{
	test_ocsp();
    exit(0);
}

#else
int  main()
{
    return 0;
}

#endif //CYSEC_NO_OCSP