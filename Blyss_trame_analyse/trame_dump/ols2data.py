import sys

print 'Getting raw ols data ...'
lines = sys.stdin.readlines()

print 'Parsing ols data ...'
rate = 1.0 / 200000
trigger = False
first_state = True
flip_state = 0
raw_times = []
for i in range(0, len(lines) - 1):
    if '@' not in lines[i]:
        if ';Rate' in lines[i]:
            rate = 1.0 / int(lines[i].split(':')[1], 10)
            print 'New frequency rate :', 1 / rate, 'Hz'
        continue
    
    raw_text_1 = lines[i].split('@')
    raw_text_2 = lines[i + 1].split('@')
		
    state_1 = int(raw_text_1[0], 10)
    
    if first_state:
        flip_state = state_1
    
    absolute_time_1 = int(raw_text_1[1], 10)
    absolute_time_2 = int(raw_text_2[1], 10)

    relative_time = absolute_time_2 - absolute_time_1
    print 'Level', flip_state, 'for', relative_time, 'samples =', (rate * relative_time) * 1000, 'ms'

    if (rate * relative_time) * 1000000 > 2000 and flip_state == 1:
        print 'Frame start trigerred !'
        trigger = True
        flip_state ^= 1
        continue

    if (rate * relative_time) * 1000000 > 2000 and flip_state == 0 and trigger == True:
        print 'Frame end trigerred !'
        break
        
    flip_state ^= 1 

    if not trigger :
        continue

    if (rate * relative_time) * 1000000 > 500:
        raw_times.append('2T')
    else:
        raw_times.append('T')

print 'Decoding low level data ...'
raw_bits = []
counter = 0
for i in range(0, len(raw_times) - 1, 2):
    if counter % 8 == 0:
        print 'Decoding byte', counter / 8
        
    if raw_times[i] == 'T' and raw_times[i + 1] == '2T':
        raw_bits.append(1)
    else:
        if raw_times[i] == '2T' and raw_times[i + 1] == 'T':
            raw_bits.append(0)
        else:
            print 'Error: unexcepted level transition detected !'
            exit(1)
    counter += 1

print 'Decoding binary data ...'
final_data = []
offset = len(raw_bits) % 8
if offset != 0:
    for i in range(0, offset):
        raw_bits.append(0)

for i in range(0, len(raw_bits) + offset - 8, 8):
    binary = raw_bits[i] * 128 + raw_bits[i + 1] * 64 + raw_bits[i + 2] * 32 + raw_bits[i + 3] * 16 + raw_bits[i + 4] * 8 + raw_bits[i + 5] * 4 + raw_bits[i + 6] * 2 + raw_bits[i + 7]
    final_data.append(binary)

print 'Decode data in stream :'
for data in final_data:
    print "%.2X" % data,
