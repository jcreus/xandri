import re
import os
import pandas as pd
import numpy as np
import pickle

if not True:
    K = 20*5
    df = pd.read_hdf('/home/joan/valbal/data/actual63/ssi71.h5', stop=20*3600)
    fmt = open('/home/joan/balloons-VALBAL/src/AvionicsDownlink.cpp').read()
    N = "actual63"
else:
    K = 5
    df = pd.read_hdf('/home/joan/valbal/data/ssi63/smol.h5')
    #fmt = open('/home/joan/63val/src/Avionics.cpp').read()
    N = "ssi63"

v = df.index.values
v = (v - v[0])//1000000
v.astype(np.uint32).tofile('index.bin')

cmds = './x blob create actual63\n./x blob create-index actual63 time -b 4 index.bin\n'
for col in df:
    print(col, df[col].dtype)
    df[col].values.tofile(col+'.bin')
    dt = str(df[col].dtype)
    t = "float" if dt.startswith("float") else "int"
    w = 1
    if dt.endswith('16'): w = 2
    if dt.endswith('32'): w = 4
    if dt.endswith('64'): w = 8
    cmds += './x blob create-key actual63 %s time -t %s -b %d %s.bin\n' % (col, t, w, col)

open("cmds.sh", "w").write(cmds)

os.system("chmod +x cmds.sh")
