# nano_distributed_pow_client

Welcome to the nano community's Distributed Proof of Work (DPoW) client. The DPoW system allows any user to support the nano network by computing the required work for transactions. This can help a lot of community-supported projects, such as faucets, tipping bots, and wallets.

## Installation

### Linux
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
nano-dpow-client --address YOUR_NANO_ADDRESS
```

### Using [nano-work-server](https://github.com/nanocurrency/nano-work-server)

This is an alternative to building the libraries above.

- `sudo apt-get install cargo`

- Follow instructions in [nano-work-server](https://github.com/nanocurrency/nano-work-server).

- Run nano-work-server: `./nano-work-server --gpu 0:0 --listen-address 127.0.0.1:7076`

Run the client

```bash
nano-dpow-client --node --address YOUR_NANO_ADDRESS
```

## Configuration

Run `nano-dpow-client --help` for configuration options. The address is mandatory, but not checked to be an existing nano account.

To save your configuration, add `--save-config` when running the client. A configuration file `.nano-dpow-client.conf` will be placed in the user's `$HOME` directory. This can be manually modified.

In case the same argument exists in the configuration file and is provided by argument, the argument takes priority.

Your address is used to keep track of your work preferences, statistics, and to send small payouts in the future.
