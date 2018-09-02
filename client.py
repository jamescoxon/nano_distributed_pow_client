import requests, argparse, ctypes, time, sys, configparser, json
import websocket #websocket-client, exceptions
from websocket import create_connection
from datetime import datetime # print timestamp

NODE_SERVER = 'ws://yapraiwallet.space:5000/group/'
WORK_SERVER = 'http://%s:%s' % ('127.0.0.1', '7076')

def get_socket():
    ws = create_connection(NODE_SERVER)
    ws.settimeout(5.0) # this causes a TimeOutException after 5 seconds stuck in .recv()
    return ws

def get_work_lib(from_hash):
    lib=ctypes.CDLL("./libmpow.so")
    lib.pow_generate.restype = ctypes.c_char_p
    work = lib.pow_generate(ctypes.c_char_p(from_hash.encode("utf-8"))).decode("utf-8")
    return work

def get_work_node(from_hash):
    get_work = '{ "action" : "work_generate", "hash" : "%s", "use_peers": "true" }' % from_hash
    r = requests.post(WORK_SERVER, data = get_work)
    resulting_work = r.json()
    work = resulting_work['work'].lower()
    return work

# ---- MAIN ---- #

payout_address = ''

config = configparser.ConfigParser()
try:
    config.read('client.conf')
    if 'DEFAULT' in config and 'address' in config['DEFAULT']:
        payout_address=config['DEFAULT']['address']
except:
    pass

parser = argparse.ArgumentParser()
parser.add_argument('--address', type=str, required=(len(payout_address)==0), metavar='XRB/NANO_ADDRESS', help='Payout address.')
parser.add_argument('--node', action='store_true', help='Compute work on the node. Default is to use libmpow.')
args = parser.parse_args()

if args.address: payout_address = args.address

print("Welcome to Distributed Nano Proof of Work System")
print("All payouts will go to %s" % payout_address)
if not args.node:
    print("You have selected local PoW processor (libmpow)")
else:
    print("You have selected node PoW processor (work_server or nanocurrency node)")

try:
    ws = get_socket()
except Exception as e:
    print('\nError - unable to connect to backend server\nTry again later or change the server in config.ini')
    sys.exit()

waiting = False
while 1:
    try:
        if not waiting:
            print("Waiting for work...", end='', flush=True)
            waiting = True

        try:
            hash_result = json.loads(str(ws.recv()))
        except websocket.WebSocketTimeoutException as e:
            print('.', end='', flush=True)
            continue

        waiting = False

        print("\n{} \tGot work".format(datetime.now()))
        t = time.perf_counter()

        if not args.node:
            work = get_work_lib(hash_result['hash'])
        else:
            work = get_work_node(hash_result['hash'])

        print("\t\t\t\t{} - took {:.2f}s".format(work, time.perf_counter()-t))

        json_request = '{"hash" : "%s", "work" : "%s", "address" : "%s"}' % (hash_result['hash'], work, payout_address)
        ws.send(json_request)

    except KeyboardInterrupt:
        print("\nCtrl-C detected, canceled by user\n")
        break

    except Exception as e:
        print("Error: {}".format(e))
        print("Reconnecting in 15 seconds...")
        del ws
        time.sleep(15)
        try:
            ws = get_socket()
            print("Connected")
        except:
            continue

