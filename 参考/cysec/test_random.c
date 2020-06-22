#include <stdio.h>
#include <cysec.h>
#include "test_util.h"

/**
 * ���������
 * @param  buf ������, ���ڴ�����ɵ������
 * @param  num ������������ֽ���
 * @return     �ɹ� -> 0, ʧ�� -> ����
 */
// CYSEC_RET cysec_random_generate(unsigned char* buf, size_t num);

static void test_random_generate()
{
	int size[] = {16, 64, 256, 1024, 4096};
	int i, ret;
	unsigned char *p = NULL;
	for (i = 0; i < sizeof(size)/sizeof(int); i++)
	{
		p = calloc(size[i], sizeof(unsigned char));
		ret = cysec_random_generate(p, size[i]);
		s_assert((ret == 0), "generate random failed");
		if (p){
			hexdump2(p, size[i]);
			SAFE_FREE(p);
		}	
	}
}

int main()
{
	test_random_generate();
	
	return 0;
}

