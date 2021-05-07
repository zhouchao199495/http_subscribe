#ifndef _SUBSCRIBE_CONFIG_H_
#define _SUBSCRIBE_CONFIG_H_

#include <stdint.h>
#include <string>
#include <vector>

using namespace std;

/**
 * 接口信息
 */
class subInterface
{
public:
	subInterface() { }

	~subInterface() { }
	
public:
	string static_data_dir;	//!< 静态数据目录
	string appkey;	//!< 类似用户名
	string appsecret;	//!< 密码
	string slave_ip_list;	//!< 从ip列表,分号分隔
};

/**
 * 配置
 */
class subConfig
{
public:
	subConfig()
	{
		vecInterface.clear();
	}

	~subConfig()
	{
		for (unsigned i = 0; i < vecInterface.size(); i++)
		{
			delete vecInterface[i];
		}

		vecInterface.clear();
	}

	int load(char *configPath);

	const char *get_appsecret(const char *appkey);
	const char *get_static_data_dir(const char *appkey);
	const char *get_slave_ip_list(const char *appkey);

public:
	vector<subInterface *> vecInterface;	//!< 接口列表
};

#endif // #ifndef _SUBSCRIBE_CONFIG_H_
