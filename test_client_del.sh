
appkey=AF099SEW2355346
appsecret=1234567890
timestamp=`date +%s`
nonce=`date +%s | awk '{printf("%d", $1 % 10000);}'`

#echo "[$appsecret] [$timestamp] [$nonce]"

sign=`./cal_sign $timestamp $nonce $appsecret`
echo $sign


curl -i -X DELETE 'http://127.0.0.1:8088/api/userlog/subscription' \
-H "x-appkey:${appkey}" \
-H "x-timestamp:${timestamp}" \
-H "x-nonce:${nonce}" \
-H "x-sign:${sign}"
