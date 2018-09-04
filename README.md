# nano_distributed_pow_client

## Installation

### Ubuntu
```bash
git clone https://github.com/jamescoxon/nano_distributed_pow_client.git`
cd nano_distributed_pow_client
sudo apt-get install python3-dev
```

To use CPU you need the blake2 library.
```bash
sudo apt-get install libb2-dev
python3 setup.py develop --user
```

To use GPU you need opencl library and ICD. Install the ICD that is relevant for your GPU.
```bash
sudo apt-get install ocl-icd-opencl-dev
sudo apt-get install nvidia-opencl-icd
sudo apt-get install amd-opencl-icd
python3 setup.py develop --user --enable-gpu
```

Run the client

```bash
python3 client.py
```

#### Using nano-work-server

[nano-work-server](https://github.com/nanocurrency/nano-work-server/tree/master).

- `sudo apt-get install cargo`

- Follow instructions in [nano-work-server](https://github.com/nanocurrency/nano-work-server/tree/master).

- Run nano-work-server: `./nano-work-server --gpu 0:0 --listen-address 127.0.0.1:7076`

Run the client

```bash
cd nano_distributed_pow_client
python3 client.py --node
```

## Configuration

See `client.conf` for configuration options. Alternatively use the command line when running the client. Do `python3 client.py --help` for instructions.
