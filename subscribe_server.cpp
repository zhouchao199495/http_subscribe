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
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <libgen.h>
#include <pthread.h>
#include <openssl/md5.h>
#include "jansson.h"

#include "IBcomlog.h"
#include "ErrLog.h"
#include "SocketFunc.h"
#include "mystrutil.h"
#include "subscribeReq.h"
#include "subscribe_config.h"
#include "httpHeader.h"
#include "httpTools.h"

using namespace std;

#define HERE ErrLog(__FILE__,__LINE__,"HERE");
#define MAX_BUF_SIZE  131072 	// 128k

enum subscribe_retcode{
	RETCODE_SUCCESS = 0,					//!< 成功
	RETCODE_URL_NOT_FOUND,					//!< 请求的URL不存在
	RETCODE_AUTHENTIFICATION_ERROR,			//!< 认证错误
	RETCODE_READ_REQEUST_INFO_ERROR,		//!< 读取请求信息出错
	RETCODE_PARSE_REQUEST_INFO_ERROR,		//!< 解析请求信息出错
	RETCODE_SUBSCRIBE_INFO_EXISTED_ERROR,	//!< 订阅信息已存在
	RETCODE_WRITE_STATIC_DATA_ERROR,		//!< 写入订阅信息失败
	RETCODE_GET_STATIC_DATA_DIR_ERROR,		//!< 获取静态数据目录失败
	RETCODE_DELETE_STATIC_DATA_ERROR,		//!< 删除订阅信息出错
	RETCODE_SUBSCRIBE_NOT_EXISTS,			//!< 订阅信息不存在
	RETCODE_CALL_SLAVE_NODE_ERROR,			//!< 调用从节点失败
	RETCODE_UNKNOWN,						//!< 未知错误
};

typedef struct _subscribe_retinfo{
	enum subscribe_retcode	retcode;
	const char *retinfo;
} subscribe_retinfo;

subscribe_retinfo ret_string_info[] = {
	{ RETCODE_SUCCESS , "success" },
	{ RETCODE_URL_NOT_FOUND , "Url not found!" },
	{ RETCODE_AUTHENTIFICATION_ERROR , "Authentification error!" },
	{ RETCODE_READ_REQEUST_INFO_ERROR , "Read request info fail!" },
	{ RETCODE_PARSE_REQUEST_INFO_ERROR , "Parse request info error!" },
	{ RETCODE_SUBSCRIBE_INFO_EXISTED_ERROR , "Already have subscribe info!" },
	{ RETCODE_WRITE_STATIC_DATA_ERROR , "Write static data error!" },
	{ RETCODE_GET_STATIC_DATA_DIR_ERROR , "Get static_data_dir error!" },
	{ RETCODE_DELETE_STATIC_DATA_ERROR, "delete static data error!" },
	{ RETCODE_SUBSCRIBE_NOT_EXISTS, "Subscribe not exists!" },
	{ RETCODE_CALL_SLAVE_NODE_ERROR, "Call salve node error!" },
	{ RETCODE_UNKNOWN, "Unknown error!" },
};


enum PROCESS_MODE
{
	MASTER_MODE = 1,
	SLAVE_MODE
};

char 	progName[64];
char 	configFile[256];
bool	g_foreground;		
int		g_master_port;
int		g_slave_port;
PROCESS_MODE	g_ProcessMode;
subConfig g_config;
const char *REQUEST_URL="/api/userlog/subscription";

char g_cmd[16], g_url[1024], g_version[32];

static const char *get_ret_string(subscribe_retcode retcode)
{
	for (unsigned int i = 0; i < sizeof(ret_string_info) / sizeof(subscribe_retinfo); i++)
	{
		if (ret_string_info[i].retcode == retcode)
			return ret_string_info[i].retinfo;
	}

	return "Unknown error!";
}

void defaultFunc(FILE *fpCli, FILE *fpCmd)
{
	char	*p1;
	char 	sIn[1024];	
	
	memset(sIn, 0, sizeof(sIn));
	
	ErrLog(__FILE__,__LINE__, "--------------------------------------->>>>>>");

	while ( (p1 = fgets(sIn, sizeof(sIn) - 1, fpCmd)) != NULL)
	{
		fprintf(fpCli, sIn);
		ErrLog(__FILE__,__LINE__, "INFO: [%s]", sIn);
		memset(sIn, 0, sizeof(sIn));
	}
	ErrLog(__FILE__,__LINE__, "<<<<<<---------------------------------------");
}

/*
static void responseBadRequest(FILE *fpOut, string &failInfo)
{
	fprintf(fpOut, "HTTP/1.1 400 Bad Request\r\n");
	fprintf(fpOut, "Content-Length: %lu\r\n", failInfo.size());
	fprintf(fpOut, "Connection: close\r\n");
	fprintf(fpOut, "Content-Type: text/plain; charset=utf-8\r\n");
	fprintf(fpOut, "\r\n");
	fprintf(fpOut, "%s", failInfo.c_str());
}

static void responseNotFound(FILE *fpOut, string &failInfo)
{
	fprintf(fpOut, "HTTP/1.1 404 Not Found\r\n");
	fprintf(fpOut, "Content-Length: %lu\r\n", failInfo.size());
	fprintf(fpOut, "Connection: close\r\n");
	fprintf(fpOut, "Content-Type: text/plain; charset=utf-8\r\n");
	fprintf(fpOut, "\r\n");
	fprintf(fpOut, "%s", failInfo.c_str());
}
*/

static void writeResponse(FILE *fpOut, subscribe_retcode retCode, string &failInfo)
{
	char fail_result[1024];

	sprintf(fail_result, "{ \"retcode\": \"%d\", \"description\": \"%s\" }\n", retCode, failInfo.c_str());

	fprintf(fpOut, "HTTP/1.1 200 OK\r\n");
	fprintf(fpOut, "Content-Length: %lu\r\n", strlen(fail_result));
	fprintf(fpOut, "Connection: close\r\n");
	fprintf(fpOut, "Content-Type: text/plain; charset=utf-8\r\n");

	fprintf(fpOut, "\r\n");
	fprintf(fpOut, "%s", fail_result);
}

static inline int get_file_line_count(char *filename)
{
	FILE *fpIn = fopen(filename, "r");

	if (fpIn == NULL)
		return 0;
	else
	{
		int line_count = 0;
		char *p1;
		char buff[1024];
	
		while ((p1 = fgets(buff, sizeof(buff) - 1, fpIn)) != NULL)
		{
			++line_count;
		}

		fclose(fpIn);

		return line_count;
	}
}

/**
 * 核查appkey与sign是否合法
 * \retval -1 验证失败
 * \retval 2 已有过滤配置
 */
static int checkAppKeyAndSign(httpHeader &header, subConfig &config)
{
	const char *appkey;
	const char *timestamp;
	const char *nonce;
	const char *sign;

	char md5in[256], md5[32], md5str[64];

	appkey = header.get_item("x-appkey");
	timestamp = header.get_item("x-timestamp");
	nonce = header.get_item("x-nonce");
	sign = header.get_item("x-sign");
	
	ErrLog(__FILE__, __LINE__, "appkey=[%s] timestamp=[%s] nonce=[%s] sign=[%s]", appkey, timestamp, nonce, sign);

	const char *appsecret = config.get_appsecret(appkey);
	if (appsecret == NULL)
	{
		ErrLog(__FILE__, __LINE__, "get_appsecret error, interface not exists!");
		return -1;
	}

	// 验证sign

	//MD5("appsecret="+x-appsecret+"~time="+x-timestamp+"~nonce="+x-nonce+"~path="+path) 
	sprintf(md5in, "appsecret=%s~time=%s~nonce=%s~path=%s", appsecret, timestamp, nonce, REQUEST_URL);
	MD5((const uint8_t *)md5in, strlen(md5in), (uint8_t *)md5);
	
	char *p1 = md5str;
	for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
	{
		p1 += sprintf(p1, "%02X", (uint8_t)md5[i]);
	}

	ErrLog(__FILE__, __LINE__, "md5in=[%s] md5str=[%s]", md5in, md5str);

	if (strcasecmp(md5str, sign) != 0)
	{
		ErrLog(__FILE__, __LINE__, "check sign error!");
		return -1;
	}

	// 检查是否已经有过滤配置
	const char *static_dir = config.get_static_data_dir(appkey);
	if (static_dir == NULL)
	{
		ErrLog(__FILE__, __LINE__, "get_static_data_dir error, interface not exists!");
		return -1;
	}

	char filename[256];
	sprintf(filename, "%s/st_compound_filter.dat", static_dir);

	int line_count = get_file_line_count(filename);
	if (line_count > 1)
	{
		//ErrLog(__FILE__, __LINE__, "Already have subscribe data!");
		return 2;	// 已有过滤配置
	}
	
	return 0;
}

// return content_length
static size_t handle_answer_header(FILE *fpIn, httpHeader &header)
{
	char sIn[1024];
	size_t ContentLength = 0;

	memset(sIn, 0, sizeof(sIn));
	while (fgets(sIn, sizeof(sIn) - 1, fpIn) != NULL)
	{
		ErrLog(__FILE__,__LINE__,"INFO: answer header sIn=[%s]", sIn);
		
		if ((sIn[0] == '\r' && sIn[1] == '\n') || sIn[0] == '\n')
		{
			break;
		}
		else
		{
			header.parse(sIn);

			if (strncmp(sIn, "Content-Length: ", 16) == 0 || strncasecmp(sIn, "Content-Length: ", 16) == 0)
			{
				ContentLength = atoi(sIn + 16);
			}
		}
		memset(sIn, 0, sizeof(sIn));
	}

	return ContentLength;
}

static subscribe_retcode parseRetCode(const char *sIn)
{
	subscribe_retcode retCode = RETCODE_UNKNOWN;

	//int ret = 0;
	json_error_t error;
	json_t *root = json_loads(sIn, JSON_DECODE_ANY, &error);

	if (root)
	{ // OBJECT
		ErrLog(__FILE__,__LINE__, "type=[%d]", root->type);	

		const char *key;
		json_t *value;
//		size_t size;

//		size = json_object_size(root);
//		ErrLog(__FILE__, __LINE__, "JSON Object of %ld pair%s:\n", size, json_plural(size));

		json_object_foreach(root, key, value) {
//			ErrLog(__FILE__, __LINE__, "key=[%s] value type=[%d]", key, value->type);

			if (strcmp(key, "retcode") == 0 && json_is_string(value))
			{
				retCode = (subscribe_retcode)(atoi(json_string_value(value)));
			}
		}
	}
	else
	{
		ErrLog(__FILE__, __LINE__, "ERROR: parse [%s] error as line %d: %s", sIn, error.line, error.text );
	}

	return(retCode);
}


static subscribe_retcode call_single_slave(const char *slave_ip, const char *appkey, const char *timestamp, const char *nonce, const char *sign, const char *req_data)
{
	subscribe_retcode retCode = RETCODE_SUCCESS;
	char curl_cmd[MAX_BUF_SIZE];

	sprintf(curl_cmd, "curl --connect-timeout 5 -i -X %s 'http://%s:%d/api/userlog/subscription' \\"
		"-H 'x-appkey:%s' \\"
		"-H 'x-timestamp:%s' \\"
		"-H 'x-nonce:%s' \\"
		"-H 'x-sign:%s' \\"
		"--data '%s'",
		g_cmd, slave_ip, g_slave_port,
		appkey, timestamp, nonce, sign, req_data);

	ErrLog(__FILE__, __LINE__, "INFO: call slave=[curl --connect-timeout 5 -i -X %s 'http://%s:%d/api/userlog/subscription]", g_cmd, slave_ip, g_slave_port);
	
	FILE *fpCmd = popen(curl_cmd, "r");
	if (fpCmd != NULL)
	{
		char buff[1024];

		httpHeader AnsHeader;

		size_t ContentLength = handle_answer_header(fpCmd, AnsHeader);

		if (ContentLength > 0 && ContentLength < sizeof(buff))
		{
			int nTry = 0;
			size_t total = 0;
			int nr;

			memset(buff, 0, sizeof(buff));
			while (total < ContentLength)
			{
				nr = fread(buff + total, 1, ContentLength - total, fpCmd); 
				if (nr == 0)
				{
					if (feof(fpCmd))
					{
						clearerr(fpCmd);
						break;
					}
					else
					{
						if (nTry > 3)
							break;
		
						nTry++;
						clearerr(fpCmd);
						usleep(10000);
					}
				}
				else
				{
					nTry = 0;
					total += nr;
				}
			}
		
			ErrLog(__FILE__,__LINE__, "INFO: read Content=[%d][%d][%s]", ContentLength, total, buff);
			retCode = parseRetCode(buff);
		}
		else
		{
			retCode = RETCODE_PARSE_REQUEST_INFO_ERROR;	
		}
	}
	else
	{
		retCode = RETCODE_CALL_SLAVE_NODE_ERROR;
		ErrLog(__FILE__,__LINE__,"popen error[%s]", curl_cmd);
	}

	if (fpCmd != NULL)
		pclose(fpCmd);

	if (retCode != RETCODE_SUCCESS)
		ErrLog(__FILE__, __LINE__, "ERROR: call_single_slave %s:%s", slave_ip, get_ret_string(retCode));

	return retCode;
}

/**
 * 调用从服务器
 */
static subscribe_retcode call_slave(httpHeader &header, const char *req_data, char *errmsg)
{
	subscribe_retcode ret = RETCODE_SUCCESS;
	char ip_list[1024];
	memset(ip_list, 0, sizeof(ip_list));

	const char *appkey = header.get_item("x-appkey");
	const char *timestamp = header.get_item("x-timestamp");
	const char *nonce = header.get_item("x-nonce");
	const char *sign = header.get_item("x-sign");
	const char *slave_ip_list = g_config.get_slave_ip_list(appkey);

	strncpy(ip_list, slave_ip_list, sizeof(ip_list) - 1);

	errmsg[0] = 0;

	if (ip_list[0] != 0)
	{
		char *saveptr1, *slave_ip;

		// 先解析到数组中
		slave_ip = mystrtok_r(ip_list, ";", &saveptr1);
		if (slave_ip != NULL)
		{
			do
			{
				ret = call_single_slave(slave_ip, appkey, timestamp, nonce, sign, req_data);
				if (ret != RETCODE_SUCCESS)
				{
					ErrLog(__FILE__,__LINE__,"ERROR: call slave [%s] error[%d][%s]", slave_ip, ret, get_ret_string(ret));
					sprintf(errmsg, "%s:%s", slave_ip, get_ret_string(ret));
					break;
				}
				else
					ErrLog(__FILE__,__LINE__,"INFO: call slave [%s] success", slave_ip);

			} while ((slave_ip = mystrtok_r(NULL, ":", &saveptr1)) != NULL);
		}
	}
	
	return ret;
}

/*
	处理输入请求	
*/
static int input_handler(int fd)
{
	char *pIn;
	char *sIn = NULL;
	int ret = 0;
	string failInfo = "";
	subscribe_retcode retCode = RETCODE_SUCCESS;
	httpHeader header;

	FILE *fpIn = fdopen(fd, "w+");
	if (fpIn == NULL)
	{
		ErrLog(__FILE__,__LINE__,"fdopen error");
		close(fd);
		ret = -1;
		goto ERR;
	}
	
	sIn = (char *)malloc(MAX_BUF_SIZE);	// 128K
	if (sIn == NULL)
	{
		goto ERR;
	}

	pIn = fgets(sIn, MAX_BUF_SIZE - 1, fpIn);
	if (pIn != NULL)
	{
		//ErrLog(__FILE__,__LINE__,"INFO: sIn=[%s]", sIn);

		if (strncmp(sIn, "DELETE ", 7) == 0)
		{ // 删除订阅
			//char cmd[16], url[1024], version[32];

			sscanf(sIn, "%s %1024s %32s", g_cmd, g_url, g_version); 

			ErrLog(__FILE__, __LINE__, "INFO: cmd=[%s] url=[%s] version=[%s]\n", g_cmd, g_url, g_version);
			
			// 调用handle_post获取header和content
			ret = handle_post(fpIn, sIn, MAX_BUF_SIZE, header);
			if (ret == 0)
			{
				if (strcmp(g_url, REQUEST_URL) == 0)
				{
					const char *appkey = header.get_item("x-appkey");
					const char *static_dir = g_config.get_static_data_dir(appkey);
					const char *slave_ip_list = g_config.get_slave_ip_list(appkey);

					// 检查合法性
					ret = checkAppKeyAndSign(header, g_config);
					if (ret < 0)
					{
						ErrLog(__FILE__, __LINE__, "ERROR: checkAppKeyAndSign error!");

						retCode = RETCODE_AUTHENTIFICATION_ERROR;
						failInfo = get_ret_string(retCode);
						writeResponse(fpIn, retCode, failInfo);
					}
					else if (ret == 2)
					{ // 2 表示已有订阅消息
						subscribeReq req;
						

						ret = req.clear_static_file(static_dir);
						if (ret == 0)
						{
							retCode = RETCODE_SUCCESS;
							failInfo = get_ret_string(RETCODE_SUCCESS);

							// call slave
							if (g_ProcessMode == MASTER_MODE && slave_ip_list[0] != 0)
							{
								char errmsg[4096];

								retCode = call_slave(header, sIn, errmsg);
								if (retCode == RETCODE_SUCCESS)
									failInfo = get_ret_string(RETCODE_SUCCESS);
								else
									failInfo = errmsg;

								ErrLog(__FILE__, __LINE__, "failInfo=[%s]", failInfo.c_str());
							}

							writeResponse(fpIn, retCode, failInfo);
						}
						else
						{
							retCode = RETCODE_DELETE_STATIC_DATA_ERROR;
							failInfo = get_ret_string(retCode);

							ErrLog(__FILE__,__LINE__, "ERROR: %s", failInfo.c_str());

							writeResponse(fpIn, retCode, failInfo);
						}
					}
					else
					{
					/*
						retCode = RETCODE_SUBSCRIBE_NOT_EXISTS;
						failInfo = get_ret_string(retCode);

						ErrLog(__FILE__,__LINE__, "ERROR: %s", failInfo.c_str());

						writeResponse(fpIn, retCode, failInfo);
					*/
						// 没有订阅信息，直接返回成功
						retCode = RETCODE_SUCCESS;
						failInfo = get_ret_string(retCode);

						// call slave
						if (g_ProcessMode == MASTER_MODE && slave_ip_list[0] != 0)
						{
							char errmsg[4096];

							retCode = call_slave(header, sIn, errmsg);
							if (retCode == RETCODE_SUCCESS)
								failInfo = get_ret_string(RETCODE_SUCCESS);
							else
								failInfo = errmsg;

							ErrLog(__FILE__, __LINE__, "failInfo=[%s]", failInfo.c_str());
						}

						writeResponse(fpIn, retCode, failInfo);
					}
				}
				else
				{
					retCode = RETCODE_URL_NOT_FOUND;
					failInfo = get_ret_string(retCode);

					ErrLog(__FILE__,__LINE__, "ERROR: %s", failInfo.c_str());

					writeResponse(fpIn, retCode, failInfo);
					
				}
			}
		}
		else if (strncmp(sIn, "POST ", 5) == 0)
		{ // 下发订阅
			//char cmd[16], url[1024], version[32];

			sscanf(sIn, "%s %1024s %32s", g_cmd, g_url, g_version); 

			ErrLog(__FILE__, __LINE__, "INFO: cmd=[%s] url=[%s] version=[%s]\n", g_cmd, g_url, g_version);

			// 调用handle_post获取header和content
			ret = handle_post(fpIn, sIn, MAX_BUF_SIZE, header);	
			if (ret == 0)
			{
				if (strcmp(g_url, REQUEST_URL) == 0)
				{
					// 检查合法性
					ret = checkAppKeyAndSign(header, g_config);
					if (ret < 0)
					{
						ErrLog(__FILE__, __LINE__, "ERROR: checkAppKeyAndSign error!");

						retCode = RETCODE_AUTHENTIFICATION_ERROR;
						failInfo = get_ret_string(retCode);
						writeResponse(fpIn, retCode, failInfo);
					}
					else if (ret == 2)
					{ // 2 表示已有订阅消息
						retCode = RETCODE_SUBSCRIBE_INFO_EXISTED_ERROR;
						failInfo = get_ret_string(retCode);

						ErrLog(__FILE__,__LINE__, "ERROR: %s", failInfo.c_str());

						writeResponse(fpIn, retCode, failInfo);
					}
					else
					{
						//ErrLog(__FILE__, __LINE__, "INFO: sIn=[%s]", sIn);
						subscribeReq req;

						if (req.parse(sIn) == 0)
						{
							const char *appkey = header.get_item("x-appkey");
							const char *static_dir = g_config.get_static_data_dir(appkey);
							const char *slave_ip_list = g_config.get_slave_ip_list(appkey);

							if (static_dir == NULL)
							{
								ErrLog(__FILE__, __LINE__, "get_static_data_dir error, interface not exists!");

								retCode = RETCODE_GET_STATIC_DATA_DIR_ERROR;
								failInfo = get_ret_string(retCode);

								ErrLog(__FILE__,__LINE__, "ERROR: %s", failInfo.c_str());

								writeResponse(fpIn, retCode, failInfo);
							}
							else
							{
								ret = req.dump(static_dir);	
								if (ret == 0)
								{
									retCode = RETCODE_SUCCESS;
									failInfo = get_ret_string(RETCODE_SUCCESS);

									// call slave
									if (g_ProcessMode == MASTER_MODE && slave_ip_list[0] != 0)
									{
										char errmsg[4096];
		
										retCode = call_slave(header, sIn, errmsg);
										if (retCode == RETCODE_SUCCESS)
											failInfo = get_ret_string(RETCODE_SUCCESS);
										else
											failInfo = errmsg;
		
										ErrLog(__FILE__, __LINE__, "failInfo=[%s]", failInfo.c_str());
									}

									writeResponse(fpIn, retCode, failInfo);
								}
								else
								{
									retCode = RETCODE_WRITE_STATIC_DATA_ERROR;
									failInfo = get_ret_string(retCode);

									ErrLog(__FILE__,__LINE__, "ERROR: %s", failInfo.c_str());

									writeResponse(fpIn, retCode, failInfo);
								}
							}
						}
						else
						{
							retCode = RETCODE_PARSE_REQUEST_INFO_ERROR;
							failInfo = get_ret_string(retCode);

							ErrLog(__FILE__,__LINE__, "ERROR: %s", failInfo.c_str());

							writeResponse(fpIn, retCode, failInfo);
						}
					}
				}
				else
				{
					retCode = RETCODE_URL_NOT_FOUND;
					failInfo = get_ret_string(retCode);

					ErrLog(__FILE__,__LINE__, "ERROR: %s", failInfo.c_str());

					writeResponse(fpIn, retCode, failInfo);
				}
	
				ErrLog(__FILE__,__LINE__,"failInfo=[%s]", failInfo.c_str());
			}
			else
			{
				retCode = RETCODE_READ_REQEUST_INFO_ERROR;
				failInfo = get_ret_string(retCode);

				ErrLog(__FILE__,__LINE__, "ERROR: %s", failInfo.c_str());

				writeResponse(fpIn, retCode, failInfo);
			}
		}
		else if (strncmp(sIn, "GET ", 4) == 0)
		{
			// 只打印日志
			handle_get_log_only(fpIn);	
		}
	}

ERR:
	if (sIn != NULL)
		free(sIn);
	if (fpIn != NULL)
		fclose(fpIn);
	
	return (ret);
}

typedef struct thread_arg
{
	int fd;
}thread_arg_t;

void *handler_thread(void *arg)
{
	thread_arg_t *thread_arg = (thread_arg_t *)arg;
	int fd = thread_arg->fd;

	input_handler(fd);

	return NULL;
}

#include <netinet/in.h>
static inline int my_getpeername(int sockfd, char *name)
{
	int ret;
	struct sockaddr_in6 addr;
	socklen_t socklen = sizeof(addr);

	ret = getpeername(sockfd, (struct sockaddr *)&addr, &socklen);
	if (ret != 0)
	{
		fprintf(stderr, "getpeername error[%s]\n", strerror(errno));
		return -1;
	}

	if(socklen == sizeof(struct sockaddr_in6))
	{
		struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)&addr;
		int i;

		for (i = 0; i < 8; i++)
		{
			inet_ntop(AF_INET6, &in6->sin6_addr, name, socklen);
		}
	}
	else if (socklen == sizeof(struct sockaddr_in))
	{
		struct sockaddr_in *in4 = (struct sockaddr_in *)&addr;
		inet_ntop(AF_INET, &in4->sin_addr, name, socklen);
	}

	return 0;
}

int main(int argc, char **argv)
{
	int			iListenSocket, iAcceptedSocket;
	struct 		sockaddr eCliAddr;

	if (argc != 5 && argc != 6)
	{
		printf("usage:%s <master|slave> <master_port> <slave_port> <config_file> [-fg]\n", argv[0]);
		exit(-1);
	}

	memset(progName, 0, sizeof(progName));
	mystrncpy(progName, basename(argv[0]), sizeof(progName));

	if (strcmp(argv[1], "master") == 0)
		g_ProcessMode = MASTER_MODE;
	else if (strcmp(argv[1], "slave") == 0)
		g_ProcessMode = SLAVE_MODE;
	else	
		fprintf(stderr, "Unknown mode, exit!!\n");

	g_master_port = atoi(argv[2]);
	g_slave_port = atoi(argv[3]);
	mystrncpy(configFile, argv[4], sizeof(configFile));

	if (argc == 6 && strcmp(argv[5], "-fg") == 0)
		g_foreground = true;
	else
		g_foreground = false;

	// 设置日志文件名称
	char sLogFile[128];
	sprintf(sLogFile, "%s.log", progName);
	IBcomlogSetFilename(sLogFile);

	if (!g_foreground)
	{
		// 父进程退出
		if (fork() != 0)
			exit(0);

		// 子进程
		setsid();
		// signal(SIGTERM,Term_signal);
		signal ( SIGINT, SIG_IGN );
		signal ( SIGPIPE, SIG_IGN );
		signal ( SIGQUIT, SIG_IGN );
		signal ( SIGHUP, SIG_IGN );
		signal ( SIGCHLD, SIG_IGN );
	}

	ErrLog(__FILE__,__LINE__,"INFO: configFile[%s]", configFile);
	g_config.load(configFile);

	// 初始化监听Socket
	if (g_ProcessMode == MASTER_MODE)
		iListenSocket = SocketInit(g_master_port, INADDR_ANY, 128);
	else
		iListenSocket = SocketInit(g_slave_port, INADDR_ANY, 128);

	if (iListenSocket <= 0)
	{
		fprintf(stderr, "Init listen socket[%s] errno[%d]", argv[1], errno);
		exit(-1);
	}

	while(1)
	{
		ErrLog(__FILE__,__LINE__,"INFO: Listen Socket=[%d]", iListenSocket);
		socklen_t	iAddrLen = sizeof(eCliAddr);
		memset(&eCliAddr, 0, sizeof(eCliAddr));

		iAcceptedSocket = accept(iListenSocket, (struct sockaddr *)&eCliAddr, &iAddrLen);
		ErrLog(__FILE__,__LINE__,"iAddrLen=[%d]", iAddrLen);
		if (iAcceptedSocket <= 0)
		{
			ErrLog(__FILE__,__LINE__,"ERROR: Accept err[%d] errno[%d]", iAcceptedSocket, errno);
			continue;
		}

		char addr_name[64];
		if (my_getpeername(iAcceptedSocket, addr_name) == 0)
		{
			ErrLog(__FILE__, __LINE__, "addr_name=[%s]", addr_name);
		}

		pthread_t tid;			
		thread_arg_t *arg = new thread_arg_t();	
		arg->fd = iAcceptedSocket;

		int ret = pthread_create(&(tid), NULL, handler_thread, arg);
		if (ret == 0)
		{
			ret = pthread_detach(tid);
			if (ret != 0)
			{
				ErrLog(__FILE__,__LINE__, "ERROR: pthread detach error[%d]", ret);
			}
		}
		else
		{
			ErrLog(__FILE__,__LINE__,"ERROR: pthread_create error[%d]", ret);
		}
	}

	return 0;
}
