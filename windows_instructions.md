# Windows installation instructions

These instructions are for Windows 10 x64 and using a GPU. Adapting to use CPU is fine, but please run as a precache client only, which does not have time restrictions.

Huge thanks to laserwean for the [original instructions](https://www.reddit.com/r/nanocurrency/comments/a25m8y/howto_run_distributed_pow_service_using_gpu_on/), which were adapted for completeness.

## Download install, and configure the Node and Developer Wallet

### Download

We need any **developer** node, beta or main net. If you already have one, you can skip this step. The node can be found on [nano.org](https://nano.org/en).

### Install and run

Run the installer. Once you're finished, run the application. Now you have two choices. See [this](https://i.imgur.com/DZ2n5iM.png) image.

- If you want to use the node normally, to interact with the Nano network, then allow it on your Private network.
  
  **NOTE**: This will have high CPU, memory, disk and network usage.
  
- If you only want to use this for DPoW, then we should block it from the network. Click **cancel**. "Status: Disconnected" should appear in your wallet.

If you change your mind later, go into your firewall (default in Windows 10 is the Windows Defender Firewall) and remove all rai_node/nano_wallet/nano_node applications. It will ask you once again when running the application.

### Configure

1. Close the wallet and **wait 30 seconds**. Kill *nano_wallet.exe* in Task Manager if it's still there.
2. Hit Start->Run (or Win+R) and write: `%LOCALAPPDATA%\RaiBlocks` , hit enter. It should open your file explorer.
3. Delete all files inside except config.json
4. Open Task Manager, and go into the Performance tab. There should be a GPU 0 device there, and maybe a GPU 1 if you have an integrated GPU as 0. Note here the number for the GPU you want to use.
5. Change the following in config.json (note *GPU_ID_FROM_TASK_MANAGER* that you should change for 0 or 1, according to your case, i think):

  ```json
  "preconfigured_peers": [
    "localhost"
  ],
  
  (...)

  "rpc": {
    "address": "::ffff:0.0.0.0",
        "port": "7076",
        "enable_control": "true",
        "frontier_request_limit": "16384",
        "chain_request_limit": "16384",
        "max_json_depth": "20"
    },
    "rpc_enable": "true",
    "opencl_enable": "true",
    "opencl": {
        "platform": "GPU_ID_FROM_TASK_MANAGER",
        "device": "0",
        "threads": "1048576"
    }
  ```

6. Save and close the file.

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
  curl -sd '{ "action": "work_generate", "hash":"0" }' localhost:7076
  ```
  
Now you can run the DPoW client normally>

  ```nano-dpow-client --node --address YOUR_NANO_ADDRESS```

About the two types of clients, doing **on demand** or **precached** work:

- If you are using your GPU, consider only computing on demand work by adding --work-type urgent_only. This is the type of work that needs to be computed as soon as possible.

- If you are using CPU, you will be timed out from on demand work if you can't complete it within 6 seconds. These clients are still very useful, but should add --work-type precache_only so that they only compute precache work, which has no time restrictions.
