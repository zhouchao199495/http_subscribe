#include <cstdio>
#include <cstring>
#include <cstring>
#include "subscribe_config.h"
#include "cfilequery.h"
#include "ErrLog.h"
#include "mystrutil.h"

int subConfig::load(char *configPath)
{
	int iRet = 0;
	int nLine = 0;
	time_t lastModifyTime;

	CFileQuery	*pFileQuery = new CFileQuery();

	if(pFileQuery->isFileExist(configPath, lastModifyTime))
	{
		pFileQuery->exec(configPath);
		if (pFileQuery->isActive ())
		{
			while (pFileQuery->next ())//下一条
			{
				subInterface *interface = new subInterface();

				// group_id,cell_id
				interface->static_data_dir = pFileQuery->stringValue(1);
				interface->appkey = pFileQuery->stringValue(2);
				interface->appsecret = pFileQuery->stringValue(3);
				interface->slave_ip_list = pFileQuery->stringValue(4);
/*
				memset(ip_list, 0, sizeof(ip_list));
				strncpy(ip_list, pFileQuery->stringValue(4), sizeof(ip_list) - 1);

				if (ip_list[0] != 0)
				{
					char *saveptr1, *p1;

					// 先解析到数组中
					p1 = mystrtok_r(ip_list, ";", &saveptr1);
					if (p1 != NULL)
					{
						do
						{
							interface->vecSlave.push_back(p1);
						} while ((p1 = mystrtok_r(NULL, ":", &saveptr1)) != NULL);
					}
				}
*/
				
				vecInterface.push_back(interface);	

				nLine++;
			}
		}

		ErrLog(__FILE__, __LINE__, "INFO: F:%s file:%s line:%d", __FILE__, configPath, nLine);
	}
	else
	{
		ErrLog(__FILE__, __LINE__, "ERROR: file:%s NOT exist", configPath);
		iRet = -1;
		goto ERROR;
	}

ERROR:
	if (pFileQuery)
		delete pFileQuery;

	return (iRet);
}

const char * subConfig::get_appsecret(const char *appkey)
{
	const char *secret = NULL;

	if (appkey == NULL)
		return NULL;
	else
	{
		for (size_t i = 0; i < vecInterface.size(); i++)
		{
			ErrLog(__FILE__, __LINE__, "INFO: config[%u].appkey=[%s]", i, vecInterface[i]->appkey.c_str());
			if (strcmp(vecInterface[i]->appkey.c_str(), appkey) == 0)
			{
				secret = vecInterface[i]->appsecret.c_str();
				break;
			}
		}
	}

	return secret;
}

const char * subConfig::get_static_data_dir(const char *appkey)
{
	const char *dir = NULL;

	if (appkey == NULL)
		return NULL;
	else
	{
		for (size_t i = 0; i < vecInterface.size(); i++)
		{
			// ErrLog(__FILE__, __LINE__, "INFO: config[%u].appkey=[%s]", i, vecInterface[i]->appkey.c_str());
			if (strcmp(vecInterface[i]->appkey.c_str(), appkey) == 0)
			{
				dir = vecInterface[i]->static_data_dir.c_str();
				break;
			}
		}
	}

	return dir;
}

const char * subConfig::get_slave_ip_list(const char *appkey)
{
	const char *value = NULL;

	if (appkey == NULL)
		return NULL;
	else
	{
		for (size_t i = 0; i < vecInterface.size(); i++)
		{
			// ErrLog(__FILE__, __LINE__, "INFO: config[%u].appkey=[%s]", i, vecInterface[i]->appkey.c_str());
			if (strcmp(vecInterface[i]->appkey.c_str(), appkey) == 0)
			{
				value = vecInterface[i]->slave_ip_list.c_str();
				break;
			}
		}
	}

	return value;
}
