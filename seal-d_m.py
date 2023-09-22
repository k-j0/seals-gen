
import subprocess

def run(scale):
    subprocess.call([
        './seals',
        '-seals',
        '-rep-max-neighbour',
        '-compute-backbone-dim',
        '-magnitude', str(0.01/scale),
        '-boundary-radius', str(0.05/scale),
        '-boundary-target-density', str(50*scale),
        '-iter', str(20000 * scale**1.5)
    ])

run(5)
