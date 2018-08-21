import os
import pandas as pd
import xandri

df = pd.read_hdf('/home/joan/valbal/data/ssi63/smol.h5', stop=2000)

xandri.load_hdf("test63", df)
