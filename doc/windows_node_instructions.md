# Setup the Node and Developer Wallet on Windows

## Download

We need any **developer** node, beta or main net. If you already have one, you can skip this step. The node can be found on [nano.org](https://nano.org/en).

## Install and run

Run the installer. Once you're finished, run the application. Now you have two choices. See [this](https://i.imgur.com/DZ2n5iM.png) image.

- If you want to use the node normally, to interact with the Nano network, then allow it on your Private network.
  
  **NOTE**: This will have high CPU, memory, disk and network usage.
  
- If you only want to use this for DPoW, then we should block it from the network. Click **cancel**. "Status: Disconnected" should appear in your wallet.

If you change your mind later, go into your firewall (default in Windows 10 is the Windows Defender Firewall) and remove all rai_node/nano_wallet/nano_node applications. It will ask you once again when running the application.

## Configure

1. Close the wallet and **wait 30 seconds**. Kill *nano_wallet.exe* in Task Manager if it's still there.
2. Hit Start->Run (or Win+R) and write: `%LOCALAPPDATA%\Nano` , hit enter. It should open your file explorer.
3. Delete all files inside except config.json
4. Open Task Manager, and go into the Performance tab. There should be a GPU 0 device there, and maybe a GPU 1 if you have an integrated GPU as 0. Note here the number for the GPU you want to use.
5. Change the following in config.json (note *GPU_ID_FROM_TASK_MANAGER* that you should change for 0 or 1, according to your case, i think):

```json
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
  }

  "node": {
    "peering_port": "0",

    "preconfigured_peers": [
      "localhost"
    ],

   "bootstrap_connections": "0",
   "bootstrap_connections_max": "0",
  }
  ```

6. Save and close the file.
