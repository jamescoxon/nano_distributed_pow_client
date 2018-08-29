import requests, argparse, ctypes, time, sys, configparser, json
import websocket #websocket-client
from websocket import create_connection

address = ''

config = configparser.ConfigParser()
try:
	config.read('client.conf')
	if 'DEFAULT' in config:
		if 'address' in config['DEFAULT']: address=config['DEFAULT']['address']
except:
	pass

parser = argparse.ArgumentParser()
parser.add_argument('--address', type=str, required=(len(address)==0), metavar='XRB/NANO_ADDRESS', help='Payout address.')
parser.add_argument('--node', action='store_true', help='Compute work on the node. Default is to use libmpow.')
args = parser.parse_args()

if args.address: address = args.address

print("Welcome to Distributed Nano Proof of Work System")
print("All payouts will go to %s" % address)
if not args.node:
    print("You have selected local PoW processor (libmpow)")
else:
    print("You have selected node PoW processor (work_server or nanocurrency node)")

node_server = 'ws://yapraiwallet.space:5000/group/'

try:
    ws = create_connection(node_server)
except:
    print('\nError - unable to connect to backend server\nTry again later or change the server in config.ini')
    sys.exit()

print("Waiting for work...", end='', flush=True)
while 1:
  try:
    hash_result = json.loads(str(ws.recv()))

    if hash_result['hash'] != "error":
        print("\nGot work")
        t = time.perf_counter()
        if not args.node:
            try:
                lib=ctypes.CDLL("./libmpow.so")
                lib.pow_generate.restype = ctypes.c_char_p
                work = lib.pow_generate(ctypes.c_char_p(hash_result['hash'].encode("utf-8"))).decode("utf-8")
                print(work)
            except KeyboardInterrupt:
                print("\nCtrl-C detected, canceled by user")
                break;
            except Exception as e:
                print("Error: {}".format(e))
                sys.exit()

        else:
            try:
                rai_node_address = 'http://%s:%s' % ('127.0.0.1', '7076')
                get_work = '{ "action" : "work_generate", "hash" : "%s", "use_peers": "true" }' % hash_result['hash']
                r = requests.post(rai_node_address, data = get_work)
                resulting_work = r.json()
                work = resulting_work['work'].lower()
            except:
                print("Error - failed to connect to node")
                sys.exit()
        print("{} - took {:.2f}s".format(work, time.perf_counter()-t))
        
        json_request = '{"hash" : "%s", "work" : "%s", "address" : "%s"}' % (hash_result['hash'], work, address)
        ws.send(json_request)

        print("Waiting for work...", end='', flush=True)

  except KeyboardInterrupt:
      print("\nCtrl-C detected, canceled by user")
  except Exception as e:
      print("Error: {}".format(e))
      print("Reconnecting...")
      time.sleep(15)
      try:
          ws = create_connection(node_server)
          print("Connected")
      except:
          pass

  time.sleep(5)
  print('.', end='', flush=True)
