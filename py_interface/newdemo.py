import xandri
import numpy as np

xs = np.arange(0, 20000)
ys = np.sin(xs/10.) + 3*np.cos(xs/20.)
ys3 = 2*np.sin(xs/10.) + 4*np.cos(xs/20.)
xs2 = np.arange(0, 3000)
ys2 = 10*np.sin(xs2/30.) + 3*np.cos(xs2/60.)

with xandri.ssiborg("demo2") as plt:
    x = plt.index(xs)
    plt.plot(x, ys, "lolvarname")
    plt.plot(x, ys3, "sameindex")
    x2 = plt.index(xs2)
    plt.plot(x2, ys2)

print("k done")
