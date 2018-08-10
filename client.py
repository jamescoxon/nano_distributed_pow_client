import requests, argparse, ctypes, time, sys

parser = argparse.ArgumentParser()
parser.add_argument('--address', type=str, required=True, metavar='XRB/NANO_ADDRESS', help='Payout address.')
parser.add_argument('--node', action='store_true', help='Compute work on the node. Default is to use libmpow.')
args = parser.parse_args()

print("Welcome to Distributed Nano Proof of Work System")

print("All payouts will go to %s" % args.address)

print("Waiting for work...")

while 1:
  try:
    r = requests.get('http://178.62.11.37/request_work')
    hash_result = r.json()
    if hash_result['hash'] != "error":
        if not args.node:
            try:
                lib=ctypes.CDLL("./libmpow.so")
                lib.pow_generate.restype = ctypes.c_char_p
                work = lib.pow_generate(ctypes.c_char_p(hash_result['hash'].encode("utf-8"))).decode("utf-8")
                print(work)

            except:
                print("Error - no mpow binary")
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
        json_request = '{"hash" : "%s", "work" : "%s", "address" : "%s"}' % (hash_result['hash'], work, args.address)
        r = requests.post('http://178.62.11.37/return_work', data = json_request)
        print(r.text)

  except:
    print("Error")

  time.sleep(5)
