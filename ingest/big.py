import re
import os
import pandas as pd
import numpy as np
import pickle

if True:
    K = 20*5
    df = pd.read_hdf('/home/joan/valbal/data/ssi71/ssi71.h5')
    fmt = open('/home/joan/balloons-VALBAL/src/AvionicsDownlink.cpp').read()
    N = "ssi71"
else:
    K = 5
    df = pd.read_hdf('/home/joan/valbal/data/ssi63/smol.h5')
    fmt = open('/home/joan/63val/src/Avionics.cpp').read()
    N = "ssi63"

v = df.index.values
v = (v - v[0])//1000000
v.astype(np.uint32).tofile('index.bin')

big = './x blob create ssi71\n./x blob create-index ssi71 time -b 4 ingest/index.bin\n'
for col in df:
    print(col, df[col].dtype)
    df[col].values.tofile(col+'.bin')
    dt = str(df[col].dtype)
    t = "float" if dt.startswith("float") else "int"
    w = 1
    if dt.endswith('16'): w = 2
    if dt.endswith('32'): w = 4
    if dt.endswith('64'): w = 8
    big += './x blob create-key ssi71 %s time -t %s -b %d ingest/%s.bin\n' % (col, t, w, col)

open("big.sh", "w").write(big)

os.system("chmod +x big.sh")
