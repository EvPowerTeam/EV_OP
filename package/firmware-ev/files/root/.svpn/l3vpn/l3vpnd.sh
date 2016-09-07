#!/bin/sh /etc/rc.common

#start sanfor l3vpnd service monitor script 
HOME_PATH=/root

start()
{
    ${HOME_PATH}/.svpn/l3vpn/l3vpn_check.sh > /dev/null 2>&1 &
    sleep 2
    (crontab -l ; echo "* * * * * /root/.svpn/l3vpn/l3vpn_check.sh > /dev/null 2>&1 &") | crontab -
    if [ $? -eq 0 ]
    then
        echo "success"
    else
        echo "failed"
    fi
}

stop(){
    uname -ar|grep mips
    if [ $? -eq 0 ];then
        PIDS=$(ps -w|grep l3vpn_check.sh|grep -v grep|grep -v vim|grep -v vi|grep -v tail|awk '{print $1}')
    else
        PIDS=$(pgrep $SCRIPT_NAME)
    fi
    echo $PIDS
    for pid in $PIDS
    do
        echo "kill $pid"
        kill -9 $pid > /dev/null 2>&1 &
    done
}
