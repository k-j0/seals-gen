
import subprocess

def run (seed):
    subprocess.call(['./seals', '-overdamped', '-iter', str(120000), '-particles', str(600), '-magnitude', str(0.005), '-growth', str(5), '-repulsion', str(1.8), '-final-target-volume', str(0.01), '-pressure', str(0.00005), '-seed', str(seed)])

max = 20
for i in range(1, max+1):
    print('==== Running with seed ' + str(i) + '/' + str(max) + ' ====')
    run(i)
