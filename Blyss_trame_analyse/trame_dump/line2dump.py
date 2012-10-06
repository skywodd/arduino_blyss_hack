import sys
lines = sys.stdin.readlines()
counter = 0

for line in lines:
    if counter % 10 == 0:
        print "\n",
    print line[:-1],
    counter += 1
