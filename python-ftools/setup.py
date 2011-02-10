from distutils.core import setup, Extension

setup(name='ftools',
    version='0.2.1',
    author='David Stainton',
    author_email='dstainton415@gmail.com',
    ext_modules=[
          Extension('ftools', ['../fallocate.c','ftools.c'], include_dirs=['..'], define_macros=[('_GNU_SOURCE', 1)])
      ],
      scripts=['scripts/python_fincore','scripts/python_fadvise']
)

