#!/bin/bash

# $1: number of processors
# $2: which party
# $3: edgeLength(number of ratings)
# $4: userLength(number of users)
# $5: itemLength(number of items)
# $6: epsilon (privacy parameter)

pkill -f FourParty
for i in $(seq 0 $(($1 - 1)))
do
	# echo ./FourParty $1 $i 40000 $2 50000 $3 $4 $5 $6 &
	./FourParty $1 $i 40000 $2 50000 $3 $4 $5 0.3 |& tee "$i.txt" & #|& tee "q$i.txt" &
done