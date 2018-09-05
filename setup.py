from distutils.core import setup, Extension

setup(name='_xandri', version='0.01',  \
      ext_modules=[Extension('_xandri', ['py_interface/_xandri.c'],
                   extra_objects=['build/libxandri.a'])])
