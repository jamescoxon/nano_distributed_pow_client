from setuptools import setup, Extension


def try_build():
    setup(
        name="nano_dpow",
        version='0.0.1',
        description='write a description',
        url='https://github.com/jamescoxon/nano_distributed_pow_client',
        author='James',
        author_email='add email',
        scripts=['client.py'],
        license='MIT',
        python_requires='>=3.0',
        install_requires=[
        'websocket-client'],
        ext_modules=[
            Extension(
                'mpow',
                sources=['mpow.c'],
                extra_compile_args=eca,
                extra_link_args=ela,
                libraries=libs,
                define_macros=macros)
        ])


eca = []
ela = []
try:
    print('\033[92m' + "Trying to build in GPU mode." + '\033[0m')
    libs = ['OpenCL']
    try:
        macros = [('HAVE_CL_CL_H', '1')]
        try_build()
    except:
        macros = [('HAVE_OPENCL_OPENCL_H', '1')]
        ela=['-framework', 'OpenCL']
        try_build()
    print('\033[92m' + "Success!!! Built with GPU work computation." +
          '\033[0m')
except:
    print('\033[91m' + "Failed to build in GPU mode." + '\033[0m')
    print('\033[92m' + "Trying to build in CPU mode." + '\033[0m')
    try:
        libs = ['b2']
        eca = ['-fopenmp']
        ela = []
        macros = []
        try_build()
        print('\033[92m' + "Success!!! Built with CPU work computation." +
              '\033[0m')
    except:
        print('\033[91m' + "Build failed." + '\033[0m')
