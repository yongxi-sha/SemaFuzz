import os
from distutils.core import setup, Extension

os.environ["CC"]  = "clang"
os.environ["CXX"] = "clang++"

semgdump_moudle = Extension('semgdump',
                    define_macros = [('MAJOR_VERSION', '1'), ('MINOR_VERSION', '0')],
                    extra_link_args=[],
                    extra_compile_args=['-std=c++17'],
                    include_dirs = ['./include'],
                    libraries = ['shmQueue', "comgraph"],
                    library_dirs = [],
                    sources = ['./source/dump.cpp'])


setup(
    name='semgDumpModule',
    version='1.0',
    description='A Python package with a C extension for dynamic monitoring and dump semantic graph',
    ext_modules=[semgdump_moudle]
)

