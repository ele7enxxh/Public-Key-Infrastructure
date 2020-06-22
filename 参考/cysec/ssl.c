#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <cysec.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "test_util.h"

// #include <string.h>  
// #include <errno.h>  
// #include <sys/socket.h>  
// #include <resolv.h>  
// #include <stdlib.h>  
// #include <netinet/in.h>  
// #include <arpa/inet.h>  
// #include <unistd.h>  
// #include <openssl/ssl.h>  
// #include <openssl/err.h>  

#ifndef CYSEC_NO_TLS

static void tls_connect()
{
	struct sockaddr_in server_info;
	int ret = 0;
	int errcode = 0;
	char errbuf[CYSEC_TLS_CLIENT_ERROR_STRING_MAX_SZ - 1];
	int socket_fd = 0;
	void *ptr = NULL;
	const char* srvaddr = NULL;
	int port = 0;
	char buf[1024];
	X509CRT_PCTX peercrt = NULL;
	//�����׽���
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_fd == -1){
    	return ;
    }
	//���÷�������ַ
	srvaddr = "192.168.10.130";
	port = 447;
    // memset(&server_info, 0 , sizeof(server_info));
    // server_info.sin_family = AF_INET;
    // server_info.sin_port = htons(port);
	// server_info.sin_addr.s_addr = inet_addr(srvaddr);
	
	/* ��ʼ���������ˣ��Է����ĵ�ַ�Ͷ˿���Ϣ */  
    bzero(&server_info, sizeof(server_info));  
    server_info.sin_family = AF_INET;  
    server_info.sin_port = htons(port);  
    if (inet_aton(srvaddr, (struct in_addr *) &server_info.sin_addr.s_addr) == 0) {  
        perror("srvaddr");  
        exit(errno);  
    }  
    printf("address created\n");  
	
	//ʹ��tcpЭ�����һ�������
	ret = connect(socket_fd, (struct sockaddr *)&server_info, sizeof(struct  sockaddr));	//success
	if (ret){
		perror("connect error");  
        exit(errno);  
	}
	/** �Դ����ӽ���ssl���� */
	//����ssl���ӻỰ��Ϣ���
	TLS_CLIENT_PCTX ctx = tls_client_new();
	printf("connecting %s:%d \n", srvaddr, port);
	//����ssl���ӵ��׽���
	ret = cysec_tls_client_set_fd(ctx, socket_fd);	//success
	if (ret){
		fprintf(stderr, "********* %d\n", __LINE__);
	}
	//����ssl����
	ret = cysec_tls_client_ssl_connect(ctx);		//error
	if (ret){
		s_assert((ret == 0), "ret=%08x\n", ret);
		goto freebuffer;
	} else {		//��ȡ���ܻỰ��Ϣ
		ptr = cysec_tls_client_get_ssl_session(ctx);
		if (ptr == NULL)
			s_assert((ptr != NULL), "get ssl session failed\n");
		else
			fprintf(stdout, "%s %d %s\n", __FUNCTION__, __LINE__, (char*)ptr);
		peercrt = cysec_tls_client_get_peer_certificate(ctx);
		if (peercrt == NULL)
			s_assert((peercrt != NULL), "get peer certificate failde\n");
		else
			dumpcrt(peercrt);
		const char * chptr = cysec_tls_client_get_ciphername(ctx);
		if (chptr == NULL)
			s_assert((chptr != NULL), "get ciphername failed\n");
		else
			fprintf(stdout, "%s %d %s\n", __FUNCTION__, __LINE__, chptr);
	}
	//�����ݿɶ�
	bzero(buf, CYSEC_TLS_CLIENT_ERROR_STRING_MAX_SZ - 1); 
	while (cysec_tls_client_pending(ctx)){
		ret = cysec_tls_client_read(ctx, buf, sizeof(buf));
		if (ret < 0)
			goto freebuffer;
	}
	//�ر�ssl���
	cysec_tls_client_shutdown(ctx);
	
freebuffer:
	if (ret){
		errcode = cysec_tls_client_get_sslerror(ctx, errcode);
		printf("ssl errcode: (%d) %s\n", errcode, cysec_tls_client_get_sslerror_string(errcode, errbuf));
	}
	cysec_tls_client_shutdown(ctx);
}


int main(void)
{

	tls_connect();
	
	return 0;
}

#else
int  main()
{
	return 0;
}

#endif //CYSEC_NO_TLS