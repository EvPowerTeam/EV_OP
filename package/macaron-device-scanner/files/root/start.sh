COUNTER=0

Reboot()
{
  TIME=`date +"%T"`
  SUB_TIME=`echo $TIME | cut -b 1-5`
  if [ "$SUB_TIME" == "23:59" ];
  then
    sleep 50
    echo "$TIME: restart system"  >> /root/log/`date "+%Y-%m-%d"`.log
    reboot
  fi
  if [ $COUNTER -gt 2 ];
  then
    IS_HCI_WORK=`hciconfig hci0 name | head -1 | cut -c 1-3`
    if [ "$IS_HCI_WORK" != "hci" ];
    then
      echo "$TIME: hci device dead, restart system" >> /root/log/`date "+%Y-%m-%d"`.log
      reboot
    fi
  fi
} 

CheckProcess()
{
  PROCESS_NUM=`ps | grep "  macaron-device-scanner" | grep -v "grep" | wc -l`
  if [ $PROCESS_NUM -eq 1 ];
  then
    return 0
  else
    return 1
  fi
}

while [ 1 ] ; do
  Reboot
  CheckProcess
  CheckQQ_RET=$?
  if [ $CheckQQ_RET -eq 1 ];
  then
    killall -9 macaron-device-scanner
    echo "`date "+%H:%M:%S"`: restart macaron"  >> /root/log/`date "+%Y-%m-%d"`.log
    (macaron-device-scanner &)
  fi
  COUNTER=$(($COUNTER+1))
  sleep 60
done