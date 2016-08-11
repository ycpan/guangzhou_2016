#!/bin/bash

##############################################################################
#源文件格式要求为 TIME,OBU,VAL,ROUTE1,RSSI1.ROUTE2,RSSI2,ROUTE3,RSSI3,ROUTE4,RSSI4,POS
#初步排序和统计
#处理格式化数据，并且根据rssi强弱进行顺序调整，然后进行第一路由第二路由进行排序
#输入文件名称为pos_format_result,输出临时文件名称为*.prevcomp,正式输出文件名称为pre
#v_comp.csv,位置./output/order/
##############################################################################
order()
{
cat pos_format_result | awk 'BEGIN{FS=","}{print $4 ","$5 "," $6 "," $7 "," $8 "," $9 "," $10 "," $11 "," $12 "," $2 "," $3 "," $1 }' > prevorder.prevcomp

cat prevorder.prevcomp | while read line
do
echo $line | awk 'BEGIN{FS = "," } {for (i=0;i<11;i++) a[i]=$i} {for (j=1;j<5;j++)  if (a[2] < a[2*j]) {b=a[2];s=a[1];a[2]=a[2*j];a[1]=a[2*j-1];a[2*j]=b;a[2*j-1]=s}} {for (k=2;k<5;k++) if (a[4] < a[2*k]) {b=a[4];s=a[3];a[4]=a[2*k];a[3]=a[2*k-1];a[2*k]=b;a[2*k-1]=s}}{for (m=3;m<5;m++) if (a[6] < a[2*m]) {b=a[6];s=a[5];a[6]=a[2*m];a[5]=a[2*m-1];a[2*m]=b;a[2*m-1]=s}} {print  a[1] ","a[3]"," a[5] ","a[7]","a[9]}' >> 2.prevcomp
#echo $line | awk 'BEGIN{FS = "," } {for (i=0;i<11;i++) a[i]=$i} {for (j=1;j<5;j++)  if (a[2] < a[2*j]) {b=a[2];s=a[1];a[2]=a[2*j];a[1]=a[2*j-1];a[2*j]=b;a[2*j-1]=s}} {for (k=2;k<5;k++) if (a[4] < a[2*k]) {b=a[4];s=a[3];a[4]=a[2*k];a[3]=a[2*k-1];a[2*k]=b;a[2*k-1]=s}}{for (m=3;m<5;m++) if (a[6] < a[2*m]) {b=a[6];s=a[5];a[6]=a[2*m];a[5]=a[2*m-1];a[2*m]=b;a[2*m-1]=s}} {print  a[1] ","a[2] ","a[3]","a[4]"," a[5] ","a[6]","a[7] ", "a[8]","a[9]","a[10]","}' >> 2.prevcomp

done
#cp 2.prevcomp order_result.csv && rm -rf *.prevcomp
#(sort 2 | uniq -c >3) && (sort -nr 3 > 4) && (:>1)
(sort 2.prevcomp | uniq -c >3.prevcomp) && (sort -nr 3.prevcomp | awk -F " " '{print $1 ","$2}' > 4.prevcomp) && sort -t, -k2,3  4.prevcomp -o prevcomp.prevcomp 
ls output/order/ && echo "the dir output/order exist,omit ..." || mkdir -v output/order
sort -t, -k 2n,2 -k 3n,3 -k 4n,4 -k 5n,5 -k 1nr,1 prevcomp.prevcomp -o m_prevcomp.prevcomp
cp m_prevcomp.prevcomp prevcomp.csv && mv prevcomp.csv output/order 
}

comp()
{
echo "0,0,0,0,0,0" >> m_prevcomp.prevcomp
data=0000
num=000
cat m_prevcomp.prevcomp | while read line
do
prevdata=$data
prevnum=$num
echo $line | awk 'BEGIN{FS=","}{print "1," $1 "," $0}' >> test1prevcomp
data="$(echo $line | awk 'BEGIN{FS=","}{print $2 $3 $4 $5}')"
num="$(echo $line | awk 'BEGIN{FS=","}{print $1}')"
#echo $prevdata
#echo $data
if [ $data -eq $prevdata ]
then 
lineprev1="$(sed -e '$!{h;d}' -e x test1prevcomp)"
echo $lineprev1 >> test1.compprevcomp
#echo $lineprev1
#echo $num
#echo $prevnum
else
if [ -f test1.compprevcomp ]
then 
lineprev1="$(sed -e '$!{h;d}' -e x test1prevcomp)"
echo $lineprev1 >> test1.compprevcomp
for i in $(cat test1.compprevcomp | awk -F "," '{print $2}');do let "sum = sum + $i " ;done
#echo "the sum is $sum" >> data.test
cat test1.compprevcomp | while read line1
do
#echo $line1 >> data.test
echo $line1 | awk -v su=$sum -F "," '{printf("%.2f",$2/su)}{print "," su "," $3"," $4 ","$5"," $6"," $7 "," $8 }' >> test1.dataprevcomp 
#echo "the avg is $avg"
done
echo " " >> test1.dataprevcomp
sum=0
rm test1.compprevcomp && echo -n "."
else
sed -e '$!{h;d}' -e x test1prevcomp >> test1.dataprevcomp
echo " " >> test1.dataprevcomp
fi
fi
done
#ls output/order/ && echo -n "." || mkdir -v output/order
cp test1.dataprevcomp order_avgresult.csv && sed '1 cpercent,sum_count,count,route1,route2,route3,route4,pos' order_avgresult.csv -i && mv order_avgresult.csv output/order && rm *prevcomp

#echo "0,0,0,0,0,0" >> test1
#addprev
#rm *prevcomp

}

order

comp
