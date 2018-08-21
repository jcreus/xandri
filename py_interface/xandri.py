import os
import numpy as np
import _xandri

def load_hdf(name, df):
    os.system('rm -rf ../data/'+name) # lol
    writer = _xandri.Writer(name)

    indices = {}
    i = 0
    for var in df:
        ptr = df[var].index.values.__array_interface__['data'][0]
        if ptr not in indices:
            sz = df[var].index.values.dtype.itemsize
            tgt = {1: np.uint8, 2: np.uint16, 4: np.uint32, 8: np.uint64}[sz]
            writer.add_index("index%d"%i, df[var].index.values.astype(tgt, copy=False), sz)
            indices[ptr] = i
            i += 1
        idx = "index%d" % indices[ptr]
        width = df[var].values.dtype.itemsize
        writer.add_key(var, idx, df[var].values, width, typ)

    print(indices)
