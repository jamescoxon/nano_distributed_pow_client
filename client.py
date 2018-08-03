import requests
import subprocess
import time

print("Welcome to Distributed Nano Proof of Work System")
address = input("Please enter your payout address: ")
print("All payouts will go to %s" % address)

while 1:
    r = requests.get('http://178.62.11.37/request_work')
    if r.text != "No Work":
        result = subprocess.check_output(["./mpow", r.text])

        work = result.decode().rstrip('\n\r')
        print(work)

        r = requests.post('http://178.62.11.37/return_work', data = {'hash':r.text, 'work':work, 'address':address})
        print(r.text)

    time.sleep(20)
