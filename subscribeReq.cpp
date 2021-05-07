#include "subscribeReq.h"
#include "mystrutil.h"
#include "jansson.h"

static void mkdir_force(char *dirname)
{
	char cmd[128];
	sprintf(cmd, "mkdir -p %s", dirname);
	system(cmd);
}

//static const char *json_plural(int count) {
//	return count == 1 ? "" : "s";
//}

int subscribeReq::parse_access_type(json_t *element)
{
	char buff[32];

	mystrncpy(buff, json_string_value(element), sizeof(buff));
	if (strcmp(buff, "3GPP") == 0)
	{
		access_type = 1;
	}
	else if (strcmp(buff, "Non-3GPP") == 0)
	{
		access_type = 2;
	}
	else
	{
		access_type = 0;	// 不参与过滤
	}

	return 0;
}

/*
 *
 *
 */

int subscribeReq::parse_city_info(json_t *element)
{
	int ret = 0;
	const char *key;
	json_t *value;
	size_t size;

	size = json_object_size(element);
	ErrLog(__FILE__, __LINE__, "JSON Object of %ld pair%s:\n", size, json_plural(size));

	json_object_foreach(element, key, value) {
		ErrLog(__FILE__, __LINE__, "key=[%s] value type=[%d]", key, value->type);

		if (strcmp(key, "localCity") == 0 && json_is_array(value))
		{
			ret = parse_string_array(key, value, vecLocalCity);
		}
		else if (strcmp(key, "ownerCity") == 0 && json_is_array(value))
		{
			ret = parse_string_array(key, value, vecOwnerCity);
		}
	}

	return (ret);
}

typedef struct interface_name_to_id
{
	int id;
	const char *name;
} interface_name_to_id_t;

static interface_name_to_id_t yd5g_interface[] = 
{
	{1,"5G Uu"},	{2,"5G Xn"},	{3,"DC-NR X2"},		{4,"5G E1"},
	{5,"5G F1"},	{6,"5G UE_MR"},	{7,"5G Cell_MR"},	{8,"N3"},
	{11,"S1-U"},	{39,"N1&N2"},	{39,"N1N2"},		{39,"N1"},
	{39,"N2"},		{40,"N4"},		{41,"N5"},			{42,"N7"},
	{43,"N8"},		{44,"N10"},		{45,"N11"},			{46,"N12"},
	{47,"N14"},		{48,"N15"},		{49,"N22"},			{50,"N26"},
	{51,"N16"},		{52,"N24"},		{53,"N20"},			{54,"N21"},
	{55,"N40"},		{56,"Nnrf"},	{57,"N9"},			
	{80,"4G Uu"},	{81,"4G X2"},	{82,"4G UE_MR"},	{83,"4G Cell_MR"},
	{84,"S1-MME"},
	{85,"S6a"},
	{86,"S11-C"},
	{87,"S10"},
	{88,"SGs"},
	{89,"S5/S8"},
	{90,"Gn-C"},
	{91,"Gm"},
	{92,"Mw"},
	{93,"Mg"},
	{94,"Mi"},
	{95,"Mj"},
	{96,"ISC"},
	{97,"Sv"},
	{98,"Cx"},
	{99,"Dx"},
	{100,"Sh"},
	{101,"Dh"},
	{102,"Zh"},
	{103,"Gx"},
	{104,"Rx"},
	{105,"I2"},
	{106,"Nc"},
	{107,"ATCF-SCCAS"},
	{108,"Ut "},
	{109,"Sx"},
	// GM几个接口的待确定
	{110,"Gm接口VoLTE/VoNR会话XDR（Gm Session XDR）"},
	{111,"Gm接口VoLTE/VoNR切片XDR（Gm Slice XDR）"},
	{112,"S1-U/N3接口VoLTE/VoNR会话XDR（S1-U Session XDR）"},
	{113,"S1-U/N3接口VoLTE/VoNR切片XDR（S1-U Slice XDR）"},
	{114,"Mb接口VoLTE/VoNR会话XDR（Mb Session XDR）"},
	{115,"Mb接口VoLTE/VoNR切片XDR（Mb Slice XDR）"},
	{116,"S1-U特定源码XDR"},
	// 以上待确定
	{117,"S11-U"},
	{131,"5GM-01"},
	{132,"5GM-02"},
	{133,"5GM-03"},
	{134,"5GM-04"},
	{135,"5GM-05"},
	{136,"DM-01"},
	{137,"Ub"},
	{138,"NNI-01"},
	{139,"GC"},
	{140,"CP-01"},
	{141,"CP-02"},
	{142,"Fli"},
	{143,"CM-01"},
	{144,"IW-01"},
	{145,"IW-02"},
	{146,"IW-03"},
	{147,"E"},
	{148,"ENUM/DNS"},
	{149,"RF"},
	{150,"IS-01"},
	{151,"Zh"},
	{152,"C"},
	{153,"Zn"},
	{154,"CB-01"},
	{155,"CB-02"},
	{156,"CB-03"},
	{157,"CB-04"},
	{158,"CB-05"},
	{159,"CB-06"},
	{160,"CB-07"},
	{161,"CB-08"},
	{162,"Ici"},
	{163,"Izi"},
	{164,"Ifi"},
	{ -1 , NULL },
};

/**
 * 解析单个接口信息
 *
            {
                "RAT": "5G",
                "interface": "N1",
                "procedureType": [
                    "11",
                    "22",
                    "33"
                ]
            },
 */
int subscribeReq::parse_interface_item(json_t *element)
{
	int ret = 0;
	const char *key;
	json_t *value;
	size_t size;

	size = json_object_size(element);
	ErrLog(__FILE__, __LINE__, "parse_interface_item: JSON Object of %ld pair%s:\n", size, json_plural(size));

	interface_info *interface = new interface_info();
	vecInterfaceInfo.push_back(interface);

	json_object_foreach(element, key, value) {
		ErrLog(__FILE__, __LINE__, "key=[%s] value type=[%d]", key, value->type);
		/*
		 * key=[RAT] value type=[2]
		 * key=[interface] value type=[2]
		 * key=[procedureType] value type=[1]
		 */
		if (strcmp(key, "RAT") == 0)
		{
			if (json_is_string(value))
			{
				char rat[16];
				mystrncpy(rat, json_string_value(value), sizeof(rat));
				int iRat = get_rat(rat);
				if(iRat>0)
					interface->rat = iRat;
				else
				{
					ErrLog(__FILE__, __LINE__, "INFO: RAT is not specified");
				}
			}
			else
			{
				ErrLog(__FILE__, __LINE__, "ERROR: RAT type is not string");
				return -1;
			}
		}
		else if (strcmp(key, "interface") == 0)
		{ // 暂时只支持5G接口
			if (json_is_string(value))
			{
				char interface_name[16];
				mystrncpy(interface_name, json_string_value(value), sizeof(interface_name));
				int i;

				for (i = 0; yd5g_interface[i].name != NULL; i++)
				{
					if (strcasecmp(interface_name, yd5g_interface[i].name) == 0)
					{
						interface->interface = yd5g_interface[i].id;
						break;
					}
				}
			}
			else
			{
				ErrLog(__FILE__, __LINE__, "ERROR: interface type is not string");
				return -1;
			}
		}
		else if (strcmp(key, "procedureType") == 0)
		{
			ret = parse_string_array(key, value, interface->vecProcType);
		}
		else
		{
			ErrLog(__FILE__, __LINE__, "unknown key=[%s]", key);	
		}
	}

	return (ret);
}

/**
 * 解析interfaceInfo信息
 */
int subscribeReq::parse_interface_info(json_t *element)
{
	size_t size = json_array_size(element);
	size_t index;
	json_t *value;

    ErrLog(__FILE__, __LINE__, "interface_info: JSON Array of %ld element%s:\n", size, json_plural(size));

    json_array_foreach(element, index, value)
    {
    	if (json_is_object(value))
    	{
    		parse_interface_item(value);	
    	}
    	else
    	{
			ErrLog(__FILE__, __LINE__, "ERROR; not object element interfaceInfo:%d", index);
			return -1;
    	}
    }

    return 0;
}

/**
 * 解析文本数组，将解析内容放入vector
 */
int subscribeReq::parse_string_array(const char *key, json_t *element, vector<string> &vecData)
{
	size_t size = json_array_size(element);
	size_t index;
	json_t *value;

    ErrLog(__FILE__, __LINE__, "DEBUG: %s JSON Array of %ld element%s:\n", key, size, json_plural(size));

    json_array_foreach(element, index, value)
    {
    	if (json_is_string(value))
    	{
    		string str_value = json_string_value(value);
			
			// 空字符串输出null
    		if (str_value == "")
				vecData.push_back("null");
			else
				vecData.push_back(json_string_value(value));
    	}
    	else
    	{
			ErrLog(__FILE__, __LINE__, "ERROR; not string element %s:%d", key, index);
			return -1;
    	}
    }

    return 0;
}

/**
 * 解析XDRFilter
 */

int subscribeReq::parse_xdr_filter(json_t *element)
{
	int ret = 0;
	const char *key;
	json_t *value;
	size_t size;

	int ip_group_num = 0;

	size = json_object_size(element);
	ErrLog(__FILE__, __LINE__, "JSON Object of %ld pair%s:\n", size, json_plural(size));

	json_object_foreach(element, key, value) {
		ErrLog(__FILE__, __LINE__, "key=[%s] value type=[%d]", key, value->type);

		/*
		   key=[accessType] value type=[2]
		   key=[interfaceInfo] value type=[1]
		   key=[cityInfo] value type=[0]
		   key=[gNBIPAdd] value type=[1]
		   key=[AMFIPAdd] value type=[1]
		   key=[gNBID] value type=[1]
		   key=[cellID] value type=[1]
		   key=[IMSI] value type=[1]
		   key=[MSISDN] value type=[1]
		   key=[IMEISV] value type=[1]
		   key=[UETAC] value type=[2]
		   */

		if (strcmp(key, "accessType") == 0 && json_is_string(value))
		{
			ret = parse_access_type(value);
		}
		else if (strcmp(key, "interfaceInfo") == 0 && json_is_array(value))
		{
			ret = parse_interface_info(value);
		}
		else if (strcmp(key, "cityInfo") == 0)
		{
			ret = parse_city_info(value);
		}
		// ================================
		// 45G融合规范调整，由gNBIPAdd、AMFIPAdd变为NE1IPAdd、NE2IPAdd
		else if (strcasestr(key, "IPAdd") != NULL && json_is_array(value))
		{ // gNBIPAdd、AMFIPAdd 等
			if (ip_group_num == 0)
			{
				ret = parse_string_array(key, value, vecDeviceIp_1);
				++ip_group_num;
			}
			else if (ip_group_num == 1)
			{
				ret = parse_string_array(key, value, vecDeviceIp_2);
				++ip_group_num;
			}
			else
			{
				ErrLog(__FILE__, __LINE__, "ERROR: Only support to ip address group");
				ret = -1;
			}
		}
		else if (strcmp(key, "NodeBID") == 0 && json_is_array(value))
		{
			ret = parse_string_array(key, value, vecGnbId);
		}
		else if (strcmp(key, "cellID") == 0 && json_is_array(value))
		{
			ret = parse_string_array(key, value, vecCellId);
		}
		else if (strcmp(key, "IMSI") == 0 && json_is_array(value))
		{
			ret = parse_string_array(key, value, vecImsi);
		}
		else if (strcmp(key, "MSISDN") == 0 && json_is_array(value))
		{
			ret = parse_string_array(key, value, vecMsisdn);
		}
		else if (strcmp(key, "IMEISV") == 0 && json_is_array(value))
		{
			ret = parse_string_array(key, value, vecImei);
		}
		else if (strcmp(key, "UETAC") == 0)
		{ 
			if (json_is_array(value))
			{
				ret = parse_string_array(key, value, vecUeTac);
			}
			else if (json_is_string(value))
			{
				vecUeTac.push_back(json_string_value(value));
			}
		}
	}

	return ret;
}

/**
  解析请求
  \param sIn 请求字符串
  \retval 0 解析成功
  \retval -1 未知请求
  \retval -2 解析请求命令中的输入参数错误
  \retval -3 解析请求命令中的属性不正确
  */
int subscribeReq::parse(const char *sIn)
{
	int ret = 0;
	json_error_t error;
	json_t *root = json_loads(sIn, JSON_DECODE_ANY, &error);

	if (root)
	{ // OBJECT
		ErrLog(__FILE__,__LINE__, "type=[%d]", root->type);	

		const char *key;
		json_t *value;
		size_t size;

		size = json_object_size(root);
		ErrLog(__FILE__, __LINE__, "JSON Object of %ld pair%s:\n", size, json_plural(size));

		json_object_foreach(root, key, value) {
			ErrLog(__FILE__, __LINE__, "key=[%s] value type=[%d]", key, value->type);

			/*
			 * key=[dataClassify] value type=[3]	// JSON_INTEGER
			 * key=[deviceNumber] value type=[2]	// JSON_STRING
			 * key=[XDRFilter] value type=[0]	// 
			 * key=[channel] value type=[0]
			 */

			if (strcmp(key, "dataClassify") == 0)
			{
				if (json_is_integer(value))
				{
					json_int_t data_classify = json_integer_value(value);
					if (data_classify != 1)
					{
						ErrLog(__FILE__, __LINE__, "Unsupport data_classify=[%ld]", data_classify);	
						return -1;
					}
				}
				else
				{
					ErrLog(__FILE__, __LINE__, "dataClassify type is not INTEGER");
					return -1;
				}
			}
			else if (strcmp(key, "deviceNumber") == 0)
			{
				// do nothing
			}
			else if (strcmp(key, "XDRFilter") == 0)
			{
				ret = parse_xdr_filter(value);
			}
			else if (strcmp(key, "channel") == 0)
			{
				// do nothing	
			}
			else
			{
				ErrLog(__FILE__, __LINE__, "ERROR: unknown key=[%s]", key);	
				return -1;
			}
		}
	}
	else
	{
		ErrLog(__FILE__, __LINE__, "ERROR: parse [%s] error as line %d: %s", sIn, error.line, error.text );
		ret = -1;
	}

	return(ret);
}

#define OUTPUT_GROUP_ID(size, group_id) \
{ \
	if (size > 0) \
		pOut += sprintf(pOut, ",%d", group_id); \
	else \
		pOut += sprintf(pOut, ",%d", 0); \
}

#define OUTPUT_GROUP_ID_AND_VALUE(FP, VECTOR, group_id) \
{ \
	if (VECTOR.size() > 0) \
	{ \
		for (size_t k = 0; k < VECTOR.size(); k++) \
			fprintf(FP, "%d,%s\n", group_id, VECTOR[k].c_str()); \
	} \
}

int subscribeReq::dump(const char *static_dir)
{
	int ret;
	size_t i, j;
	
	// 先写临时目录
	char tempdir[128];
	sprintf(tempdir, "%s/temp", static_dir);
	mkdir_force(tempdir);

	// 设置文件名
	set_temp_file_name(static_dir);

	// 打开文件
	if (open_temp_file() != 0)
	{
		ErrLog(__FILE__, __LINE__, "ERROR: open_temp_file error!");
		return -1;
	}
	
	// 写表头
	write_temp_file_header();

	char buff[128];
	char *pOut = buff;
	interface_info *interface;
	int proc_type_id = 1;

	// 写st_compound_filter.dat
	//access_type,rat,interface,procedure_type_group,local_city_group,owner_city_group,device_ip_group_1,device_ip_group_2,cell_id_group,gnb_id_group,imsi_group,msisdn_group,imei_group,imei_tac_group\n");
	if (vecInterfaceInfo.size() > 0)
	{
		for (i = 0; i < vecInterfaceInfo.size(); i++)
		{
			interface = vecInterfaceInfo[i];

			pOut = buff;

			if (interface->vecProcType.size() > 0)
			{
				pOut += sprintf(pOut, "%d,%d,%d,%d", access_type, interface->rat, interface->interface, proc_type_id);
	
				// 写过程类型文件
				for (j = 0; j < interface->vecProcType.size(); j++)
				{
					fprintf(fpProcType, "%d,%s\n", proc_type_id, interface->vecProcType[j].c_str());
				}

				proc_type_id++;
			}
			else
			{
				pOut += sprintf(pOut, "%d,%d,%d,%d", access_type, interface->rat, interface->interface, 0);
			}
	
			// 拼其他字段
			OUTPUT_GROUP_ID(vecLocalCity.size(), 1)
			OUTPUT_GROUP_ID(vecOwnerCity.size(), 1)
			OUTPUT_GROUP_ID(vecDeviceIp_1.size(), 1)
			OUTPUT_GROUP_ID(vecDeviceIp_2.size(), 2)
			OUTPUT_GROUP_ID(vecCellId.size(), 1)
			OUTPUT_GROUP_ID(vecGnbId.size(), 1)
			OUTPUT_GROUP_ID(vecImsi.size(), 1)
			OUTPUT_GROUP_ID(vecMsisdn.size(), 1)
			OUTPUT_GROUP_ID(vecImei.size(), 1)
			OUTPUT_GROUP_ID(vecUeTac.size(), 1)

			ErrLog(__FILE__, __LINE__, "INFO: buff=[%s]", buff); 
			fprintf(fpFilter, "%s\n", buff);
		}
	}
	else
	{
		pOut = buff;
		pOut += sprintf(pOut, "%d,0,0,0", access_type);

		// 拼其他字段
		OUTPUT_GROUP_ID(vecLocalCity.size(), 1)
		OUTPUT_GROUP_ID(vecOwnerCity.size(), 1)
		OUTPUT_GROUP_ID(vecDeviceIp_1.size(), 1)
		OUTPUT_GROUP_ID(vecDeviceIp_2.size(), 2)
		OUTPUT_GROUP_ID(vecCellId.size(), 1)
		OUTPUT_GROUP_ID(vecGnbId.size(), 1)
		OUTPUT_GROUP_ID(vecImsi.size(), 1)
		OUTPUT_GROUP_ID(vecMsisdn.size(), 1)
		OUTPUT_GROUP_ID(vecImei.size(), 1)
		OUTPUT_GROUP_ID(vecUeTac.size(), 1)

		fprintf(fpFilter, "%s\n", buff);
	}

	// 输出其他文件
	OUTPUT_GROUP_ID_AND_VALUE(fpLocalCity, vecLocalCity, 1)
	OUTPUT_GROUP_ID_AND_VALUE(fpOwnerCity, vecOwnerCity, 1)
	OUTPUT_GROUP_ID_AND_VALUE(fpDeviceIp, vecDeviceIp_1, 1)
	OUTPUT_GROUP_ID_AND_VALUE(fpDeviceIp, vecDeviceIp_2, 2)
	OUTPUT_GROUP_ID_AND_VALUE(fpCellId, vecCellId, 1)
	OUTPUT_GROUP_ID_AND_VALUE(fpGnbId, vecGnbId, 1)
	OUTPUT_GROUP_ID_AND_VALUE(fpImsi, vecImsi, 1)
	OUTPUT_GROUP_ID_AND_VALUE(fpMsisdn, vecMsisdn, 1)
	OUTPUT_GROUP_ID_AND_VALUE(fpImei, vecImei, 1)
	OUTPUT_GROUP_ID_AND_VALUE(fpUeTac, vecUeTac, 1)

	close_temp_file();
	
	ret = rename_temp_file();
	if (ret != 0)
	{
		ErrLog(__FILE__, __LINE__, "ERROR: rename_temp_file error!");
		return -1;
	}

	return 0;
}

/**
 * 清空订阅信息
 */
int subscribeReq::clear_static_file(const char *static_dir)
{
	int ret;

	ErrLog(__FILE__, __LINE__, "INFO: delete subscribe");
	
	// 先写临时目录
	char tempdir[128];
	sprintf(tempdir, "%s/temp", static_dir);
	mkdir_force(tempdir);

	// 设置文件名
	set_temp_file_name(static_dir);

	// 打开文件
	if (open_temp_file() != 0)
	{
		ErrLog(__FILE__, __LINE__, "ERROR: open_temp_file error!");
		return -1;
	}
	
	// 写表头
	write_temp_file_header();

	close_temp_file();
	
	ret = rename_temp_file();
	if (ret != 0)
	{
		ErrLog(__FILE__, __LINE__, "ERROR: rename_temp_file error!");
		return -1;
	}

	return 0;
}
