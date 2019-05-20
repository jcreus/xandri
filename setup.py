from distutils.core import setup, Extension
import numpy as np

setup(name='_xandri', version='0.01',  \
      ext_modules=[Extension('_xandri', ['py_interface/_xandri.c'],
                   extra_objects=['build/libxandri.a'])],
      include_dirs = [np.get_include()])
