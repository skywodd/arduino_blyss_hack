import sys
lines = sys.stdin.readlines()

for line in lines:
    line = line.replace('ACK', '')
    line = line.replace('NACK', '')
    line = line.replace('   ', '\n')
    fields = line.split('\n')
    for field in fields:
        print field
