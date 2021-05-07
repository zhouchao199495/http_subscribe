
homedir=$1
source ${homedir}/.bashrc

ProgDir=${homedir}/interface/yd45g_subscribe

ps -ef |grep "subscribe_server master" |grep -v grep
if [ $? -ne 0 ];then
	${ProgDir}/subscribe_45g_server master 8088 8089 ${ProgDir}/subscribe_server.conf
fi
