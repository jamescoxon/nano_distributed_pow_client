from setuptools import setup, Extension
import sys, os

def which(pgm):
    path=os.getenv('PATH')
    for p in path.split(os.path.pathsep):
        p=os.path.join(p,pgm)
        if os.path.exists(p) and os.access(p,os.X_OK):
            return p

if sys.platform != 'win32':
    os.environ["CC"] = "gcc"
if sys.platform == 'darwin':
    gcc=None
    for i in range(9, 5, -1):
        gcc = 'gcc-'+str(i)
        if which(gcc):
            os.environ["CC"] = gcc
            break

eca = []
ela = []
libs = []
macros = []
src = []

if '--enable-gpu' in sys.argv:
    sys.argv.remove('--enable-gpu')
    if '--use-vc' in sys.argv:
        sys.argv.remove('--use-vc')
    libs = ['OpenCL']
    macros = [('HAVE_CL_CL_H', '1')]
    if sys.platform == 'darwin':
        macros = [('HAVE_OPENCL_OPENCL_H', '1')]
        ela=['-framework', 'OpenCL']
    src = ['mpow.c']
else:
    eca = ['-fopenmp']
    ela=['-fopenmp']
    src = ['b2b/blake2b.c', 'mpow.c']
    if '--use-vc' in sys.argv:
        sys.argv.remove('--use-vc')
        macros = [('USE_VISUAL_C', '1')]
        eca = ['/openmp', '/arch:SSE2', '/arch:AVX', '/arch:AVX2']
        ela = ['/openmp', '/arch:SSE2', '/arch:AVX', '/arch:AVX2']

setup(
    name="nano-dpow-client",
    version='1.1.1',
    description='Work client for the Nano (cryptocurrency) Distributed Proof of Work System. Supports CPU and GPU.',
    url='https://github.com/jamescoxon/nano_distributed_pow_client',
    author='James Coxon',
    author_email='james@joltwallet.com',
    scripts=['nano-dpow-client'],
    license='MIT',
    python_requires='>=3.0',
    install_requires=[
    'websocket-client', 'requests'],
    ext_modules=[
        Extension(
            'mpow',
            sources=src,
            extra_compile_args=eca,
            extra_link_args=ela,
            libraries=libs,
            define_macros=macros)
    ])


