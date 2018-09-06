# nano_distributed_pow_client

## Installation

### Ubuntu
```bash
git clone https://github.com/jamescoxon/nano_distributed_pow_client.git`
cd nano_distributed_pow_client
sudo apt-get install python3-dev
```

#### Build
To use CPU you need the blake2 library.
```bash
sudo apt-get install libb2-dev
python3 setup.py build
```

To use GPU you need opencl library and ICD. Install the ICD that is relevant for your GPU.
```bash
sudo apt-get install ocl-icd-opencl-dev
sudo apt-get install nvidia-opencl-icd
sudo apt-get install amd-opencl-icd
python3 setup.py build --enable-gpu
```

#### Install
```bash
python3 setup.py install
```

You can supply the flag `--user` to the install command if you want to install it just for 1 user and not system wide.

#### Run the client
```bash
nano-dpow-client
```

### Using [nano-work-server](https://github.com/nanocurrency/nano-work-server)

- `sudo apt-get install cargo`

- Follow instructions in [nano-work-server](https://github.com/nanocurrency/nano-work-server).

- Run nano-work-server: `./nano-work-server --gpu 0:0 --listen-address 127.0.0.1:7076`

Run the client

```bash
nano-dpow-client --node
```

## Configuration

See `.nano-dpow-client-conf` for configuration options. The configuration file should be placed in the user's `$HOME` directory. Alternatively use the command line when running the client. Do `client --help` for instructions.
