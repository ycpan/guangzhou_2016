#!/bin/sh

while true
do


sc=`ps | grep crond | grep -v grep |awk '{print $2}'`
if [  "${sc}" = ""  ]
	then
       crond

fi
	sleep 10


sn=`ps | grep pppd | grep -v grep |awk '{print $2}'`
if [  "${sn}" = ""  ]
	then


	killall -9 pppd 
	sleep 3
	pppd call wcdma &
	sleep 10
	route del default 
	route add default gw 10.64.64.64


fi
	sleep 10

#watch2 and watch3 watch each other
sk=`ps | grep watch2.sh | grep -v grep |awk '{print $2}'`     
        if [  "${sk}" = ""  ]
         then
   	      
                       
        	 /root/watch2.sh &                     
                                
                                       
          fi                                       
           sleep 5                                               
                           
        done
