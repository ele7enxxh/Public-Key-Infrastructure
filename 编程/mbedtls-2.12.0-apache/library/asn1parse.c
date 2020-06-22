/*
 *  Generic ASN.1 parsing
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_ASN1_PARSE_C)

#include "mbedtls/asn1.h"
#include "mbedtls/platform_util.h"

#include <string.h>

#if defined(MBEDTLS_BIGNUM_C)
#include "mbedtls/bignum.h"
#endif

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdlib.h>
#define mbedtls_calloc    calloc
#define mbedtls_free       free
#endif

/*
 * ASN.1 DER decoding routines
 */
 /**
	���ȴ������Ϊ���ͺͳ������ַ�ʽ
	1. ���ͳ��ȴ�����
	   �� ���������ݳ���С�� 127 �����ͣ�
	   �� ����ֻ��һ���ֽڣ�
	   �� �� 8 λ��Ϊ 0������ 7 λ��䳤�ȵ�ֵ�������ݴ��ĳ��ȣ�
	2. ���ͳ��ȴ�����
	   �� ���������ݳ��ȴ��� 127 �����ͣ�
	   �� ������ 2~127 ���ֽ���ɣ�
	   �� �� 1 ���ֽڵĵ� 8 λ��Ϊ 1������ 7 λ��������䳤��ֵ���е��ֽ�����
	   �� �� 2 ���ֽ��Ժ󣨰����� 2 ���ֽڣ����ֽڣ�������ݴ��ĳ���ֵ
 
	���ͳ��ȴ�����ʾ����
		+--------+--------+--------+--...--+--------+----
		|  tag   | length | value  |       | value  | val
		| ��λ�� | ��λ�� | ��λ�� |       | ��λ�� | ��λ
		+--------+--------+--------+--...--+--------+----
	���ͳ��ȴ�����ʾ����
		+--------+--------+--------+--...--+--------+--------+--------+----
		|  tag   | length |   len  |       |   len  |  value | value  | val
		| ��λ�� | ��λ�� | ��λ�� |       | ��λ�� | ��λ�� | ��λ�� | ��λ
		+--------+--------+--------+--...--+--------+--------+--------+----		
  */
int mbedtls_asn1_get_len( unsigned char **p,
                  const unsigned char *end,
                  size_t *len )
{
    if( ( end - *p ) < 1 )
        return( MBEDTLS_ERR_ASN1_OUT_OF_DATA );

    if( ( **p & 0x80 ) == 0 )			// ���ͳ��ȴ�����
        *len = *(*p)++;					// ���ݴ�����
    else		// ���ͳ��ȴ�����
    {
        switch( **p & 0x7F )
        {
        case 1:
            if( ( end - *p ) < 2 )
                return( MBEDTLS_ERR_ASN1_OUT_OF_DATA );

            *len = (*p)[1];		// ���ݴ�����
            (*p) += 2;			// ָ�����ݴ����ֽ�
            break;

        case 2:
            if( ( end - *p ) < 3 )
                return( MBEDTLS_ERR_ASN1_OUT_OF_DATA );

            *len = ( (size_t)(*p)[1] << 8 ) | (*p)[2];
            (*p) += 3;
            break;

        case 3:
            if( ( end - *p ) < 4 )
                return( MBEDTLS_ERR_ASN1_OUT_OF_DATA );

            *len = ( (size_t)(*p)[1] << 16 ) |
                   ( (size_t)(*p)[2] << 8  ) | (*p)[3];
            (*p) += 4;
            break;

        case 4:			// ����̫����һ����������
            if( ( end - *p ) < 5 )
                return( MBEDTLS_ERR_ASN1_OUT_OF_DATA );

            *len = ( (size_t)(*p)[1] << 24 ) | ( (size_t)(*p)[2] << 16 ) |		// ��˴洢
                   ( (size_t)(*p)[3] << 8  ) |           (*p)[4];
            (*p) += 5;
            break;

        default:
            return( MBEDTLS_ERR_ASN1_INVALID_LENGTH );
        }
    }

    if( *len > (size_t) ( end - *p ) )			// ȷ�� *len �����ݴ���ʵ�������
        return( MBEDTLS_ERR_ASN1_OUT_OF_DATA );

    return( 0 );
}
/**
	��ʶ�������Ϊ�ͱ�ʶ����͸߱�ʶ����������ʽ��
	1. �ͱ�ʶ����
	   �� ���������ͱ�ʶֵС�� 30 �����ͣ�
	   �� ������ֻ�� 1 ���ֽڣ�
	   �� ���е� 8 λ�͵� 7 λ��ʾ Class ���ͣ��丳ֵ�������£�
			+------------------+----------+----------++------------------+----------+----------+
			|       Class      |  �� 8 λ |  �� 7 λ ||       Class      |  �� 8 λ |  �� 7 λ |
			+------------------+----------+----------++------------------+----------+----------+
			|     Universal    |    0     |     0    || Context-Specific |     1    |     0    |
			+------------------+----------+----------++------------------+----------+----------+
			|    Application   |    0     |     1    ||       Private    |     1    |     1    |
			+------------------+----------+----------++------------------+----------+----------+
		�� �� 6 λ��Ϊ 0 ��ʾ�ǻ������͵ı��룻
		�� �� 5 λ���� 1 λ����������ͱ�ʶ��ֵ
	2. �߱�ʶ����
	   �� ���������ͱ�ʶֵ���� 30 �����ͣ�
	   �� ������������ 2 ���ֽڣ�
	   �� ���˽��� 5 ���� 1 λ��Ϊ 1����һ���ֽ�����������õı�ʶ���Ĺ�����ͬ��
	   �� ���� 2 ���ֽ��Ժ󣨰����� 2 ���ֽڣ����ֽ�������ͱ�ʶ��ֵ�������� 128��ÿ���ֽڵĵ� 8 λ��
	      Ϊ������־���������һ���ֽ�����������Ϊ 1
 */
int mbedtls_asn1_get_tag( unsigned char **p,
                  const unsigned char *end,
                  size_t *len, int tag )	
{
    if( ( end - *p ) < 1 )
        return( MBEDTLS_ERR_ASN1_OUT_OF_DATA );

    if( **p != tag )			// ����ı�ʶ���봫��ı�ʶ�Ƚ�
        return( MBEDTLS_ERR_ASN1_UNEXPECTED_TAG );

    (*p)++;						// ָ�򳤶ȴ�

    return( mbedtls_asn1_get_len( p, end, len ) );	// p ָ�����ݴ���len ������ݴ�����
}

/** ���¶��Ƕ����ݴ��Ļ�ȡ�����Ͷ���� asn1.h */
/**
	BOOLEAN ����ͨ���ࣺ
		��ʶ��			0x01
		mbedtls ��ʶ��	MBEDTLS_ASN1_BOOLEAN
		���ȣ�			1						DER ������̱������BOOLEAN ֻ�� 1 ���ֽڴ洢
 */
int mbedtls_asn1_get_bool( unsigned char **p,
                   const unsigned char *end,
                   int *val )
{
    int ret;
    size_t len;

    if( ( ret = mbedtls_asn1_get_tag( p, end, &len, MBEDTLS_ASN1_BOOLEAN ) ) != 0 )
        return( ret );

    if( len != 1 )
        return( MBEDTLS_ERR_ASN1_INVALID_LENGTH );

    *val = ( **p != 0 ) ? 1 : 0;		// ���ݴ�ֵ
    (*p)++;								// ָ����һ�� tlv

    return( 0 );
}
/**
	INTEGER ����ͨ���ࣺ
		��ʶ��			0x02
		mbedtls ��ʶ��	MBEDTLS_ASN1_INTEGER
		���ݴ����ȣ�	1 ~ 4
 */
int mbedtls_asn1_get_int( unsigned char **p,
                  const unsigned char *end,
                  int *val )
{
    int ret;
    size_t len;

    if( ( ret = mbedtls_asn1_get_tag( p, end, &len, MBEDTLS_ASN1_INTEGER ) ) != 0 )
        return( ret );

	/**
		���� INTEGER ���ͣ�DER ��������ݴ��� 1 ���ֽڵ� 8 λ��ʾ����������������������
		�� 1 �ֽڵ� 8 λΪ 1 ʱ����ǰ��� 1 ���ֽ� 0x00���� 0x80�������Ϊ 00 80��
		
		mbedtls ��֧�ֶԸ� INTEGER �Ľ������������ݴ��в���Ҫ��� 0x00
	 */
    if( len == 0 || len > sizeof( int ) || ( **p & 0x80 ) != 0 )
        return( MBEDTLS_ERR_ASN1_INVALID_LENGTH );

    *val = 0;

    while( len-- > 0 )
    {
        *val = ( *val << 8 ) | **p;
        (*p)++;
    }

    return( 0 );
}

#if defined(MBEDTLS_BIGNUM_C)
int mbedtls_asn1_get_mpi( unsigned char **p,
                  const unsigned char *end,
                  mbedtls_mpi *X )
{
    int ret;
    size_t len;

    if( ( ret = mbedtls_asn1_get_tag( p, end, &len, MBEDTLS_ASN1_INTEGER ) ) != 0 )
        return( ret );

    ret = mbedtls_mpi_read_binary( X, *p, len );

    *p += len;

    return( ret );
}
#endif /* MBEDTLS_BIGNUM_C */
/**
	BIT STRING ����ͨ���ࣺ
		��ʶ��			0x03
		mbedtls ��ʶ��	MBEDTLS_ASN1_BIT_STRING
		���ȣ�			1 ~ 4
	BIT STRING �����ʽ��
	+--------+--------+-----------+-----------+-----------+----
	|  tag   | length | value for | value for | value for | 
	| octet  | octet  |unused bits| bit string| bit string| ...
	+--------+--------+-----------+-----------+-----------+---
	BIT STRING�� tlv �е� value �У��� 1 λ�洢 unused bits ��������ȷ�������� 7
	��Ϊ value �ĵ� 1 λҪ�洢 unused bits ������ value ����ռ�� 1 ����λ��
	
	length octet = 1 + ���������ݴ�����
	��� BIT STRING Ϊ�գ��� value for unused bits ֵΪ 0
 */
int mbedtls_asn1_get_bitstring( unsigned char **p, const unsigned char *end,
                        mbedtls_asn1_bitstring *bs)
{
    int ret;

    /* Certificate type is a single byte bitstring */
    if( ( ret = mbedtls_asn1_get_tag( p, end, &bs->len, MBEDTLS_ASN1_BIT_STRING ) ) != 0 )
        return( ret );

    /* Check length, subtract one for actual bit string length */
    if( bs->len < 1 )			// ���ݴ�����ռһ����λ��
        return( MBEDTLS_ERR_ASN1_OUT_OF_DATA );
    bs->len -= 1;

    /* Get number of unused bits, ensure unused bits <= 7 */
    bs->unused_bits = **p;	
    if( bs->unused_bits > 7 )
        return( MBEDTLS_ERR_ASN1_INVALID_LENGTH );
    (*p)++;						// ָ�����������ݴ�

    /* Get actual bitstring */
    bs->p = *p;
    *p += bs->len;				// bs->len ���������ݴ��ĳ���

    if( *p != end )
        return( MBEDTLS_ERR_ASN1_LENGTH_MISMATCH );

    return( 0 );
}

/*
 * Get a bit string without unused bits
 */
/**
	����������ģ�
	+--------+--------+-----------+-----------+-----------+----
	|  tag   | length |  sub tag  | sub len   | sub value | 
	| octet  | octet  |   octet   |   octet   |   octet   | ...
	+--------+--------+-----------+-----------+-----------+----
	                  |<-         content octets            ->|
	|<-                     BIT STRING                      ->|
	
 */
int mbedtls_asn1_get_bitstring_null( unsigned char **p, const unsigned char *end,
                             size_t *len )
{
    int ret;

    if( ( ret = mbedtls_asn1_get_tag( p, end, len, MBEDTLS_ASN1_BIT_STRING ) ) != 0 )
        return( ret );

    if( (*len)-- < 2 || *(*p)++ != 0 )
        return( MBEDTLS_ERR_ASN1_INVALID_DATA );

    return( 0 );
}



/*
 *  Parses and splits an ASN.1 "SEQUENCE OF <tag>"
 */
 /**
	һ��ṹ���͵Ľ�����
 	��һ��ASN.1��Ϊ����
		FooQuestion ::= SEQUENCE{
			trackingNumber		INTEGER(0.199),
			question			IA5String
		}
	PDU
		myQuestion FooQuestion ::= {
			trackNumber		5,
			question		"Anybody there?"
		}
	ʹ��UNIVERSAL��ǩ���ͣ������� SEQUENCE OF ���б���(hexadecimal)��
		+-----+------+-----+------+-----+-----+------+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
		| tag |length| tag |length|value|tag  |length|value|value|value|value|value|value|value|value|value|value|value|value|value|value|
		|octet|octet |octet|octet |octet|octet|octet |octet|octet|octet|octet|octet|octet|octet|octet|octet|octet|octet|octet|octet|octet|
		+-----+------+-----+------+-----+-----+------+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
		|10/20|  13  | 02  |  01  | 05  | 16  |  0e  |  41 |  6e |  79 |  62 |  6f |  64 | 79  |  20 |  74 |  68 | 65  |  72 |  65 |  3f |
		+-----+------+-----+------+-----+-----+------+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
									5				   'A'	 'n'   'y'   'b'   'o'   'd'   'y'   ' '   't'   'h'   'e'   'r'   'e'   '?'		
	�㼶��
		FooQuestion			// 1 ��
			trackNumber		// 2 ��
			question		// 2 ��
  */	
int mbedtls_asn1_get_sequence_of( unsigned char **p,
                          const unsigned char *end,
                          mbedtls_asn1_sequence *cur,		// ָ������ĵ�һ���ڵ�
                          int tag)
{
    int ret;
    size_t len;
    mbedtls_asn1_buf *buf;

    /* Get main sequence tag */
    if( ( ret = mbedtls_asn1_get_tag( p, end, &len,
            MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) ) != 0 )		// mbedtls�涨��MBEDTLS_ASN1_CONSTRUCTED �� MBEDTLS_ASN1_SEQUENCE ͬ��
        return( ret );

    if( *p + len != end )		// ���ݴ�����У��
        return( MBEDTLS_ERR_ASN1_LENGTH_MISMATCH );

    while( *p < end )	// ��ѭ��
    {
        buf = &(cur->buf);
        buf->tag = **p;

        if( ( ret = mbedtls_asn1_get_tag( p, end, &buf->len, tag ) ) != 0 )
            return( ret );

        buf->p = *p;
        *p += buf->len;		// ��һ�� mbedtls_asn1_sequence ����ڵ�� tlv

        /* Allocate and assign next pointer */
        if( *p < end )		// mbedtls_asn1_sequence �е� tlv ��ͬ���ġ�Ϊ��չ����Ҫ���� 1 ���ڵ�֮��� mbedtls_asn1_buf �ڶ��з����ڴ�
        {
            cur->next = (mbedtls_asn1_sequence*)mbedtls_calloc( 1,
                                            sizeof( mbedtls_asn1_sequence ) );

            if( cur->next == NULL )
                return( MBEDTLS_ERR_ASN1_ALLOC_FAILED );

            cur = cur->next;
        }
    }

    /* Set final sequence entry's next pointer to NULL */
    cur->next = NULL;

    if( *p != end )
        return( MBEDTLS_ERR_ASN1_LENGTH_MISMATCH );

    return( 0 );
}
/**
	�ض��ṹ���� �㷨�ṹ �Ľ�����
	AlgorithmIdentifier ::= SEQUENCE{
		algorithm		OBJECT IDENTIFIER,
		parameters		ANY DEFINED BY algorithm OPTIONAL		�㷨��������Ϊ NULL, Ҳ���Բ�ֹ 1 ��
	}
	AlgorithmIdentifier����ʾ�⣨��һ���SEQUENCE��ͬ����
	+------+------+------+------+------+------+------+------+------+------+------+------+------+------+------+------+
	| tag  |length|	tag  |length|value |value |value |value | tag  |length|value |value |value |value |value |value |
	|octet |octet |octet |octet |octet |octet |octet |octet |octet |octet |octet |octet |octet |octet |octet |octet |
	+------+------+------+------+------+------+------+------+------+------+------+------+------+------+------+------+
	
	                            |<-   algorithm value     ->|             |<-           parameters value          ->|
	              |<-                             AlgorithmIdentifier value                                       ->|
 */
int mbedtls_asn1_get_alg( unsigned char **p,		// p ָ�� AlgorithmIdentifier �� tlv
                  const unsigned char *end,
                  mbedtls_asn1_buf *alg, mbedtls_asn1_buf *params )		// alg �洢�㷨 oid��params �洢�����㷨���������������Ϊ��
{
    int ret;
    size_t len;

    if( ( ret = mbedtls_asn1_get_tag( p, end, &len,
            MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) ) != 0 )
        return( ret );

    if( ( end - *p ) < 1 )
        return( MBEDTLS_ERR_ASN1_OUT_OF_DATA );
	
    alg->tag = **p;			// p ָ�� algorithm �� tlv
    end = *p + len;			// len �� algorithm �� tlv �� parameters �� tlv ֮��

    if( ( ret = mbedtls_asn1_get_tag( p, end, &alg->len, MBEDTLS_ASN1_OID ) ) != 0 )
        return( ret );

    alg->p = *p;
    *p += alg->len;		// ʹ p ָ�� parameters �� tlv 

    if( *p == end )
    {
        mbedtls_platform_zeroize( params, sizeof(mbedtls_asn1_buf) );
        return( 0 );
    }

    params->tag = **p;
    (*p)++;

    if( ( ret = mbedtls_asn1_get_len( p, end, &params->len ) ) != 0 )
        return( ret );

    params->p = *p;
    *p += params->len;

    if( *p != end )
        return( MBEDTLS_ERR_ASN1_LENGTH_MISMATCH );

    return( 0 );
}

int mbedtls_asn1_get_alg_null( unsigned char **p,		// �㷨�ṹ�в���Ϊ�յ�����
                       const unsigned char *end,
                       mbedtls_asn1_buf *alg )
{
    int ret;
    mbedtls_asn1_buf params;

    memset( &params, 0, sizeof(mbedtls_asn1_buf) );

    if( ( ret = mbedtls_asn1_get_alg( p, end, alg, &params ) ) != 0 )
        return( ret );

    if( ( params.tag != MBEDTLS_ASN1_NULL && params.tag != 0 ) || params.len != 0 )
        return( MBEDTLS_ERR_ASN1_INVALID_DATA );

    return( 0 );
}

void mbedtls_asn1_free_named_data( mbedtls_asn1_named_data *cur )	// �ͷ� ANY DEFINED BY �е�һ���ڵ㣬ֻ��սڵ����ݣ����������������ع�
{
    if( cur == NULL )
        return;

    mbedtls_free( cur->oid.p );
    mbedtls_free( cur->val.p );

    mbedtls_platform_zeroize( cur, sizeof( mbedtls_asn1_named_data ) );
}

void mbedtls_asn1_free_named_data_list( mbedtls_asn1_named_data **head )	// ��ǰ����ͷ� ANY DEFINED BY ��������
{
    mbedtls_asn1_named_data *cur;

    while( ( cur = *head ) != NULL )
    {
        *head = cur->next;
        mbedtls_asn1_free_named_data( cur );
        mbedtls_free( cur );
    }
}
/**
	�������������µ�ASN.1���ͣ����� contentType���������Ӧ�� content
	ContenInfo ::= SEQUENCE{
		contentType		ContentType,
		content		[0]	EXPLICIT ANY DEFINED BY contentType
	}
	ContentType ::= OBJECT IDENTIFIER
 */
mbedtls_asn1_named_data *mbedtls_asn1_find_named_data( mbedtls_asn1_named_data *list,
                                       const char *oid, size_t len )		//oid �� __In, len Ҳ�� __In
{
    while( list != NULL )
    {
        if( list->oid.len == len &&
            memcmp( list->oid.p, oid, len ) == 0 )
        {
            break;
        }

        list = list->next;
    }

    return( list );
}

#endif /* MBEDTLS_ASN1_PARSE_C */
