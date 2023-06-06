
import subprocess

def run (seed):
    subprocess.call(['./seals', '-seals', '-rep-max-neighbour', '-seed', str(seed)])

max = 100
for i in range(1, max+1):
    print('==== Running with seed ' + str(i) + '/' + str(max) + ' ====')
    run(i)
