from setuptools import setup
import os
setup(
    setup_requires=["cffi>=1.0.0"],
    cffi_modules=[os.path.join(os.environ['LEGATO_ROOT'],"framework/tools/ifgen/langPython/builder.py:ffibuilder")],
    install_requires=["cffi>=1.0.0"],
)
