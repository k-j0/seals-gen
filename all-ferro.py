
import subprocess

def run (seed):
    subprocess.call(['./seals', '-overdamped', '-particles', str(500), '-iter', str(120000), '-growth', str(6), '-magnitude', str(0.004), '-pressure', str(0.001), '-surface-tension', str(1.3), '-seed', str(seed)])

max = 100
for i in range(1, max+1):
    print('==== Running with seed ' + str(i) + '/' + str(max) + ' ====')
    run(i)
