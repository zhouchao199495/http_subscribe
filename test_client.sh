
appkey=AF099SEW2355346
appsecret=1234567890
timestamp=`date +%s`
nonce=`date +%s | awk '{printf("%d", $1 % 10000);}'`

#echo "[$appsecret] [$timestamp] [$nonce]"

sign=`./cal_sign $timestamp $nonce $appsecret`
echo $sign


curl -i -X POST 'http://127.0.0.1:8088/api/userlog/subscription' \
-H "x-appkey:${appkey}" \
-H "x-timestamp:${timestamp}" \
-H "x-nonce:${nonce}" \
-H "x-sign:${sign}" \
--data '{
    "dataClassify": 1,
    "deviceNumber": "10",
    "XDRFilter": {
        "accessType": "3GPP",
        "interfaceInfo": [
            {
                "RAT": "5G",
                "interface": "N1",
                "procedureType": [
                    "11",
                    "22",
                    "33"
                ]
            },
            {
                "RAT": "5G",
                "interface": "N12",
                "procedureType": [
                    "44"
                ]
            }
        ],
        "cityInfo": {
            "localCity": [
                "025",
                "0512"
            ],
            "ownerCity": [
                "021",
                "010"
            ]
        },
        "gNBIPAdd": [
            "192.168.20.1",
            "192.168.20.2"
        ],
        "AMFIPAdd": [
            "192.168.30.1",
            "192.168.40.1"
        ],
        "gNBID": [
            "12345",
            "67890"
        ],
        "cellID": [
            "1234567",
            "2345678"
        ],
        "IMSI": [
            "460011234567890",
            "460011234567891",
            "460011234567892",
            "460011234567893"
        ],
        "MSISDN": [
            "13912345670",
            "13912345671",
            "13912345672",
            "13912345673"
        ],
        "IMEISV": [
            "11000000000001",
            "11000000000002"
        ],
        "UETAC": "11000000"
    },
    "channel": {
        "protocol": "SDTP",
        "server": [
            {
                "ip": "133.10.65.166",
                "port": 21,
                "loginID": "asd",
                "password": "mm@sef123"
            },
            {
                "ip": "133.10.65.165",
                "port": 21,
                "loginID": "asd",
                "password": "mm@sef123"
            }
        ]
    }
}'
