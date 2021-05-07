#ifndef _LTDPI_RETRANS_REQUEST_H_
#define _LTDPI_RETRANS_REQUEST_H_

/*
	用于实现移动5G合成服务器规范中的 数据订阅接口
1、数据订阅
2、删除订阅


x-appkey	String	是	调用接口的身份凭证，由提供接口的服务端给客户端分配。
x-timestamp	Integer	是	时间戳的明文。
由客户端产生,单位为秒（1970年1月1日0时0分0秒起至当前的偏移总秒数）
x-nonce	String	是	由客户端产生的随机数
x-sign	String	是	用于鉴别源用户。其值通过计算32位大写MD5得出。当对端用相同方式计算之后与接收的值比较，如果计算出来的值相同，则通过校验，否则出错。 
					sign= MD5(“appsecret=”+x-appsecret+“~time=”+x-timestamp+“~nonce=”+x-nonce+“~path=”+path) 
					其中x-appsecret为与x-appkey对应的秘钥，由提供接口的服务端给客户端事先分配； 
					x-timestamp为本消息带的x-timestamp字段数值；
					x-nonce为本消息带的x-nonce字段数值； 
                    path为URI路径，例如： /api/userlog/subscription。
                    计算结果取32位大写。
*/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdint.h>
#include <string>
#include <map>
#include <vector>
#include "errno.h"
#include "jansson.h"
#include "ErrLog.h"
#include "mystrutil.h"

using namespace std;

static inline const char *json_plural(int count) {
	return count == 1 ? "" : "s";
}

class interface_info
{
public:
	interface_info()
	{
		rat = 0;
		interface = 0;
		vecProcType.clear();
	}

public:
	uint8_t rat;
	uint8_t interface;
	vector<string> vecProcType;
};

#define OPEN_TEMP_STATIC_FILE(fp, filename) \
{ \
	fp = fopen(filename.c_str(), "w"); \
	if (fp == NULL) \
	{ \
		ErrLog(__FILE__, __LINE__, "open [%s] error", filename.c_str()); \
		close_temp_file(); \
		return -1; \
	} \
}

/**
	请求包解析类
*/
class subscribeReq
{
public:
/*
	string id;
	string type;			// query_request
	vector<string> vecFile;
	vector<bool> vecFileExists;
*/
	uint8_t access_type;

	vector<interface_info *> vecInterfaceInfo;
	
	// cityInfo
	vector<string> vecLocalCity;
	vector<string> vecOwnerCity;

	vector<string> vecDeviceIp_1;
	vector<string> vecDeviceIp_2;
	vector<string> vecGnbId;
	vector<string> vecCellId;
	vector<string> vecImsi;
	vector<string> vecMsisdn;
	vector<string> vecImei;
	vector<string> vecUeTac;

public:
	subscribeReq()
	{
		initDict();
		clear();
	}

#if 0
	subscribeReq & operator=(subscribeReq &other)
	{
		return *this;
	}
#endif

	void clear()
	{
		access_type = 0;

		vecInterfaceInfo.clear();
	
		vecLocalCity.clear();
		vecOwnerCity.clear();

		vecDeviceIp_1.clear();
		vecDeviceIp_2.clear();
		vecGnbId.clear();
		vecCellId.clear();
		vecImsi.clear();
		vecMsisdn.clear();
		vecImei.clear();
		vecUeTac.clear();
	}

	int get_rat(string rat)
	{
		map<string,int>::iterator it = mapRatDict.find(rat);
		if(it!=mapRatDict.end()) 
			return it->second;
		else 
			return -1;
	}

	void initDict()
	{
		// RAT类型字典
		mapRatDict["UTRAN"] = 1;
		mapRatDict["GERAN"] = 2;
		mapRatDict["WLAN"] = 3;
		mapRatDict["GAN"] = 4;
		mapRatDict["HSPA Evolution"] = 5;
		mapRatDict["EUTRAN"] = 6;
		mapRatDict["NB-IOT"] = 7;
		mapRatDict["5G"] = 10;


	}

	/**
		解析请求
		\param sIn 请求字符串
		\retval 0 解析成功
		\retval -1 未知请求
		\retval -2 解析请求命令中的输入参数错误
		\retval -3 解析请求命令中的属性不正确
	*/
	int parse(const char *sIn);
	
	/*
	 * 写静态数据文件,先写入临时目录，然后将其挪到正式目录
	 */
	int dump(const char *static_dir);
	
	/**
	 * 请空订阅信息
	 */
	int clear_static_file(const char *static_dir);

	void set_temp_file_name(const char *static_dir)
	{
		string str_static_dir = static_dir; 

		compound_filter_file = str_static_dir + "st_compound_filter.dat.tmp";
		owner_city_group_file = str_static_dir + "st_filter_owner_city_group.dat.tmp";
		local_city_group_file = str_static_dir + "st_filter_local_city_group.dat.tmp";
		procedure_type_group_file = str_static_dir + "st_filter_procedure_type_group.dat.tmp";
		device_ip_group_file = str_static_dir + "st_filter_device_ip_group.dat.tmp";
		cell_id_group_file = str_static_dir + "st_filter_cell_id_group.dat.tmp";
		gnb_id_group_file = str_static_dir + "st_filter_gnb_id_group.dat.tmp";
		msisdn_group_file = str_static_dir + "st_filter_msisdn_group.dat.tmp";
		imsi_group_file = str_static_dir + "st_filter_imsi_group.dat.tmp";
		imei_group_file = str_static_dir + "st_filter_imei_group.dat.tmp";
		imei_tac_group_file = str_static_dir + "st_filter_imei_tac_group.dat.tmp";
	}

	int write_temp_file_header()
	{
		fprintf(fpFilter, "access_type,rat,interface,procedure_type_group,local_city_group,owner_city_group,device_ip_group_1,device_ip_group_2,cell_id_group,gnb_id_group,imsi_group,msisdn_group,imei_group,imei_tac_group\n");
		fprintf(fpOwnerCity, "owner_city_group_id,owner_city\n");
		fprintf(fpLocalCity, "local_city_group_id,local_city\n");
		fprintf(fpProcType, "proc_type_group_id,proc_type\n");
		fprintf(fpDeviceIp, "device_ip_group_id,device_ip\n");
		fprintf(fpCellId, "cell_group_id,cell_id\n");
		fprintf(fpGnbId, "gnb_group_id,gnb_id\n");
		fprintf(fpMsisdn, "msisdn_group_id,msisdn\n");
		fprintf(fpImsi, "imsi_group_id,imsi\n");
		fprintf(fpImei, "imei_group_id,imei\n");
		fprintf(fpUeTac, "tac_group_id,imei_tac\n");

		return 0;
	}

	int open_temp_file(void)
	{
		fpFilter = NULL;
		fpOwnerCity = NULL;
		fpLocalCity = NULL;
		fpProcType = NULL;
		fpDeviceIp = NULL;
		fpCellId = NULL;
		fpGnbId = NULL;
		fpMsisdn = NULL;
		fpImsi = NULL;
		fpImei = NULL;
		fpUeTac = NULL;

		OPEN_TEMP_STATIC_FILE(fpFilter, compound_filter_file);
		OPEN_TEMP_STATIC_FILE(fpOwnerCity, owner_city_group_file);
		OPEN_TEMP_STATIC_FILE(fpLocalCity, local_city_group_file);
		OPEN_TEMP_STATIC_FILE(fpProcType, procedure_type_group_file);
		OPEN_TEMP_STATIC_FILE(fpDeviceIp, device_ip_group_file);
		OPEN_TEMP_STATIC_FILE(fpCellId, cell_id_group_file);
		OPEN_TEMP_STATIC_FILE(fpGnbId, gnb_id_group_file);
		OPEN_TEMP_STATIC_FILE(fpMsisdn, msisdn_group_file);
		OPEN_TEMP_STATIC_FILE(fpImsi, imsi_group_file);
		OPEN_TEMP_STATIC_FILE(fpImei, imei_group_file);
		OPEN_TEMP_STATIC_FILE(fpUeTac, imei_tac_group_file);

		return 0;
	}

	void close_temp_file(void)
	{
		if (fpFilter)	fclose(fpFilter);
		if (fpOwnerCity) fclose(fpOwnerCity);
		if (fpLocalCity) fclose(fpLocalCity);
		if (fpProcType) fclose(fpProcType);
		if (fpDeviceIp) fclose(fpDeviceIp);
		if (fpCellId) fclose(fpCellId);
		if (fpGnbId) fclose(fpGnbId);
		if (fpMsisdn) fclose(fpMsisdn);
		if (fpImsi) fclose(fpImsi);
		if (fpImei) fclose(fpImei);
		if (fpUeTac) fclose(fpUeTac);
	}

	int rename_single_temp_file(string &old_name)
	{
		char new_name[256];

		mystrncpy(new_name, old_name.c_str(), sizeof(new_name));
		char *p1 = strstr(new_name, ".tmp");
		if (p1 != NULL)
		{
			*p1 = 0;
			int ret = rename(old_name.c_str(), new_name);
			if (ret != 0)
			{
				ErrLog(__FILE__, __LINE__, "rename error [%s]->[%s] error[%s]", old_name.c_str(), new_name, strerror(errno));
				return -1;
			}
		}
		else
		{
			ErrLog(__FILE__, __LINE__, "source filename error [%s]", old_name.c_str());
			return -1;
		}

		return 0;
	}

	int rename_temp_file(void)
	{
		return (rename_single_temp_file(compound_filter_file) ||
			rename_single_temp_file(owner_city_group_file) ||
			rename_single_temp_file(local_city_group_file) ||
			rename_single_temp_file(procedure_type_group_file) ||
			rename_single_temp_file(device_ip_group_file) ||
			rename_single_temp_file(cell_id_group_file) ||
			rename_single_temp_file(gnb_id_group_file) ||
			rename_single_temp_file(msisdn_group_file) ||
			rename_single_temp_file(imsi_group_file) ||
			rename_single_temp_file(imei_group_file) ||
			rename_single_temp_file(imei_tac_group_file));
	}

private:
	map<string, int> mapRatDict;
private:
	int parse_xdr_filter(json_t *element);	// 解析xdrFilter
	int parse_access_type(json_t *element);
	int parse_city_info(json_t *element);
	int parse_string_array(const char *key, json_t *element, vector<string> &vecData);	// 解析字符串数组
	int parse_interface_item(json_t *element);
	int parse_interface_info(json_t *element);

	
	// 静态数据文件名
	string compound_filter_file;
	string owner_city_group_file;
	string local_city_group_file;
	string procedure_type_group_file;
	string device_ip_group_file;
	string cell_id_group_file;
	string gnb_id_group_file;
	string msisdn_group_file;
	string imsi_group_file;
	string imei_group_file;
	string imei_tac_group_file;
	
	// 文件句柄
	FILE *fpFilter;
	FILE *fpOwnerCity;
	FILE *fpLocalCity;
	FILE *fpProcType;
	FILE *fpDeviceIp;
	FILE *fpCellId;
	FILE *fpGnbId;
	FILE *fpMsisdn;
	FILE *fpImsi;
	FILE *fpImei;
	FILE *fpUeTac;
};

#endif
