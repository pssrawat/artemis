import sys
import fileinput
import itertools
result = 0.0
numLines = 0

firstFile = 1
def sum(result, elem):
    return result + elem

operation = sum

if len(sys.argv) > 1 and sys.argv[1] == "--min":
    result = float('inf')
    operation = min

if len(sys.argv) > 1 and sys.argv[1] == "--max":
    result = 0
    operation = max

for i in range(1, len(sys.argv)):
    if sys.argv[i].startswith('-'):
        firstFile = i + 1
        continue
    break

for line in fileinput.input(itertools.chain(['-'], sys.argv[firstFile:])):
    result = operation(result, float(line))
    numLines += 1

if len(sys.argv) > 1 and sys.argv[1] == "--ave":
    result /= numLines

print(result)
