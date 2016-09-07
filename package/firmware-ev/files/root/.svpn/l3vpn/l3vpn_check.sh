#!/bin/sh

HOME_PATH=/root
is_mips_openwrt(){
    uname -ar|grep mips
    if [ $? -eq 0 ];then
        return 1
    fi
    return 0
}
check_is_running(){
    SCRIPT_NAME=`basename "$0"`
    is_mips_openwrt
    if [ $? -eq 0 ];then
        pids=$(ps -w|grep l3vpn_check.sh|grep -v grep|grep -v vim|grep -v vi|grep -v tail|awk '{print $1}')
    else
        pids=$(pgrep $SCRIPT_NAME)
    fi

    for pid in $pids
    do
        if [ $pid -ne $$ ];then
            echo "is already running"
            return $pid
        fi
    done
    return 0
}

check_l3vpn(){
    pidof l3vpnd
    if [ $? -ne 0 ]
    then
        cd ${HOME_PATH}/.svpn/l3vpn && ./l3vpnd >/dev/null 2>&1 &
    fi
}

do_start(){
    CHECK_TIME=5
    check_is_running
    if [ $? -ne 0 ];then
        echo "l3vpn_check.sh is already running"
        exit 0
    fi
    echo "start success"
    while true
    do
        check_l3vpn
        sleep $CHECK_TIME
    done
}

do_start_mips(){
    CHECK_TIME=5
    IS_RUN=0
    ls /var/run/l3vpn_check.pid > /dev/null 2>&1 
    if [ $? -eq 0 ];then
        PID=$(cat /var/run/l3vpn_check.pid)
        ls /proc/$PID/ > /dev/null 2>&1 
        if [ $? -eq 0 ];then
            echo "mips l3vpnd_check.sh is already running"
            exit 0
        fi
    fi
    echo "start l3vpn_check.sh"
    echo $$ > /var/run/l3vpn_check.pid
    while true
    do
        check_l3vpn
        sleep $CHECK_TIME
    done
}

uname -ar|grep mips
if [ $? -eq 0 ];then
    do_start_mips
else
    do_start
fi

