import requests, argparse, ctypes, time, sys

address = ''
try:
    with open('address.txt') as f:
        address = f.readline().strip()
except Exception as e:
    pass
  
if(len(address) == 0):
    parser = argparse.ArgumentParser()
    parser.add_argument('--address', type=str, required=True, metavar='XRB/NANO_ADDRESS', help='Payout address.')
    parser.add_argument('--node', action='store_true', help='Compute work on the node. Default is to use libmpow.')
    args = parser.parse_args()
    
    address = args.address
    pow_souce = args.node

print("Welcome to Distributed Nano Proof of Work System")
print("All payouts will go to %s" % address)
if pow_souce == 0:
    print("You have selected local PoW processor (libmpow)")
elif pow_source == 1:
    print("You have selected node PoW processor (work_server or nanocurrency node)")

print("Waiting for work...", end='', flush=True)
while 1:
  try:
    r = requests.get('http://178.62.11.37/request_work')
    if not r.ok:
        print("Server request error {} - {}".format(r.status_code, r.reason))
        time.sleep(10)
        continue
    hash_result = r.json()
    if hash_result['hash'] != "error":
        print("\nGot work")
        t = time.time()
        if pow_source == 0:
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
        print("{} - took {:.2f}s".format(work, time.time()-t))
        json_request = '{"hash" : "%s", "work" : "%s", "address" : "%s"}' % (hash_result['hash'], work, address)

        json_request = '{"hash" : "%s", "work" : "%s", "address" : "%s"}' % (hash_result['hash'], work, args.address)

        r = requests.post('http://178.62.11.37/return_work', data = json_request)
        print(r.text)
        print("Waiting for work...", end='', flush=True)

  except KeyboardInterrupt:
      print("\nCtrl-C detected, canceled by user")
  except Exception as e:
      print("Error: {}".format(e))

  time.sleep(5)
  print('.', end='', flush=True)
