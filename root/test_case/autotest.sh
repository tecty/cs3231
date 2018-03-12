sh $1 > $1.out
diff $1.out exp_out/$1.exp
rm $1.out 
