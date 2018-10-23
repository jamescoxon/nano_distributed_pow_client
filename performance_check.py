import argparse
from timeit import timeit
from sys import exit
import requests, json
import mpow

HASH = 'C876D20ECA5D4F8C7BD06B55FEFEABB46117A6BB3825ED95BEFC33B5CCB3316D'

WORK_SERVER = 'http://%s:%s' % ('127.0.0.1', '7070')
NANO_THRESHOLD = 'ffffffc000000000'

def get_work_lib(from_hash, threshold):
    work = format(mpow.generate(bytes.fromhex(from_hash), int(threshold, 16)), '016x')
    return work

def get_work_node(from_hash, threshold):
    get_work = '{ "action" : "work_generate", "hash" : "%s", "threshold": "%s", "use_peers": "true" }' % ( from_hash, threshold )
    r = requests.post(WORK_SERVER, data = get_work)
    resulting_work = r.json()
    work = resulting_work['work'].lower()
    return work

def wrapper(func, *args, **kwargs):
    def wrapped():
        return func(*args, **kwargs)
    return wrapped

if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument('works', type=int,
                        help='Number of works to compute for each evaluation')
    args = parser.parse_args()

    # try one of each, to check if they are available
    try:
        get_work_node(HASH, NANO_THRESHOLD)
    except Exception as e:
        print('Error getting work from the node, wont proceed: {}'.format(e))
        exit(1)

    try:
        get_work_lib(HASH, NANO_THRESHOLD)
    except Exception as e:
        print('Error getting work from the library, wont proceed: {}'.format(e))
        exit(1)

    node_wrapped = wrapper(get_work_node, HASH, NANO_THRESHOLD)
    lib_wrapped = wrapper(get_work_lib, HASH, NANO_THRESHOLD)

    print('Computing {} works using the node.'.format(args.works))
    node_time = timeit(node_wrapped, number=args.works)

    print('Computing {} works using the library'.format(args.works))
    lib_time = timeit(lib_wrapped, number=args.works)

    print('\nRESULTS\n\tNode: {}\n\tLibrary: {}'.format(node_time, lib_time))
