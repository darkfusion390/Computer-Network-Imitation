rm -f prof.txt
touch prof.txt

# Upto 95

drop="5"
while [ $drop -lt 100 ]
do
	dr=$(echo "scale=2; $drop/100" | bc)
	echo $dr
	for i in {1..10}
	do
		(./server $dr 0) &
		sleep 1 ; (./relay $dr 0) &
		sleep 1 ; ./client 0 ;
		wait
	done
	drop=$[$drop+5]
done

# From 96 to 99

drop="96"
while [ $drop -lt 100 ]
do
	dr=$(echo "scale=2; $drop/100" | bc)
	echo $dr
	for i in {1..5}
	do
		(./server $dr 0) &
		sleep 1 ; (./relay $dr 0) &
		sleep 1 ; ./client 0 ;
		wait
	done
	drop=$[$drop+1]
done

### Averaging

rm -f prof_avg.txt

drp="5"
while [ $drp -lt 100 ]
do
        dr=$(echo "scale=2; $drp/100"|bc)
        cnt=$(grep -c "^0$dr" prof.txt)
        s=$(grep "^0$dr" prof.txt | grep -o " [0-9]*.[0-9]*" | tr -d \  | awk '{s+=$1} END {print s}')
        (printf "0$dr " && echo "scale=2;$s/$cnt" | bc) >> prof_avg.txt

        drp=$[$drp+5]
done

drp="96"
while [ $drp -lt 100 ]
do
        dr=$(echo "scale=2; $drp/100"|bc)
        cnt=$(grep -c "^0$dr" prof.txt)
        s=$(grep "^0$dr" prof.txt | grep -o " [0-9]*.[0-9]*" | tr -d \  | awk '{s+=$1} END {print s}')
        (printf "0$dr " && echo "scale=2;$s/$cnt" | bc) >> prof_avg.txt

        drp=$[$drp+1]
done

gnuplot -p -e "set xlabel 'Dropping probability'; set ylabel 'Time taken'; set title 'File size : 1580 bytes' ; set key off ;
plot 'prof.txt', 'prof_avg.txt' smooth csplines" > /dev/null 2>&1