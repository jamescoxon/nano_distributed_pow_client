# nano_distributed_pow_client

Welcome to the nano community's Distributed Proof of Work (DPoW) client. The DPoW system allows any user to support the nano network by computing the required work for transactions. This can help a lot of community-supported projects, such as faucets, tipping bots, and wallets.

## Installation

### Windows

See [the Windows instructions](windows_instructions.md) for W10 x64.

### Linux
```bash
git clone https://github.com/jamescoxon/nano_distributed_pow_client.git
cd nano_distributed_pow_client
sudo apt-get install python3-dev
```

#### Build
To use CPU.
```bash
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

### Using [dpow-work-server](https://github.com/guilhermelawless/dpow-work-server)

This is an alternative to building the libraries above.

- `sudo apt-get install cargo`

- Follow instructions in [dpow-work-server](https://github.com/guilhermelawless/dpow-work-server).

- Run nano-work-server: `./dpow-work-server --gpu 0:0 --listen-address 127.0.0.1:7076`

Run the client

```bash
nano-dpow-client --node --address YOUR_NANO_ADDRESS
```

## Configuration

About the two types of clients, doing **on demand** or **precached** work:

- If you are using your GPU, consider only computing on demand work by adding --work-type urgent_only. This is the type of work that needs to be computed as soon as possible.

- If you are using CPU, you will be timed out from on demand work if you can't complete it within 6 seconds. These clients are still very useful, but should add --work-type precache_only so that they only compute precache work, which has no time restrictions.

Run `nano-dpow-client --help` for configuration options. The address is mandatory, but not checked to be an existing nano account.

To save your configuration, add `--save-config` when running the client. A configuration file `.nano-dpow-client.conf` will be placed in the user's `$HOME` directory. This can be manually modified.

In case the same argument exists in the configuration file and is provided by argument, the argument takes priority.

Your address is used to keep track of your work preferences, statistics, and to send payouts.
