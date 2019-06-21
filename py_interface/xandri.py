import os
import webbrowser
import time
import errno
import random
import numpy as np
import _xandri
import string
import socket

PATH = os.path.dirname(os.path.dirname(_xandri.__file__))

def remove_df(name):
    os.system('rm -rf %s' % os.path.join(PATH, 'data', name))

def import_df(name, df):
    remove_df(name)
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

class ssiborg:
    def mkname(self):
        return ''.join([random.choice(string.ascii_lowercase) for _ in range(8)])
        
    def __init__(self, name=None):
        if name is None:
            name = self.mkname()
        self.name = name
        remove_df(self.name)
        self.writer = _xandri.Writer(self.name)

    def __enter__(self):
        return self

    def index(self, arr, name=None):
        if name is None:
            name = "index"+self.mkname() 
        dtype = arr.dtype
        if np.issubdtype(dtype, np.floating):
            raise ValueError("Floating point indices are not supported.")
        sz = dtype.itemsize
        self.writer.add_index(name, arr, sz)
        return name

    def plot(self, index, arr, name=None):
        if name is None:
            name = "var"+self.mkname() 
        dtype = arr.dtype
        width = dtype.itemsize
        if np.issubdtype(dtype, np.integer):
            typ = "int"
        elif np.issubdtype(dtype, np.floating):
            typ = "float"
        elif dtype == np.bool:
            typ = "int"
            width = 1
        else:
            raise ValueError('Unsupported type! '+repr(dtype))
        if typ == "int" and width == 8:
            print("WARNING: JavaScript inexplicably doesn't have native 64-bit ints. ssiborg will emulate them and will therefore be slower")
        print('adding', name)
        self.writer.add_key(name, index, arr, width, typ)


    def __exit__(self, exc_type, exc_value, traceback):
        ok = False
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.bind(("127.0.0.1", 2718))
            s.close()
        except OSError as e:
            if e.errno == errno.EADDRINUSE:
                print("You already have an open Xandri server, great!")
                ok = True
            else:
                print("what the hell", e)
        if not ok:
            name = "ssiborg%s" % (''.join([random.choice(string.ascii_lowercase) for _ in range(8)]))
            os.system("screen -S %s -dm bash -c 'cd %s; build/x serve -p 2718'" % (name, PATH))
            print("Started screen '%s' with a Xandri server..." % name)
        webbrowser.open("http://localhost/ssiborg")
        if input("Do you want to erase the resulting dataframe? [Y/n] ") in ["", "y", "Y"]:
            print("Removing...")
            remove_df(self.name)
