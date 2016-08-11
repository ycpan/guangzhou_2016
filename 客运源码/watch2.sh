#!/bin/sh
sleep 60
while true
do
	sn=`ps | grep /root/coach | wc -l`
	if [  $sn -lt 3  ]
	then
		ping_ret=`ping -c 2 114.114.114.114 | grep "icmp_seq"`
		if [ "$ping_ret" != "" ]
		then 
			killall -9 coach 
			sleep 10
			/root/coach & # "&"must be exist,or this process always still here,can't enter next step  
		else
			killall -9 coach 
			sleep 10
		fi
	fi
	sleep 5
	sm=`ps | grep watch3.sh | grep -v grep |awk '{print $2}'`
	if [  "${sm}" = ""  ]
	then
		/root/watch3.sh &
	fi
	sleep 5 
done
