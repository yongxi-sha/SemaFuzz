import os
import sys
from distutils.core import setup, Extension

cgdump_moudle = Extension('semcore',
                    define_macros = [('MAJOR_VERSION', '1'), ('MINOR_VERSION', '0')],
                    extra_link_args=['-lcomgraph'],
                    extra_compile_args=['-std=c++17'],
                    include_dirs = ['./include'],
                    sources = ['./source/semcore.cpp', './source/semmodule.cpp'])


setup(
    name='semcore',
    version='1.0',
    description='A Python package with a C extension for semmantic learning proxy',
    ext_modules=[cgdump_moudle]
)

