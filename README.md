# nano_distributed_pow_client

## Installation

### Ubuntu
`git clone https://github.com/jamescoxon/nano_distributed_pow_client.git`

```bash
cd nano_distributed_pow_client
sudo apt-get install libb2-dev
autoconf
./configure
make
```

To enable GPU,
```
sudo apt-get install ocl-icd-opencl-dev
autoconf
./configure --enable-gpu
make
```

Optionally, compile executable
`make mpow`

```bash
python3 client.py
# Choose to compute PoW locally
```

#### Using GPU

Using a GPU is much faster, but it's not yet implemented directly in the client, so we'll use [nano-work-server](https://github.com/nanocurrency/nano-work-server/tree/master).

- `sudo apt-get install cargo`

- Follow instructions in [nano-work-server](https://github.com/nanocurrency/nano-work-server/tree/master).

- Run nano-work-server: `./nano-work-server --gpu 0:0 --listen-address 127.0.0.1:7076`

Run the client

```bash
cd nano_distributed_pow_client
python3 client.py
# Choose to connect to the node
```

## Configuration

See `client.conf` for configuration options. Alternatively use the command line when running the client. Do `python3 client.py --help` for instructions.
