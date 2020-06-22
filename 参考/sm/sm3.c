/**
 *	Purpose: 	ʵ�ֹ���SM3/SCHժҪ�㷨
 *	Version��	1.2
 *	Update:		2010-12-23 created by ������
 *	Update:		2011-03-22 update according to ����
 *	Update:		2011-05-05 rewrite by ������
 *
 *	������㷨�淶�ĵ����Դӹ��ܵ���վ�����أ�
 *	http://www.oscca.gov.cn/Doc/2/News_1199.htm
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "sm3.h"

//#define SM3_DEBUG

/***************************************************************************************
 *
 *	��0����: 
 *	�����õĹ��ߺ���
 *
 ***************************************************************************************/

#ifdef SM3_DEBUG
static void sm3_debug(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vfprintf(stdout, format, ap);
	va_end(ap);
}

void sm3_dump(const char *desc, const unsigned char *data, int data_cb)
{
	return;
	{
   		int i = 0;
   		char line[128] = {0};

		if (desc) {
			sm3_debug("%s[%p/%d]\n", desc, data, data_cb);
		}
		else  {
			sm3_debug("data[%p/%d]\n", data, data_cb);
		}
	    
		for (i =0; i<data_cb; i++) {
			if ( i>0 && i%32 == 0 ) {
				sm3_debug("%s\n", line);
				line[0] = 0;
			}

			_snprintf(line + (i%32) * 3, 4, "%02X ", *(data+i));
		}
		
		sm3_debug("%s\n\n", line);
	}
}
#else
static void sm3_debug(const char *format, ...){}
void sm3_dump(const char *desc, const unsigned char *data, int data_cb) {}
#endif /* SM3_DEBUG */


/***************************************************************************************
 *
 *	��1����: 
 *	���ݡ�SM3�����Ӵ��㷨��ʵ�ָ��½ڶ�����㷨�߼�
 *
 *	!!!! ע�⣬���ݹ淶����������еġ��֡�Ϊ32λ��4�ֽڣ���������ͨ��x86�ܹ������е�16λ��2�ֽڣ�
 *
 ***************************************************************************************/

/*	Win32������Ĭ��ΪС�ˣ�linux��������c�������ж���	*/
#ifdef WIN32
#define __LITTLE_ENDIAN
#endif /* WIN32 */

/*	4.1 ��ʼֵ			*/
static sm3_word_t SM3_IV[8] = {	0x7380166f, \
								0x4914b2b9, \
								0x172442d7, \
								0xda8a0600, \
								0xa96f30bc, \
								0x163138aa, \
								0xe38dee4d, \
								0xb0fb0e4e  };

/*	4.2 ����			*/
#define T(j)			((j >= 0 && j <= 15) ? 0x79cc4519 :((j >= 16 && j <= 63) ? 0x7a879d8a : 0))

/*	4.3 ���������Ӻ���	*/
#define FG(x,y,z)		(x ^ y ^ z)
#define F1(x,y,z)		((x & y) | (y & z) | (x & z))
#define G1(x,y,z)		((x & y) | (~x & z))

#define FF(j,x,y,z)		((j >= 0 && j <= 15) ? FG(x,y,z) :((j >= 16 && j <= 63) ? F1(x,y,z) : 0))

#define GG(j,x,y,z)		((j >= 0 && j <= 15) ? FG(x,y,z) :((j >= 16 && j <= 63) ? G1(x,y,z) : 0))

/*	4.4 �û�����P0		*/
#define P0(x)			(x ^ (sm3_lshift(x,9))  ^ (sm3_lshift(x,17)))
#define P1(x)			(x ^ (sm3_lshift(x,15)) ^ (sm3_lshift(x,23)))




/*	��Ե��ֵ�ѭ������	*/
static sm3_word_t sm3_lshift(sm3_word_t num, unsigned int bits)
{
	return (num >> (32 - bits) | (num << bits));
}

/*	��Ե��ֵĴ�С��ת��	*/
static sm3_word_t sm3_rot(sm3_word_t num)
{
#ifdef __LITTLE_ENDIAN
	int i = 0;
	sm3_word_t num_b = 0;

	unsigned char *ln = (unsigned char *)(&num);
	unsigned char *bn = (unsigned char *)(&num_b);

	for (i = 0; i < 4; i++)
	{
		bn[i] = ln[3-i];
	}

	return num_b;
#else
	return num;
#endif /* __LITTLE_ENDIAN */
}

/*	���������Ĵ�С��ת��	*/
static void sm3_rot_r(const sm3_word_t* in, unsigned int count, sm3_word_t* out)
{
#ifdef __LITTLE_ENDIAN
	unsigned int i = 0;
	for (i = 0; i < count; i++) {
		out[i] = sm3_rot(in[i]);
	}
#else
	memcpy(out, in, count * sizeof(sm3_word_t));
#endif /* __LITTLE_ENDIAN */
}

/**
 *	5.2 ���
 *	
 *	������Ϣm�ĳ���Ϊmbits���ء����Ƚ�����1��ӵ���Ϣ��ĩβ�������k��0��
 *	k������mbits + 1 + k = 448mod512 ����С�ķǸ�������
 *	Ȼ�������һ��64λ���ش����ñ��ش��ǳ���mbits�Ķ����Ʊ�ʾ��
 *	�������Ϣm'�ı��س���Ϊ512�ı�����
 *
 *	��1������Ϣ01100001 01100010 01100011���䳤��mbits=24��k=448-1-24=443�������õ����ش���
 *	                             {423����0}  {64����}
 *	01100001 01100010 01100011 1 00......00  00 ... 000011000
 *	                                         {24�Ķ����Ʊ�ʾ}
 *
 *	��2������Ϣ01100001 01100010 01100011���䳤��mbits=440��k=448-1-440=7�������õ����ش���
 *	                             {7����0}    {64����}
 *	01100001 01100010 01100011 1 00......00  00 ... 110111000
 *	                                         {440�Ķ����Ʊ�ʾ}
 *
 *	��2������Ϣ00000000 ........ 00000000���䳤��mbits=504��k=512+448-1-504=455�������õ����ش���
 *	                             {455����0}  {64����}
 *	00000000 ........ 00000000 1 00......00  00 ... 111111000
 *	                                         {504�Ķ����Ʊ�ʾ}
 */
 
static unsigned int sm3_padding(unsigned int m_bytes, unsigned char* out)
{
	unsigned int k = 0;
	unsigned int m_bits = m_bytes * 8;
	unsigned int mod_bits = m_bits % 512;
	unsigned char *p = NULL;
	
	/*	�������k���ȣ�k = 448mod512 - 1 - mod_bits������Ϊm_bitsΪ8�������������k����Ϊ0	*/
	if (mod_bits <= 447) {
		k = 447 - mod_bits;
	}
	else  {
		k = 512 + 447 - mod_bits;
	}
	
	/*	���δָ���������ֻ���㳤�ȣ��ֽڣ�������	*/
	if (NULL == out) {
		return (m_bits + 1 + k + 64)/8;
	}

	p = out;

	/*	��Ϊ���Ǵ����m_bits����8�ı�������������ֱ����0x80�������1�������	*/
	*p = 0x80;
	p++;

	/*	�ٲ���(k/8)�ֽ�0		*/
	if ( (k/8) > 0 ) {
		memset(p, 0, k/8);
		p += k/8;
	}

	/*	�ٲ���8�ֽ�(64����)���ȣ���m_bytesΪ32λ����£�ǰ4�ֽڹ̶�Ϊ0	*/
	memset(p, 0, 4);
	p += 4;

	*p++ = (unsigned char)((m_bits & 0xFF000000) >> 24);
	*p++ = (unsigned char)((m_bits & 0x00FF0000) >> 16);
	*p++ = (unsigned char)((m_bits & 0x0000FF00) >> 8);
	*p++ = (unsigned char)((m_bits & 0x000000FF));

	sm3_dump("sm3_padding: padding of m", out, p - out);

	/*	�����������Ϣ���ȣ��ֽڣ�����ֵӦ����64�ֽ�(512����)��������	*/
	return p - out;
}

/**
 *	5.3.2 ��Ϣ��չ
 *
 *	����Ϣ����B(i)�����·�����չ����132����W0, W1, ...W67, W��0, W��1, ...W��63
 *	a)	����Ϣ����B(i)����Ϊ16����W0, W1, ...W15
 *	b)	FOR j=16 TO 67
 *			Wj = P1(Wj-16 �� Wj-9 �� (Wj-3 <<< 15)) �� (Wj-13 <<< 7) �� Wj-6
 *		ENDFOR
 *	c)	FOR j=0 TO 63
 *			W��j = Wj �� Wj+4
 *		ENDFOR
 *	
 *	ע1����  ��ʾ32�����������
 *	     <<< ��ʾѭ������k��������
 */
static void sm3_extend(const unsigned char *b, sm3_word_t *w)
{
	unsigned int i = 0;
	unsigned int j = 0;

	sm3_dump("sm3_extend: b", b, 64);
	
	/*	b�ĳ���Ӧ�ù̶�Ϊ16���֣�Ҳ��64�ֽ�	*/
	sm3_rot_r((const sm3_word_t *)b, 16, w);
	
	for (i = 16; i < 68; i++) {
		w[i] = P1((w[i - 16]) ^ (w[i - 9]) ^ (sm3_lshift(w[i - 3],15))) ^ (sm3_lshift(w[i - 13],7))  ^ w[i - 6];
	}

 	for (j = 0; j < 64; j++)
 	{
 		w[68 + j] = w[j] ^ w[j + 4];
 	}

	sm3_dump("sm3_extend: w", (const unsigned char *)w, 68 * sizeof(sm3_word_t));
	sm3_dump("sm3_extend: w'", (const unsigned char *)(w + 68), 64 * sizeof(sm3_word_t));
}

/**
 *	5.3.2 ѹ������
 *	��A,B,C,D,E,F,G,HΪ�ּĴ���, SS1,SS2,TT1,TT2Ϊ�м����,
 *	ѹ������V(i+1) = CF(V(i),B(i)), 0 <= i <= n-1��
 *
 *	��������������£�
 *	ABCDEFGH = V(i)
 *	FOR j=0 TO 63
 *		SS1 = ((A <<< 12) + E + (Tj <<< j)) <<< 7
 *		SS2 = SS1 �� (A <<< 12)
 *		TT1 = FFj(A,B,C) + D + SS2 +W��j
 *		TT2 = GGj(E,F,G) + H + SS1 +Wj
 *		D = C
 *		C = B <<< 9
 *		B = A
 *		A = TT1
 *		H = G
 *		G = F <<< 19
 *		F = E
 *		E = P0(TT2)
 *	ENDFOR
 *	
 *	V(i+1) = ABCDEFGH �� V(i)
 *
 *	ע1����  ��ʾ32�����������
 *	     <<< ��ʾѭ������k��������
 *	ע2���ֵĴ洢Ϊ���(big-endian)��ʽ��
 */
static void sm3_compress(sm3_word_t *v, sm3_word_t *w)
{
	/*	v��vi���̶�Ϊ8���֣�Ҳ��32�ֽ�	*/
	sm3_word_t vi[8] = {0};
	sm3_word_t *A = vi;
	sm3_word_t *B = vi+1;
	sm3_word_t *C = vi+2;
	sm3_word_t *D = vi+3;
	sm3_word_t *E = vi+4;
	sm3_word_t *F = vi+5;
	sm3_word_t *G = vi+6;
	sm3_word_t *H = vi+7;
	
	sm3_word_t SS1 = 0;
	sm3_word_t SS2 = 0;
	sm3_word_t TT1 = 0;
	sm3_word_t TT2 = 0;

	unsigned int j = 0;
	
	/*	ABCDEFGH = V(i)	*/
	memcpy(vi, v, sizeof(vi));

	sm3_dump("sm3_compress: v(i)", (const unsigned char *)v, 32);
	
	for (j = 0; j <= 63; j++) {
		/*		SS1 = ((A <<< 12) + E + (Tj <<< j)) <<< 7	*/
		SS1 = sm3_lshift(sm3_lshift(*A, 12) + (*E) + sm3_lshift(T(j), j), 7);
		
		/*		SS2 = SS1 �� (A <<< 12)						*/
		SS2 = SS1 ^ sm3_lshift(*A, 12);
		
		/*		TT1 = FFj(A,B,C) + D + SS2 +W��j			*/
		TT1 = FF(j, *A, *B, *C) + (*D) + SS2 + w[68+j];

		/*		TT2 = GGj(E,F,G) + H + SS1 +Wj				*/
		TT2 = GG(j, *E, *F, *G) + (*H) + SS1 + w[j];
		
		/*		D = C										*/
		*D = *C;
		
		/*		C = B <<< 9									*/
		*C = sm3_lshift(*B, 9);

		/*		B = A										*/
		*B = *A;
		
		/*		A = TT1										*/
		*A = TT1;
		
		/*		H = G										*/
		*H = *G;
		
		/*		G = F <<< 19								*/
		*G = sm3_lshift(*F, 19);
		
		/*		F = E										*/
		*F = *E;

		/*		E = P0(TT2)									*/
		*E = P0(TT2);
	}

	/*	V(i+1) = ABCDEFGH �� V(i)	*/
	for (j = 0; j < 8; j++) {
		v[j] = v[j] ^ vi[j];
	}

	sm3_dump("sm3_compress: v(i+1)", (const unsigned char *)v, 32);
}


/**
 *	5.3.1 ��������
 *	���������Ϣm�䰴64�ֽڣ�512���أ����з��飺m�� = B(0)B(1)...B(n-1)
 *	����n=m_size/512��
 *	��m�䰴���з�ʽ������
 *
 *	FOR i=0 TO n-1
 *		V(i+1) = CF(V(i), B(i))
 *	ENDFOR
 *
 *	����CF��ѹ��������V(0)Ϊ256���س�ʼֵIV��B(i)Ϊ�������Ϣ���飬����ѹ���Ľ��ΪV(n)�� 
 */
static unsigned int sm3_loop(const unsigned char *m, unsigned int m_bytes, sm3_word_t *iv)
{
	unsigned int left_bytes = m_bytes;
	const unsigned char *b = m;
	unsigned int i = 0;
	
	/*	�������Ϣ����(132����)	*/
	sm3_word_t w[132] = {0};

	while (left_bytes > 0) {

		if (left_bytes < 64) {
			/*	���������Ӧ�ó���	*/
			sm3_debug("invalid size: m_bytes=%d, left_bytes=%d\n", m_bytes, left_bytes);
			return 0;
		}

		sm3_extend(b, w);

		sm3_compress(iv, w);
	
		left_bytes -= 64;
		b += 64;
		
		i++;
	}	

	/*	���ص�������	*/
	return i;
}



/***************************************************************************************
 *
 *	��2����: 
 *	���㷨ԭ�Ӳ�����װΪ3��ʽ(init/update/final)�ĵ��ú���
 *
 ***************************************************************************************/

int sm3_init(SM3_CTX *ctx)
{
	memset(ctx, 0, sizeof(SM3_CTX));

	memcpy(ctx->iv, SM3_IV, sizeof(ctx->iv));

	return 1;
}

int sm3_update(SM3_CTX *ctx, const unsigned char *m, unsigned int m_bytes)
{
	unsigned int pm_len = 0;
	const unsigned char *pm = NULL;

	/*	��¼���ݳ��ȣ�������������padding	*/
	ctx->m_size += m_bytes;

	if ( ctx->r_len && (ctx->r_len + m_bytes) >= 64 ) {

		/*	�������ʣ�����ݣ��ҿ��Ժ����������һ���µĿ飬�����ƴ�Ӳ�����	*/
		memcpy(ctx->remain + ctx->r_len, m, 64 - ctx->r_len);
		sm3_loop(ctx->remain, 64, ctx->iv);

		/*	�ƶ�m��ָ�룬���ݼ�m_bytes		*/
		m += (64 - ctx->r_len);
		m_bytes -= (64 - ctx->r_len);

		/*	ʣ��������0						*/
		memset(ctx->remain, 0, sizeof(ctx->remain));
		ctx->r_len = 0;
	}
	
	if (ctx->r_len) {

		/*	ʣ�����ݺ���������Ȼ���������һ���µĿ飬ֻ�ܽ������ݼ������浽remain��	*/
		memcpy(ctx->remain + ctx->r_len, m, m_bytes);
		ctx->r_len += m_bytes;
	}
	else {

		/*	ֻ������뵽�鳤�ȵ����ݣ������ı�����ctx->remain��������������	*/
		pm = m;
		pm_len = m_bytes - (m_bytes % 64);
			
		if (pm_len) {
			sm3_loop(pm, pm_len, ctx->iv);
		}

		/*	����ʣ�����ݵ�remain��	*/
		if (m_bytes > pm_len) {
			memcpy(ctx->remain, pm + pm_len, (m_bytes - pm_len));
			ctx->r_len = (m_bytes - pm_len);
		}
	}

	return 1;
}

int sm3_final(SM3_CTX *ctx, unsigned char *dgst)
{
	unsigned int pm_len = 0;
	
	pm_len = sm3_padding(ctx->m_size, ctx->remain + ctx->r_len);
	pm_len += ctx->r_len;

	sm3_loop(ctx->remain, pm_len, ctx->iv);

	sm3_rot_r(ctx->iv, 8, (sm3_word_t*)dgst);

	sm3_dump("sm3_final: dgst", dgst, sizeof(ctx->iv));

	return sizeof(ctx->iv);
}

int sm3_hash_simple(const unsigned char *m, int m_bytes, unsigned char *dgst)
{
	unsigned char *pm = NULL;
	unsigned int pm_len = 0;
	sm3_word_t iv[8] = {0};

	/*	padding	*/
	pm_len = sm3_padding(m_bytes, NULL);
	pm = (unsigned char *)calloc(m_bytes + pm_len, 1);

	memcpy(pm, m, m_bytes);
	sm3_padding(m_bytes, pm + m_bytes);

	/*	loop	*/
	memcpy(iv, SM3_IV, sizeof(iv));
	sm3_loop(pm, pm_len, iv);

	/*	output	*/
	sm3_rot_r(iv, 8, (sm3_word_t*)dgst);
	sm3_dump("sm3_hash_simple: dgst", dgst, 32);

	if (pm) {
		free(pm);
		pm = NULL;
	}

	return 0;
}

int sm3_hash(const unsigned char* pBuffer, unsigned uLen, unsigned char *dgst)
{
	unsigned char dgst2[32] = {0};

	sm3_dump("buffer", pBuffer, uLen);

	/**
	 *	dgst2 must be:
	 *	debe9ff9 2275b8a1 38604889 c18e5a4d 6fdb70e5 387e5765 293dcba3 9c0c5732
	 */
	if (1) {
		sm3_hash_simple(pBuffer, uLen, dgst2);
	}
	else {
		//*	����ʽ���ã�����Ϊ�ؽ��������ݲ��Ϊ3����	*/
		SM3_CTX ctx = {0};
		sm3_init(&ctx);

		sm3_update(&ctx, pBuffer, 31);
		sm3_update(&ctx, pBuffer + 31, 3);
		sm3_update(&ctx, pBuffer + 31 + 3, uLen - 31 - 3);

		sm3_final(&ctx, dgst2);

		///*	����ʽ���ã�����Ϊ�ؽ��������ݲ��Ϊ2����	*/
		//SM3_CTX ctx = {0};
		//sm3_init(&ctx);

		//sm3_update(&ctx, pBuffer, 68);
		//sm3_update(&ctx, pBuffer + 68, uLen - 68);

		//sm3_final(&ctx, dgst2);


	}
	memcpy(dgst, dgst2, 32);

	sm3_dump("dgst:", dgst, 32);

	return 1;
}

int sm3(const unsigned char* pBuffer, unsigned uLen, unsigned char *dgst)
{
	return sm3_hash(pBuffer, uLen, dgst);
}