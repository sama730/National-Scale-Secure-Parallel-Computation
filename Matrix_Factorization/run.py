#!/usr/bin/python
import subprocess
import os.path
import time
import sys

# $1: processors(number of machines)
# $2: party
# $3: edgeLength(number of ratings)
# $4: userLength(number of users)
# $5: itemLength(number of items)
# $6: epsilon (privacy parameter)

processors = sys.argv[1]
party = sys.argv[2]
edgeLength = sys.argv[3]
userLength = sys.argv[4]
itemLength = sys.argv[5]
epsilon = sys.argv[6]

params = str(processors) + " " + str(party) + " " + str(edgeLength) + " " + str(userLength) + " " + str(itemLength) + " " + str(epsilon)
subprocess.call(["./runAll.sh " + params], shell=True)



