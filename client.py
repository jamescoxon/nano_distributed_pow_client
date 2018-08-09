import requests
import subprocess
import time, sys

print("Welcome to Distributed Nano Proof of Work System")
address = ''
try:
    with open('address.txt') as f:
        address = f.readline().strip()
except Exception as e:
    pass

if(len(address) == 0):
    address = input("Please enter your payout address: ")
print("All payouts will go to %s" % address)
pow_source = int(input("Select PoW Source, 0 = local, 1 = node: "))
if pow_source > 1:
    print("Incorrect Entry, Exiting")
    sys.exit()

print("Waiting for work...")

while 1:
  try:
    r = requests.get('http://178.62.11.37/request_work')
    print("Got work")
    hash_result = r.json()
    if hash_result['hash'] != "error":
        if pow_source == 0:
            try:
                result = subprocess.check_output(["./mpow", hash_result['hash']])
                work = result.decode().rstrip('\n\r')
                print(work)

            except KeyboardInterrupt:
                print("\nCtrl-C detected, canceled by user")
                break;
            except:
                print("Error - no mpow binary")
                sys.exit()

        elif pow_source == 1:
            try:
                rai_node_address = 'http://%s:%s' % ('127.0.0.1', '7076')
                get_work = '{ "action" : "work_generate", "hash" : "%s", "use_peers": "true" }' % hash_result['hash']
                r = requests.post(rai_node_address, data = get_work)
                resulting_work = r.json()
                work = resulting_work['work'].lower()
            except:
                print("Error - failed to connect to node")
                sys.exit()
        json_request = '{"hash" : "%s", "work" : "%s", "address" : "%s"}' % (hash_result['hash'], work, address)
        r = requests.post('http://178.62.11.37/return_work', data = json_request)
        print(r.text)

  except KeyboardInterrupt:
      print("\nCtrl-C detected, canceled by user")
  except Exception as e:
      print("Error: {}".format(e))

  time.sleep(5)
