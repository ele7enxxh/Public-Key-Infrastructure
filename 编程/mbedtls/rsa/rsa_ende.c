#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mbedtls/config.h"
#include "mbedtls/rsa.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"

#define mbedtls_printf       printf
#define mbedtls_fprintf      fprintf

#define MBEDTLS_RSA_ENCRYPT_ME      0
#define MBEDTLS_RSA_DECRYPT_ME      1

//���Կ���
// #define DEBUG_ENCRYPT
// #define DEBUG_DECRYPT

#define RSA_N   "9292758453063D803DD603D5E777D788" \
                "8ED1D5BF35786190FA2F23EBC0848AEA" \
                "DDA92CA6C3D80B32C4D109BE0F36D6AE" \
                "7130B9CED7ACDF54CFC7555AC14EEBAB" \
                "93A89813FBF3C4F8066D2D800F7C38A8" \
                "1AE31942917403FF4946B0A83D3D3E05" \
                "EE57C6F5F5606FB5D4BC6CD34EE0801A" \
                "5E94BB77B07507233A0BC7BAC8F90F79"

#define RSA_E   "10001"

#define RSA_D   "24BF6185468786FDD303083D25E64EFC" \
                "66CA472BC44D253102F8B4A9D3BFA750" \
                "91386C0077937FE33FA3252D28855837" \
                "AE1B484A8A9A45F7EE8C0C634F99E8CD" \
                "DF79C5CE07EE72C7F123142198164234" \
                "CABB724CF78B8173B9F880FC86322407" \
                "AF1FEDFDDE2BEB674CA15F3E81A1521E" \
                "071513A1E85B5DFA031F21ECAE91A34D"

#define RSA_P   "C36D0EB7FCD285223CFB5AABA5BDA3D8" \
                "2C01CAD19EA484A87EA4377637E75500" \
                "FCB2005C5C7DD6EC4AC023CDA285D796" \
                "C3D9E75E1EFC42488BB4F1D13AC30A57"

#define RSA_Q   "C000DF51A7C77AE8D7C7370C1FF55B69" \
                "E211C2B9E5DB1ED0BF61D0D9899620F4" \
                "910E4168387E3C30AA1E00C339A79508" \
                "8452DD96A9A5EA5D9DCA68DA636032AF"

void printMPI(char * desc, mbedtls_mpi * X)
{
    int i, j, k, index = X->n - 1, tlen = sizeof(mbedtls_mpi_uint);

    mbedtls_printf("%s\n", desc);
    
    for(i = X->n - 1; i >= 0; i--, index--)
        if (X->p[i] != 0)
            break;
    for (i = index, k = 0; i >= 0; i--, k++)
    {
        for (j = tlen - 1; j >= 0; j--)
            mbedtls_printf("%02X", (X->p[i] >> (j << 3)) & 0xFF);
        if (k % 2)
            mbedtls_printf("\n");
    }
    if (k % 2)
        mbedtls_printf("\n");
}   
//KEY_LEN�ĳ����� mbedtls_rsa_context.len �ĳ��ȵ�ͬ
#define KEY_LEN     128

#define mbedtls_err(ret)    \
do{ \
    char errbuf[1024];  \
    mbedtls_strerror(ret, errbuf, 1024);    \
    mbedtls_fprintf(stderr, "%d ret(%02x) : %s\n", __LINE__, ret, errbuf);          \
    goto cleanup;   \
}while(0);

#define USAGE   \
    "\n  ./* <mode> <input filename> <output filename>\n" \
    "\n    <mode>: 'e'=encrypt 'd'=decrypt\n" \
    "\n    input file and output file should not use the same name\n" \
    "\n"

/** 
    ʹ��rsa���ܻ����ע�����
    1.mbedtls_rsa_context.len�Ĵ�С��mbedtls_rsa_context.N�ĳ��ȣ����ߵĳ�������Կλ�����
    2.���ܻ����ʱ��������ļ��зֶζ�ȡ���ݽ��в�����ÿ�����ɼӽ���mbedtls_rsa_context.len
    ���ȵ��ֽڣ���������Կλ��Ϊ1024���������ܳ���Ϊ128�ֽڡ����Լӽ��ܻ������ĳ��ȵ�����Ӧ
    ��С��mbedtls_rsa_context.len
    3.ÿ�ζ����ݼ���ʱ�����ݵ�hex��תΪmbedtls_mpi���ͺ��СӦС��mbedtls_rsa_context.N����
        mbedtls_mpi_read_binary(&T, ibuf, rsa_ctx->len);
        ASSERT(mbedtls_mpi_cmp_mpi(&T, &rsa_ctx->N) < 0);
    ������ÿ�ζ�ȡ(mbedtls_rsa_context-1)�����ֽ����ݣ������ܻ������ֽ���Ϊ0������ʹ�ö�ȡ����
    �ڽ�����䣬�Դ˱�֤��������  
    4.������ܵĳ�������mbedtls_rsa_context.len�����Բ��ý�������3�Ĳ���
    5.������ܵĳ�������mbedtls_rsa_context.len����Ҫʹ�����ģʽ�����ʱ��Ҫʹ�������������
    6.MBEDTLS_RSA_PKCS_V15���ģʽ
    ����mbedtls_rsa_context.N����λ��Ϊ1024����mbedtls_rsa_context.len=128��
    �������ݳ���Ϊ100���ֽڣ�����128���趨һ������buf���������������
        [0][MBEDTLS_RSA_CRYPT][25�ֽڵķ�����������][0][100�ֽڵļ�������]
    ��һ���ֽ���0���ڶ����ֽ�����ܱ�־�����һ�η������ֵ��������0��β����������������
    7.MBEDTLS_RSA_PKCS_V21 ���ģʽ
    ��� MBEDTLS_RSA_PKCS_V15, MBEDTLS_RSA_PKCS_V21��Ҫָ��һ�ֹ�ϣ�㷨
    ���������ʹ�� MBEDTLS_RSA_PKCS_V21 ���ģʽ��ֻ��Ҫ��
        mbedtls_rsa_init(&rsa_ctx, MBEDTLS_RSA_PKCS_V15, 0);
    �滻Ϊ
        mbedtls_rsa_init(&rsa_ctx, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_MD5);
    ��Ȼ����3����������ѡ��������ϣ����
    8.rsa���� != rsa��ǩ   rsa���� != rsaǩ��
*/
/**
 ���ܣ����ļ�����rsa����
 �����
    PC�¶�һ��28M���ļ����мӽ��ܲ���
        ����ʱ�����(15s)
            real    0m15.121s
            user    0m14.858s
            sys     0m0.169s
        ����ʱ�����(8min)
            real    7m59.919s
            user    7m57.090s
            sys     0m0.392s
    ����rsa���÷ֶζ������ܻ��ƣ�����ʹ�ö��߳̽��мӽ���
 ע�⣺����������PC��ʹ�ô˳���Դ��ļ����мӽ��ܲ��������׶Դ��������
 */
int main(int argc, char * argv[])
{
    int mode, n, flag, ret = 0;
    
    const char *indiv_data = "created by C";    
    FILE *fin = NULL, *fout = NULL;
    off_t filesize, offset;
    
    mbedtls_mpi K;
    mbedtls_rsa_context rsa_ctx;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;

    //ȷ�����뻺�����������ȳ�
    unsigned char ibuf[KEY_LEN];
    unsigned char obuf[KEY_LEN];
    
    //����У������ȡ
    if (argc < 4)
    {
        mbedtls_printf(USAGE);
        return -1;
    }
    
    mode = strncmp(argv[1], "e", 1) == 0 ? MBEDTLS_RSA_ENCRYPT_ME : 
                (strncmp(argv[1], "d", 1) == 0 ? MBEDTLS_RSA_DECRYPT_ME : -1);
    if(mode != MBEDTLS_RSA_ENCRYPT_ME && mode != MBEDTLS_RSA_DECRYPT_ME)
    {
        mbedtls_printf(USAGE);
        return -1;
    }
    
    if ((fin = fopen(argv[2], "rb")) == NULL)
    {
        mbedtls_fprintf(stderr, "fopen(%s,rb) failed\n", argv[2]);
        goto cleanup;
    }
    if ((fout = fopen(argv[3], "wb+")) == NULL)
    {
        mbedtls_fprintf(stderr, "fopen(%s,wb+) failed\n", argv[3]);
        goto cleanup;
    }
    if ((filesize = lseek(fileno(fin), 0, SEEK_END)) < 0)
    {
        perror("lseek");
        goto cleanup;
    }
    if (fseek(fin, 0, SEEK_SET) < 0)
    {
        mbedtls_fprintf(stderr, "fseek(0,SEEK_SET) failed\n");
        goto cleanup;
    }
    
    mbedtls_mpi_init(&K);
    //������ȡ��Դ����Ϊα�����������������
    mbedtls_entropy_init(&entropy);
    //����aes���������㷨��α�����������
    mbedtls_ctr_drbg_init(&ctr_drbg);
    //�������̫�̣�����С��KEY_LEN���ֽڣ����� MBEDTLS_RSA_PKCS_V15 ģʽ���
    mbedtls_rsa_init(&rsa_ctx, MBEDTLS_RSA_PKCS_V15, 0);
    // MBEDTLS_RSA_PKCS_V21 ���ģʽ�´�
    // mbedtls_rsa_init(&rsa_ctx, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_MD5);
    //������Դ����α���������
    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                 (const unsigned char *)indiv_data, strlen(indiv_data))) != 0)
        goto cleanup;
    //��ɴ������� N, P, Q�Ĺ�ϵ N=P��Q
    if ((ret = mbedtls_mpi_read_string(&K, 16, RSA_N)) != 0)
        mbedtls_err(ret);
    if ((ret = mbedtls_rsa_import(&rsa_ctx, &K, NULL, NULL, NULL, NULL)) != 0)
        mbedtls_err(ret);
    if ((ret = mbedtls_mpi_read_string(&K, 16, RSA_P)) != 0)
        mbedtls_err(ret);
    if ((ret = mbedtls_rsa_import(&rsa_ctx, NULL, &K, NULL, NULL, NULL)) != 0)
        mbedtls_err(ret);
    if ((ret = mbedtls_mpi_read_string(&K, 16, RSA_Q)) != 0)
        mbedtls_err(ret);
    if ((ret = mbedtls_rsa_import(&rsa_ctx, NULL, NULL, &K, NULL, NULL)) != 0)
        mbedtls_err(ret);
    if ((ret = mbedtls_mpi_read_string(&K, 16, RSA_D)) != 0)
        mbedtls_err(ret);
    if ((ret = mbedtls_rsa_import(&rsa_ctx, NULL, NULL, NULL, &K, NULL)) != 0)
        mbedtls_err(ret);
    if ((ret = mbedtls_mpi_read_string(&K, 16, RSA_E)) != 0)
        mbedtls_err(ret);
    if ((ret = mbedtls_rsa_import(&rsa_ctx, NULL, NULL, NULL, NULL, &K)) != 0)
        mbedtls_err(ret);
    if ((ret = mbedtls_rsa_complete(&rsa_ctx)) != 0)
        mbedtls_err(ret);
    //��˽Կ���
    if ((ret = mbedtls_rsa_check_pubkey(&rsa_ctx)) != 0 || (ret = mbedtls_rsa_check_privkey(&rsa_ctx)) != 0)
        mbedtls_err(ret);
#if defined(DEBUG_ENCRYPT) || defined(DEBUG_DECRYPT)
    mbedtls_mpi T;
    mbedtls_mpi_init(&T);
#endif
    if (mode == MBEDTLS_RSA_ENCRYPT_ME)
    {
        for (offset = 0; offset < filesize; offset += KEY_LEN - 1)
        {
            n = (filesize - offset > KEY_LEN - 1) ? KEY_LEN - 1 : (int)(filesize - offset);
            //����
            if (n == KEY_LEN - 1)
            {
                ibuf[0] = 0;        //ȷ�����Ķ�ֵС��rsa_ctx.N
                if (fread(ibuf + 1, 1, n, fin) != (size_t)n)
                {
                    mbedtls_fprintf(stderr, "fread(%d bytes) failed\n", n);
                    goto cleanup;
                }
                if ((ret = mbedtls_rsa_public(&rsa_ctx, ibuf, obuf)) != 0)
                    mbedtls_err(ret);
            }
            else
            {
                if (fread(ibuf, 1, n, fin) != (size_t)n)
                {
                    mbedtls_fprintf(stderr, "fread(%d bytes) failed\n", n);
                    goto cleanup;
                }
#ifdef DEBUG_DECRYPT
                if (mbedtls_mpi_read_binary(&T, ibuf, n) != 0)
                    fprintf(stderr, "%d error occured\n", __LINE__);
                printMPI("en fread T: ", &T);
#endif
                //ע�⣡����������������ʹ��ͬһ������
                if ((ret = mbedtls_rsa_pkcs1_encrypt(&rsa_ctx, mbedtls_ctr_drbg_random, 
                                                    &ctr_drbg, MBEDTLS_RSA_PUBLIC, n, ibuf, obuf)) != 0)
                    mbedtls_err(ret);
            }
            if (fwrite(obuf, 1, KEY_LEN, fout) != KEY_LEN)
            {
                mbedtls_fprintf(stderr, "fwrite(%d bytes) failed\n", rsa_ctx.len);
                goto cleanup;
            }
#ifdef DEBUG_ENCRYPT
            if (mbedtls_mpi_read_binary(&T, obuf, rsa_ctx.len) != 0)
                fprintf(stderr, "%d error occured\n", __LINE__);
            printMPI("fwrite T: ", &T);
#endif
        }
    }
    else
    {
        //���ļ����Ƚ���У��
        if (filesize % KEY_LEN != 0)
        {
            mbedtls_fprintf(stderr, "%d filesize(%ld) KEY_LEN(%d)", __LINE__, filesize, KEY_LEN);
            goto cleanup;
        }
        for (offset = 0; offset < filesize; offset += KEY_LEN)
        {
            if (fread(ibuf, 1, KEY_LEN, fin) != (size_t)KEY_LEN)
            {
                mbedtls_fprintf(stderr, "fread(%d bytes) failed\n", KEY_LEN);
                goto cleanup;
            }
#ifdef DEBUG_ENCRYPT
            if (mbedtls_mpi_read_binary(&T, ibuf, rsa_ctx.len) != 0)
                fprintf(stderr, "%d error occured\n", __LINE__);
            printMPI("fread T: ", &T);
#endif
            //δ���ļ�ĩβ
            if (offset + KEY_LEN != filesize)
            {
                if ((ret = (mbedtls_rsa_private(&rsa_ctx, mbedtls_ctr_drbg_random, &ctr_drbg, ibuf, obuf))) != 0)
                    mbedtls_err(ret);
                if (fwrite(obuf + 1, 1, KEY_LEN - 1, fout) != (size_t)(KEY_LEN - 1))
                {
                    mbedtls_fprintf(stderr, "fwrite(%d bytes) failed\n", KEY_LEN - 1);
                    goto cleanup;
                }
            }
            else
            {
                if ((ret = (mbedtls_rsa_pkcs1_decrypt(&rsa_ctx, mbedtls_ctr_drbg_random, 
                                            &ctr_drbg, MBEDTLS_RSA_PRIVATE, (size_t*)&n, ibuf, obuf, sizeof(obuf)))) != 0)
                    mbedtls_err(ret);
                if (fwrite(obuf, 1, n, fout) != (size_t)n)
                {
                    mbedtls_fprintf(stderr, "fwrite(%d bytes) failed\n", n);
                    goto cleanup;
                }
#ifdef DEBUG_DECRYPT
                if (mbedtls_mpi_read_binary(&T, obuf, n) != 0)
                    fprintf(stderr, "%d error occured\n", __LINE__);
                printMPI("de fwrite T: ", &T);
#endif
            }
        }
    }
    
cleanup:
    if (fin) fclose(fin);
    if (fout) fclose(fout);
    
    mbedtls_mpi_free(&K);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    mbedtls_rsa_free(&rsa_ctx);
    
    return 0;
}