import requests, argparse, ctypes, time, sys, configparser, json
import websocket #websocket-client, exceptions
from websocket import create_connection
from datetime import datetime # print timestamp

NODE_SERVER = 'ws://yapraiwallet.space:5000/group/'
WORK_SERVER = 'http://%s:%s' % ('127.0.0.1', '7076')
WORK_TYPES = ['urgent_only', 'precache_only', 'any']

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

def is_desired_work_type(work, preferred):
    if preferred == 'any':
        return work in ['urgent', 'precache']
    if preferred == 'urgent_only':
        return work == 'urgent'
    if preferred == 'precache_only':
        return work == 'precache'
    return False


# returns empty str to signal that the option is not there
def config_safe_get_str(config, section, option):
    try:
        return config.get(section, option)
    except:
        return ''

# ---- MAIN ---- #

payout_address = ''
desired_work_type = ''

# get arguments with config file first
config = configparser.ConfigParser()
config.read('client.conf')
payout_address = config_safe_get_str(config, 'DEFAULT', 'address')
desired_work_type = config_safe_get_str(config, 'DEFAULT','work_type')

if desired_work_type not in WORK_TYPES:
    print('Invalid work_type set in config ( {} ). Choose from {} '.format(desired_work_type, WORK_TYPES))
    sys.exit()


# get arguments that were not given via config file
parser = argparse.ArgumentParser()
parser.add_argument('--address', type=str, required=(len(payout_address)==0), metavar='XRB/NANO_ADDRESS', help='Payout address.')
parser.add_argument('--work-type', type=str, action='store', choices=WORK_TYPES, required=(len(desired_work_type)==0), metavar='WORK_TYPE', help='Desired work type.')
parser.add_argument('--node', action='store_true', help='Compute work on the node. Default is to use libmpow.')
args = parser.parse_args()

if args.address: payout_address = args.address
if args.work_type: desired_work_type = args.work_type

print("Welcome to Distributed Nano Proof of Work System")
print("All payouts will go to %s" % payout_address)
print("You have chosen to only do work of type %s" % desired_work_type)
if not args.node:
    print("You have selected local PoW processor (libmpow)")
else:
    print("You have selected node PoW processor (work_server or nanocurrency node)")
print('\n')

try:
    ws = get_socket()

    set_work_type = '{"work_type" : "%s", "address": "%s"}' % (desired_work_type, payout_address)
    ws.send(set_work_type)

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
            waiting = False
        except websocket.WebSocketTimeoutException as e:
            print('.', end='', flush=True)
            continue

        has_type = 'type' in hash_result
        print("\n{} \tGot work ({})".format(datetime.now(), hash_result['type'] if has_type else 'unknown'))

        # check if work type is the expected
        if has_type and not is_desired_work_type(hash_result['type'], desired_work_type):
            print("\t\t\t\t! Unexpected/Undesired work type: {}".format(hash_result['type']))

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
        time.sleep(15)
        try:
            ws = get_socket()
            set_work_type = '{"work_type" : "%s", "address": "%s"}' % (desired_work_type, payout_address)
            ws.send(set_work_type)
            print("Connected")
        except:
            continue

