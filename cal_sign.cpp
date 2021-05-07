/*!
	用于实现移动集团上网日志SY接口的过滤与监控请求接收
	用法如下：
	\Author genghb
	\Date	2017.3.20
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <map>
#include <sys/types.h>
#include <sys/socket.h>
#include <libgen.h>
#include <pthread.h>
#include "IBcomlog.h"
#include "ErrLog.h"
#include "SocketFunc.h"
#include "mystrutil.h"
#include <string>
#include <openssl/md5.h>

#define HERE ErrLog(__FILE__,__LINE__,"HERE");
const char *REQUEST_URL="/api/userlog/subscription";

using namespace std;

/**
 * 核查appkey与sign是否合法
 * \retval -1 验证失败
 * \retval 2 已有过滤配置
 */
static int checkAppKeyAndSign(const char*timestamp, const char *nonce, const char *appsecret)
{
	char md5in[256], md5[32], md5str[64];

//	fprintf(stderr, "timestamp=[%s] nonce=[%s] appsecret=[%s]", timestamp, nonce, appsecret);

	// 验证sign

	//MD5("appsecret="+x-appsecret+"~time="+x-timestamp+"~nonce="+x-nonce+"~path="+path) 
	sprintf(md5in, "appsecret=%s~time=%s~nonce=%s~path=%s", appsecret, timestamp, nonce, REQUEST_URL);
	MD5((const uint8_t *)md5in, strlen(md5in), (uint8_t *)md5);
	
	char *p1 = md5str;
	for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
	{
		p1 += sprintf(p1, "%02X", (uint8_t)md5[i]);
	}

//	fprintf(stderr, "md5in=[%s] md5str=[%s]", md5in, md5str);

	fprintf(stdout, "%s", md5str);
	
	return 0;
}

int main(int argc, char **argv)
{
	if (argc != 4)
	{
		printf("usage:%s <timestamp> <nonce> <appsecret>\n", argv[0]);
		exit(-1);
	}

	checkAppKeyAndSign(argv[1], argv[2], argv[3]);

	return 0;
}
