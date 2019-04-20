# Windows installation instructions

These instructions are for Windows 10 x64 and using a GPU. Adapting to use CPU is fine, but please run as a precache client only, which does not have time restrictions.

Huge thanks to laserwean for the [original instructions](https://www.reddit.com/r/nanocurrency/comments/a25m8y/howto_run_distributed_pow_service_using_gpu_on/), which were adapted for completeness.

Looking for the old instructions with the node? They are outdated (the new instructions here are easier), but you can find them [here](windows_node_instructions.md).

## Download and run the work server

1. Download a pre-compiled binary of nano-work-server [here](https://files.nanocenter.org/work/nano-work-server.zip).

2. Extract the zip to any folder, which should contain **nano-work-server.exe**.

3. In the folder with the extracted file, use *shift + right-click* and select **Open Powershell window here**.

4. Write `ls` and *Enter* to make sure **nano-work-server.exe** is there.

5. Open Task Manager, and go into the Performance tab. There should be a GPU 0 device there, and maybe a GPU 1 if you have an integrated GPU as 0. Note here the number for the GPU you want to use.

5. Write `.\nano-work-server.exe --gpu 1:0 --listen-address 127.0.0.1:7076` and press *Enter*. If it fails, then try replace `1:0` by `0:0` after `--gpu`. Always wait a few seconds for initialization, and you should see some text such as "Initializing GPU: (...)".

All ready, you have **nano-work-server** running locally and listening to requests on port **7076**. Now we need to setup DPoW.

## Install Windows 10 Sub System for Linux, and Ubuntu

1. Open PowerShell as administrator: hit *Win*, write *Powershell*, right-click and Run as Administrator. Run:

  ```Enable-WindowsOptionalFeature -Online -FeatureName Microsoft-Windows-Subsystem-Linux```

2. Restart your computer.

3. Install [Ubuntu from Windows Store](https://www.microsoft.com/en-us/p/ubuntu/9nblggh4msv6).

4. Run Ubuntu from the start menu and finish its setup, see [this](https://i.redd.it/ghpd74y9kp121.png) image.

## Configure and run DPoW

Almost there!

Excecute the follwing commands within your ubuntu windows (one after the other, don't copy everything).

  ```
  cd ~/
  sudo apt-get update
  sudo apt install git python3-dev python3-setuptools libb2-dev build-essential -y
  
  git clone https://github.com/jamescoxon/nano_distributed_pow_client.git
  cd nano_distributed_pow_client
  python3 setup.py build
  sudo python3 setup.py install
  cd ~/
  ```

Test work generation (the node must be running). Your output should be similar to [this](https://i.redd.it/xjnrrr1pnp121.png):

  ```
  curl -sd '{ "action": "work_generate", "hash":"3E7C5911880A5DB34D12FD1D306B572ED5C0363A28D11414650537FE8BBBD174" }' localhost:7076
  ```
  
Now you can run the DPoW client normally>

  ```nano-dpow-client --node --address YOUR_NANO_ADDRESS```

About the two types of clients, doing **on demand** or **precached** work:

- If you are using your GPU, consider only computing on demand work by adding --work-type urgent_only. This is the type of work that needs to be computed as soon as possible.

- If you are using CPU, you will be timed out from on demand work if you can't complete it within 6 seconds. These clients are still very useful, but should add --work-type precache_only so that they only compute precache work, which has no time restrictions.
