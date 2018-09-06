from setuptools import setup, Extension
import sys, os


os.environ["CC"] = "gcc"
if sys.platform == 'darwin':
    os.environ["CC"] = "gcc-8"

eca = []
ela = []
libs = []
macros = []

if '--enable-gpu' in sys.argv:
    sys.argv.remove('--enable-gpu')
    libs = ['OpenCL']
    macros = [('HAVE_CL_CL_H', '1')]
    if sys.platform == 'darwin':
        macros = [('HAVE_OPENCL_OPENCL_H', '1')]
        ela=['-framework', 'OpenCL']
else:
    libs = ['b2']
    eca = ['-fopenmp']

setup(
    name="nano-dpow-client",
    version='0.0.1',
    description='Work client for the Nano (cryptocurrency) Distributed Proof of Work System. Supports CPU and GPU.',
    url='https://github.com/jamescoxon/nano_distributed_pow_client',
    author='James Coxon',
    author_email='james@joltwallet.com',
    scripts=['client'],
    license='MIT',
    python_requires='>=3.0',
    install_requires=[
    'websocket-client', 'requests'],
    ext_modules=[
        Extension(
            'mpow',
            sources=['mpow.c'],
            extra_compile_args=eca,
            extra_link_args=ela,
            libraries=libs,
            define_macros=macros)
    ])


