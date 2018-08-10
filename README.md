# nano_distributed_pow_client

##Installation

### Ubuntu
`git clone https://github.com/jamescoxon/nano_distributed_pow_client.git`

`cd nano_distributed_pow_client`

`sudo apt-get install libb2-dev`

Compile library `gcc -lb2 -fopenmp -shared -Wl,-soname,libmpow -o libmpow.so -fPIC mpow.c`

Optionally, compile executable `gcc -o mpow mpow.c -lb2 -fopenmp`

`python3 client.py`
