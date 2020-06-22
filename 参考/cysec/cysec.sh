#!/bin/sh

if [ -e $1 ] && [ -f $1 ]; then
	rm -f $1
fi

echo "wait for some time..."

echo "========================="
echo "======= HASH���� ========"
echo "========================="
gcc -g -o2 test_digest.c test_util.c -o test_digest -I$CYSEC/include -L$CYSEC/lib -lcysec
if [ $? -ne 0 ]; then
	echo "generate test_digest failed"
	exit 1
fi

if [ $# -ge 1 ]; then
	./test_digest >> $1
else
	./test_digest
fi
	
if [ $? -ne 0 ]; then
	echo "execute test_digest failed"
	exit 1
fi
echo "======= HASH��� ========"

echo "========================="
echo "======= HMAC���� ========"
echo "========================="
gcc -g -o2 test_hmac.c test_util.c -o test_hmac -I$CYSEC/include -L$CYSEC/lib -lcysec
if [ $? -ne 0 ]; then
	echo "generate test_hmac failed"
	exit 1
fi

if [ $# -ge 1 ]; then
	./test_hmac >> $1
else
	./test_hmac
fi

if [ $? -ne 0 ]; then
	echo "execute test_hmac failed"
	exit 1
fi
echo "======= HMAC��� ========"

echo "========================="
echo "==== �ԳƼӽ��ܲ��� ====="
echo "========================="
gcc -g -o2 test_cipher.c test_util.c -o test_cipher -I$CYSEC/include -L$CYSEC/lib -lcysec
if [ $? -ne 0 ]; then
	echo "generate test_cipher failed"
	exit 1
fi

if [ $# -ge 1 ]; then
	./test_cipher >> $1
else
	./test_cipher
fi

if [ $? -ne 0 ]; then
	echo "execute test_cipher failed"
	exit 1
fi
echo "==== �ԳƼӽ������ ====="

echo "========================="
echo "===== pkcs7ǩ������ ====="
echo "========================="
gcc -g -o2 test_pkcs7.c test_util.c -o test_pkcs7 -I$CYSEC/include -L$CYSEC/lib -lcysec
if [ $? -ne 0 ]; then
	echo "generate test_pkcs7 failed"
	exit 1
fi

if [ $# -ge 1 ]; then
	./test_pkcs7 >> $1
else
	./test_pkcs7
fi

if [ $? -ne 0 ]; then
	echo "execute test_pkcs7 failed"
	exit 1
fi
echo "===== pkcs7ǩ����� ====="

echo "========================="
echo "===== ǩ��ժҪ���� ======"
echo "===== ��Կ�ӿڲ��� ======"
echo "=== �ǶԳƼӽ��ܲ��� ===="
echo "========================="
gcc -g -o2 test_pkey.c test_util.c -o test_pkey -I$CYSEC/include -L$CYSEC/lib -lcysec
if [ $? -ne 0 ]; then
	echo "generate test_pkey failed"
	exit 1
fi

if [ $# -ge 1 ]; then
	./test_pkey >> $1
else
	./test_pkey
fi

if [ $? -ne 0 ]; then
	echo "execute test_pkey failed"
	exit 1
fi
echo "===== ǩ��ժҪ��� ======"
echo "===== ��Կ�ӿ���� ======"
echo "=== �ǶԳƼӽ������ ===="


echo "========================="
echo "===== SCEP�ӿڲ��� ======"
echo "========================="
gcc -g -o2 test_scep_pkcsreq.c test_util.c -o test_scep_pkcsreq -I$CYSEC/include -L$CYSEC/lib -lcysec
if [ $? -ne 0 ]; then
	echo "generate test_scep_pkcsreq failed"
	exit 1
fi

if [ $# -ge 1 ]; then
	./test_scep_pkcsreq >> $1
else
	./test_scep_pkcsreq
fi

if [ $? -ne 0 ]; then
	echo "execute test_scep_pkcsreq failed"
	exit 1
fi
echo "===== SCEP�ӿ���� ======"

echo "========================="
echo "===== SCEP�ӿڲ��� ======"
echo "========================="
gcc -g -o2 test_scep_renewalreq.c test_util.c -o test_scep_renewalreq -I$CYSEC/include -L$CYSEC/lib -lcysec
if [ $? -ne 0 ]; then
	echo "generate test_scep_renewalreq failed"
	exit 1
fi

if [ $# -ge 1 ]; then
	./test_scep_renewalreq >> $1
else
	./test_scep_renewalreq
fi

if [ $? -ne 0 ]; then
	echo "execute test_scep_renewalreq failed"
	exit 1
fi
echo "===== SCEP�ӿ���� ======"


echo "========================="
echo "===== OCSP�ӿڲ��� ======"
echo "========================="
gcc -g -o2 test_ocsp.c test_util.c -o test_ocsp -I$CYSEC/include -L$CYSEC/lib -lcysec
if [ $? -ne 0 ]; then
	echo "generate test_ocsp failed"
	exit 1
fi

if [ $# -ge 1 ]; then
	./test_ocsp >> $1
else
	./test_ocsp
fi

if [ $? -ne 0 ]; then
	echo "execute test_ocsp failed"
	exit 1
fi
echo "===== OCSP�ӿ���� ======"

echo "========================="
echo "====== TLS���Ӳ��� ======"
echo "========================="
gcc -g -o2 test_tls.c test_util.c -o test_tls -I$CYSEC/include -L$CYSEC/lib -lcysec



echo "========================="
echo "==== ��������ɲ��� ====="
echo "========================="


gcc -g test_csr.c test_util.c -o test_csr -I$CYSEC/include -L$CYSEC/lib -lcysec

gcc -g test_mgr.c test_util.c -o test_mgr -I$CYSEC/include -L$CYSEC/lib -lcysec

gcc -g test_pkey_pair_check.c test_util.c -o test_pkey_pair_check -I$CYSEC/include -L$CYSEC/lib -lcysec

gcc -g test_pkey.c test_util.c -o test_pkey -I$CYSEC/include -L$CYSEC/lib -lcysec


gcc -g tls.c test_util.c -o tls -I$CYSEC/include -L$CYSEC/lib -lcysec

cysec_pkey_gen_rsa �޷�������Կ

cysec_scep_response_decode ����assert�� ����ԭ���޷�ȷ��rsa.scep.certrep.der��sm2.scep.certrep.der��ʲô�ļ�����


gcc -g  tls.c test_util.c -o tls -I$CYSEC/include -L$CYSEC/lib -lcysec

gcc -g  12.c test_util.c -o 12 -I$CYSEC/include -L$CYSEC/lib -lcysec

gcc -g  pkey.c test_util.c -o pkey -I$CYSEC/include -L$CYSEC/lib -lcysec












