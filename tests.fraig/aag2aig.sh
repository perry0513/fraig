for i in *.aag; do
	echo $i
	aigtoaig $i ${i%.*}.aig
done
