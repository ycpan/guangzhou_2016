#!/bin/bash
prev_diff_value()
{
	#route_11=0;route_12=0;route_13=0
	#route_14=0;route_15=0;route_16=0
	#route_17=0;route_18=0;route_19=0
	dos2unix -n pos_format_result prev_diff_1.difmp
	cat prev_diff_1.difmp | while read line
	do
		route_11=0;route_12=0;route_13=0
	        route_14=0;route_15=0;route_16=0
        	route_17=0;route_18=0;route_19=0

		x1="$(echo $line | awk -F "," '{print $4 }')"
		x2="$(echo $line | awk -F "," '{print $6 }')"
		x3="$(echo $line | awk -F "," '{print $8 }')"
		x4="$(echo $line | awk -F "," '{print $10 }')"
		x1_value="$(echo $line | awk -F "," '{print $5 }')"
		x2_value="$(echo $line | awk -F "," '{print $7 }')"
		x3_value="$(echo $line | awk -F "," '{print $9 }')"
		x4_value="$(echo $line | awk -F "," '{print $11 }')"
		#echo "$x1 $x2 $x3 $x4"
		#echo "$x1_value $x2_value $x3_value $x4_value"
		echo "route_$x1=`echo $x1_value`" >> prev_diff_3.difmp
		echo "route_$x2=`echo $x2_value`" >> prev_diff_3.difmp
		echo "route_$x3=`echo $x3_value`" >> prev_diff_3.difmp
		echo "route_$x4=`echo $x4_value`" >> prev_diff_3.difmp
		#exec route_`echo $x1`=`echo $x1_value`
		source prev_diff_3.difmp
		
		#echo "$route_11 $route_12 $route_13 $route_14 $route_15 $route_16 $route_17 $route_18 $route_19"
		echo $line | awk -F "," -v rou_11=$route_11 -v rou_12=$route_12 -v rou_13=$route_13 -v rou_14=$route_14 -v rou_15=$route_15 -v rou_16=$route_16 -v  rou_17=$route_17  -v rou_18=$route_18 -v rou_19=$route_19 '{print $12 "," $2 "," $3 ",11," rou_11 ",15," rou_15 ",12," rou_12 ",16,"  rou_16 ",13," rou_13 ",17," rou_17 ",14," rou_14 ",18," rou_18 ",19," rou_19 }' >> fun1_prev_diff_1.difmp
		rm -rf prev_diff_3.difmp && echo -n "." 
		done
		cp fun1_prev_diff_1.difmp prev_diff_result.csv && rm fun1_prev_diff_1.difmp
}

prev_diff()
{
	rm prev_diff_4.difmp
	cat prev_diff_result.csv | while read line
	do
        #num="$(echo $line | awk -F "," '{print $5}')"
	
        #echo $num | awk '{print strtonum("0x"$0)}'
        #echo $num
        	echo $line | awk -F "," '{n1=strtonum("0x"$5)-strtonum("0x"$7)}{n2=strtonum("0x"$9)-strtonum("0x"$11)}{n3=strtonum("0x"$13)-strtonum("0x"$15)}{n4=strtonum("0x"$17)-strtonum("0x"$19)}{n4=strtonum("0x"$21)} {if (n1 >= 0) {printf("%X,",n1)} if ( n1 < 0 ) {printf("-%X,",0-n1)}  } {if (n2 >= 0) {printf("%X,",n2)} if ( n2 < 0 ) {printf("-%X,",0-n2)}  }{if (n3 >= 0) {printf("%X,",n3)} if ( n3 < 0 ) {printf("-%X,",0-n3)}  }{if (n4 >= 0) {printf("%X,%X,",n4,n5)} if ( n4 < 0 ) {printf("-%X,%X,",0-n4,n5)}  }{print $1 "," $2 "," $3  } ' >> prev_diff_4.difmp
	echo -n "."
	done

}

prev_diff_rough()
{
	rm prev_diff_5.difmp
	cat prev_diff_result.csv | while read line
	do
        #num="$(echo $line | awk -F "," '{print $5}')"
	
        #echo $num | awk '{print strtonum("0x"$0)}'
        #echo $num
        	echo $line | awk -F "," '{n1=strtonum("0x"$5)-strtonum("0x"$7)}{n2=strtonum("0x"$9)-strtonum("0x"$11)}{n3=strtonum("0x"$13)-strtonum("0x"$15)}{n4=strtonum("0x"$17)-strtonum("0x"$19)}{n5=strtonum("0x"$21)} {if (n1 > 0) {a=3} if ( n1 == 0 ) {a=2}  if ( n1 < 0 ) {a =1} } {if (n2 > 0) {b=3} if ( n2 == 0 ) {b=2} if ( n2 < 0 ) {b=1} }{if ( n3 > 0 )  {c=3} if ( n3 == 0 ) {c=2} if ( n3 < 0 ) {c=1}  }{if ( n4 > 0 ) {d=3 } if ( n4 == 0 ) {d=2} if ( n4 < 0 ) {d=1}  }{if ( n5 > 0 ) {e=3 } if ( n5 == 0 ) {e=2} if ( n5 < 0 ) {e=1}  }{print $1 "," $2 "," $3 "," a ","b "," c "," d "," e   } ' >> prev_diff_5.difmp
	echo -n "."
	done
	#sort -t, -k 1n,1 -k 3n,3 -k 4n,4 -k 5n,5 -k 6n,6 -k 7n,7 -k 8n,8 prev_diff_5.difmp  | uniq -c | sed 's/\(.*\) \(.*\)/\1,\2/' > prev_diff_6.difmp
	cat prev_diff_5.difmp | awk -F "," '{print $1 "," $4 "," $5 "," $6 "," $7 "," $8 }' > noobu_diff_rough.difmp
#| sort -t, -k 3n,3 -k 4n,4 -k 5n,5 -k 6n,6 -k 7n,7 -k 1nr,1 -k 2n,2 | uniq -c | sed 's/\(.*\) \(.*\)/\1,\2/' > noobu_diff_rough.csv 
	sort -t, -k 2n,2 -k 3n,3 -k 4n,4 -k 5n,5 -k 6n,6  noobu_diff_rough.difmp | uniq -c | sed 's/\(.*\) \(.*\)/\1,\2/' > diff_comp_1.difmp
}

diff_comp()
{
	echo "0,0,0,0,0,0,0" >> diff_comp_1.difmp
	data=0000
	num=000
	cat diff_comp_1.difmp | while read line
	do
		prevdata=$data
		prevnum=$num
		echo $line | awk 'BEGIN{FS=","}{print "1," $1 "," $0}' >> com1.difmp
		data="$(echo $line | awk 'BEGIN{FS=","}{print $3 $4 $5 $6 $7 }')"
		num="$(echo $line | awk 'BEGIN{FS=","}{print $1}')"
			if [ $data -eq $prevdata ]
			then 
				lineprev1="$(sed -e '$!{h;d}' -e x com1.difmp)"
				echo $lineprev1 >> com2.difmp
			else
			if [ -f com2.difmp ]
			then 
				lineprev1="$(sed -e '$!{h;d}' -e x com1.difmp)"
				echo $lineprev1 >> com2.difmp
				for i in $(cat com2.difmp | awk -F "," '{print $2}');do let "sum = sum + $i " ;done
				cat com2.difmp | while read line1
				do
					echo $line1 | awk -v su=$sum -F "," '{printf("%.2f",$2/su)}{print "," su "," $3 "," $4 "," $5 "," $6 "," $7 "," $8 "," $9  }' >> com3.difmp 
				done
				echo " " >> com3.difmp
				sum=0
				rm com2.difmp && echo -n ".."
			else
				sed -e '$!{h;d}' -e x com1.difmp >> com3.difmp
				echo " " >> com3.difmp
			fi

		fi
	done
	
	ls output/diff && echo "the dir output/diff exist,omit ..." || mkdir -p -v output/diff 
	sed -i -n '2,$p' com3.difmp
	awk -F "," 'BEGIN { OFS="," } {for(i=5;i<=9;i++) if($i=="3"){$i="1"} else if($i=="2"){$i="0"} else if($i=="1"){$i="-1"} print }' com3.difmp > diff_rough_result.csv && sed -i '1 cpercent,sum_count,count,pos,r11-r15,r12-r16,r13-r17,r14-r18,r19' diff_rough_result.csv  && mv diff_rough_result.csv output/diff/  



}

diff_precise()
{
cat prev_diff_4.difmp | awk -F "," '{print $6 "," $1 "," $2 "," $3 "," $4 "," $5 }' | sort -t, -k 2d,2 -k 3d,3 -k 4d,4 -k 5d,5 -k 6d,6 | uniq -c | sed 's/\(.*\) \(.*\)/\1,\2/' > diff_precise_1.difmp
sort -t, -k 2n,2 -k 1nr,1  diff_precise_1.difmp > output/diff/diff_precise_result.csv && sed -i '1 icount,pos,r11-r15,r12-r16,r13-r17,r14-r18,r19' output/diff/diff_precise_result.csv
}






prev_diff_value

prev_diff

prev_diff_rough

diff_comp

diff_precise

rm *.difmp
