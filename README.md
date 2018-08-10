# nano_distributed_pow_client

## Installation

### Ubuntu
`git clone https://github.com/jamescoxon/nano_distributed_pow_client.git`

#### Using CPU

```bash
cd nano_distributed_pow_client
sudo apt-get install libb2-dev
```
Compile library 
`gcc -lb2 -fopenmp -shared -Wl,-soname,libmpow -o libmpow.so -fPIC mpow.c`

Optionally, compile executable 
`gcc -o mpow mpow.c -lb2 -fopenmp`
```
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

## Tips

Save your address in `address.txt` so the client doesn't ask for it every time it starts
