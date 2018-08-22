import os
import numpy as np
import pandas as pd
import xandri

#df = pd.read_hdf('/home/joan/valbal/data/ssi63/smol.h5')#, stop=2000)
df = pd.read_hdf('/home/joan/valbal/data/ssi71/ssi71.h5')#, stop=2000)
df.index = df.index.astype(np.uint64) // 10**6

xandri.import_df("test63", df)
