
import subprocess

def run (seed):
    subprocess.call(['./seals', '-overdamped', '-iter', str(40000), '-particles', str(200), '-magnitude', str(0.005), '-growth', str(2), '-repulsion', str(1.8), '-seed', str(seed)])

max = 100
for i in range(1, max+1):
    print('==== Running with seed ' + str(i) + '/' + str(max) + ' ====')
    run(i)
