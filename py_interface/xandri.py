import os
import time
import numpy as np
import _xandri

def import_df(name, df):
    os.system('rm -rf ../data/'+name) # lol
    t0 = time.time()
    writer = _xandri.Writer(name)

    indices = {}
    i = 0
    for var in df:
        ptr = df[var].index.values.__array_interface__['data'][0]
        if ptr not in indices:
            sz = df[var].index.values.dtype.itemsize
            tgt = {1: np.uint8, 2: np.uint16, 4: np.uint32, 8: np.uint64}[sz]
            if sz == 8:
                print("WARNING: JavaScript inexplicably doesn't have native 64-bit ints. ssiborg will emulate them and will therefore be slower")
            writer.add_index("index%d"%i, df[var].index.values.astype(tgt, copy=False), sz)
            indices[ptr] = i
            i += 1
        idx = "index%d" % indices[ptr]
        dtype = df[var].values.dtype
        width = dtype.itemsize
        if np.issubdtype(dtype, np.integer):
            typ = "int"
        elif np.issubdtype(dtype, np.floating):
            typ = "float"
        elif dtype == np.bool:
            typ = "int"
            width = 1
        else:
            print('Unsupported type!', dtype)
            continue
        if typ == "int" and width == 8:
            print("WARNING: JavaScript inexplicably doesn't have native 64-bit ints. ssiborg will emulate them and will therefore be slower")
        print('adding', var)
        writer.add_key(var, idx, df[var].values, width, typ)
    print('Imported dataframe in %.2f ms, %d rows' % ((time.time()-t0)*1000, df.shape[0]))
