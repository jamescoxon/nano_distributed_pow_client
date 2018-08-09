# nano_distributed_pow_client

##Installation

### Ubuntu
`git clone https://github.com/jamescoxon/nano_distributed_pow_client.git`

`cd nano_distributed_pow_client`

`sudo apt-get install libb2-dev`

`gcc -o mpow mpow.c -lb2 -fopenmp`

`python3 client.py`

Save your address in `address.txt` so the client doesn't ask for it every time it starts
