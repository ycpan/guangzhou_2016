#!/bin/bash

##############################################################################
# route_count
# input 
# fliename:pos_format_result
# format:TIME,OBU,VAL,ROUTE1,RSSI1.ROUTE2,RSSI2,ROUTE3,RSSI3,ROUTE4,RSSI4,POS
# output
# fliename1:dif_route_
# format:COUNT,POS,VALUE,
##############################################################################

route()
{
	dos2unix -n pos_format_result pos_format_result.routmp
	cat pos_format_result.routmp | awk -F "," '{print $12 "," $4 "," $5 }' > 1.routmp
	cat pos_format_result.routmp | awk -F "," '{print $12 "," $6 "," $7 }' > 2.routmp
	cat pos_format_result.routmp | awk -F "," '{print $12 "," $8 "," $9 }' > 3.routmp
	cat pos_format_result.routmp | awk -F "," '{print $12 "," $10 "," $11 }' > 4.routmp
	wait
	cat 2.routmp >> 1.routmp && cat 3.routmp >> 1.routmp && cat 4.routmp >> 1.routmp 
	#sort 1 | uniq -c | sed 's/\(.*\) \(.*\)/\1,\2/' > 5.routmp 
	sort 1.routmp -o 5.routmp && sleep 5 
	#&& uniq -c 5.routmp | sed 's/\(.*\) \(.*\)/\1,\2/' | sed 's/  .* //'  >  route_value_1.routmp && echo "have a nice day!"
}

route_comp()
{
	echo "0,0,0" >> 6.routmp
	data=0000
	num=000
	cat 6.routmp | while read line
	do
		prevdata=$data
		prevnum=$num
		echo $line | awk 'BEGIN{FS=","}{print "1," $1 "," $0}' >> com1.routmp
		data="$(echo $line | awk 'BEGIN{FS=","}{print $2 }')"
		num="$(echo $line | awk 'BEGIN{FS=","}{print $1}')"
			if [ $data -eq $prevdata ]
			then 
				lineprev1="$(sed -e '$!{h;d}' -e x com1.routmp)"
				echo $lineprev1 >> com2.routmp
			else
			if [ -f com2.routmp ]
			then 
				lineprev1="$(sed -e '$!{h;d}' -e x com1.routmp)"
				echo $lineprev1 >> com2.routmp
				for i in $(cat com2.routmp | awk -F "," '{print $2}');do let "sum = sum + $i " ;done
				cat com2.routmp | while read line1
				do
					echo $line1 | awk -v su=$sum -F "," '{printf("%.2f",$2/su)}{print "," su "," $3"," $4 ","$5 }' >> com3.routmp 
				done
				echo " " >> com3.routmp
				sum=0
				rm com2.routmp && echo -n ".."
			else
				sed -e '$!{h;d}' -e x com1.routmp >> com3.routmp
				echo " " >> com3.routmp
			fi

		fi
	done
	
	ls output/route > /dev/null && echo -n ".." || mkdir -p -v output/route 
	sed -i -n '2,$p' com3.routmp
	mv com3.routmp avg_route_result.csv && mv avg_route_result.csv output/route  
	sed '1 ipercent,sum,count,route,pos' output/route/avg_route_result.csv -i


}
route_count()
{
	cat 5.routmp | awk -F "," '{print $2 "," $1  }' | uniq -c | sed 's/\(.*\) \(.*\)/\1,\2/' | sed  "s/  .* //" | sort -t, -k2n,2 -k3n,3 -k1nr,1 -o 6.routmp

#	route_comp
	

}

prev_route_value()
{
	(uniq -c 5.routmp | sed 's/\(.*\) \(.*\)/\1,\2/' | sed 's/  .* //' | sort -t, -k 2n,2 -k 3n,3 -k 4,4 -k 1n,1  >  route_value_1.routmp) &
	wait
	#rm route_value_2.routmp
	cat route_value_1.routmp | while read line
	do
        #num="$(echo $line | awk -F "," '{print $4}')"

        #echo $num | awk '{print strtonum("0x"$0)}'
        #echo $num
        	echo $line | awk -F "," '{n=strtonum("0x"$4)}{printf("%d",n * $1)}{print ","$0}' >> route_value_2.routmp
	done

}
route_value()
{
	echo "0,0,0" >> route_value_2.routmp
	route_value_route_value_data=0000
	route_value_sum=000
	cat route_value_2.routmp | while read route_value_line
	do
		prevroute_value_data=$route_value_data
		#prevroute_value_num=$route_value_num
		echo $route_value_line | awk 'BEGIN{FS=","}{print $0}' >> val1.routmp
		route_value_data="$(echo $route_value_line | awk 'BEGIN{FS=","}{print $3 $4 }')"
		#route_value_num="$(echo $route_value_line | awk 'BEGIN{FS=","}{print $1}')"
		
			#echo "route_value_data is $route_value_data"
			#echo "prevroute_value_data is $prevroute_value_data"
			if [ $route_value_data -eq $prevroute_value_data ]
			then 
				route_value_lineprev1="$(sed -e '$!{h;d}' -e x val1.routmp)"
				echo $route_value_lineprev1 >> val2.routmp
			else
			if [ -f val2.routmp ]
			then 
				route_value_sum=0
                                route_value_count=0
				route_value_lineprev1="$(sed -e '$!{h;d}' -e x val1.routmp)"
				echo $route_value_lineprev1 >> val2.routmp
				route_value_max="$(cat val2.routmp | awk -F "," '{print $5}' | sort | sed '/^$/d' | tail -1 )"
				route_value_min="$(cat val2.routmp | awk -F "," '{print $5}' | sort | sed '/^$/d' | head -1 )"
				#count="$(cat val2.routmp | wc -l)"
				#echo "the count is $count"
				#echo $route_value_lineprev1
				#cat val2.routmp | awk -F "," '{print $5}' | tr "\n" "  "
				for j in $(cat val2.routmp | awk -F "," '{print $2}');do let "route_value_count = route_value_count + $j " ;done
				for i in $(cat val2.routmp | awk -F "," '{print $1}');do let "route_value_sum = route_value_sum + $i " ;done
				#route_value_avg="$( expr $route_value_sum \/ $count)"
				cat val2.routmp | while read route_value_line1
				do
					#echo "the count is $count" 
					echo $route_value_line1 | awk -v max=$route_value_max -v min=$route_value_min -v cou=$route_value_count -v su=$route_value_sum -F "," '{printf("%X", su / cou)}{print "," cou ","su "," $0 "," min "~" max}' >> val3.routmp 
				done
				echo " " >> val3.routmp
				#route_value_sum=0
				#route_value_count=0
				rm val2.routmp && echo -n ".."
			else
				sed -e '$!{h;d}' -e x val1.routmp | awk -F "," '{print $5 "," "1," $1","$0 "," $5 "~" $5 }' >> val3.routmp
				echo " " >> val3.routmp
			fi

		fi
	done
	
	echo -n "the output flie is:" && ls output/route || mkdir -p -v output/route 
	sed -i -n '2,$p' val3.routmp
	cp val3.routmp detail_avg_route_value.csv && sed '1 iavg_value,sum_count,all_sum_value,sum_value,count,pos,route,value,range' detail_avg_route_value.csv -i && mv detail_avg_route_value.csv output/route 
	cat val3.routmp | awk -F "," '{print $6 "," $7 "," $9 "," $1 }' | sort -u > avg_route_value.csv.routmp && sort -t, -k 1n,1 -k 2n,2 avg_route_value.csv.routmp -o output/route/avg_route_value.csv && sed '1 ipos,route,range,avg_value' output/route/avg_route_value.csv -i && rm val3.routmp
	


}
#deal_prev_route_value &&

#uniq -c 5.routmp | sed 's/\(.*\) \(.*\)/\1,\2/' | sed 's/  .* //'  >  route_value_1.routmp &&  
#(prev_route_value) &
#wait
#route_value


##(route) &
##wait
##(route_count) &
##wait
##(route_comp) &
##wait
##(uniq -c 5.routmp | sed 's/\(.*\) \(.*\)/\1,\2/' | sed 's/  .* //'  >  route_value_1.routmp) &
##wait
##(prev_route_value) &
##wait
##(route_value) &
##wait
#route && route_count && route_comp && prev_route_value && route_value
#rm *.routmp
ls *.routmp > /dev/null && rm *.routmp
route
route_count
route_comp
prev_route_value
route_value
rm *.routmp
