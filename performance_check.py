import client
import argparse
from timeit import timeit
from sys import exit

HASH = 'C876D20ECA5D4F8C7BD06B55FEFEABB46117A6BB3825ED95BEFC33B5CCB3316D'

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
        client.get_work_node(HASH)
    except Exception as e:
        print('Error getting work from the node, wont proceed: {}'.format(e))
        exit(1)

    try:
        client.get_work_lib(HASH)
    except Exception as e:
        print('Error getting work from the library, wont proceed: {}'.format(e))
        exit(1)

    node_wrapped = wrapper(client.get_work_node, HASH)
    lib_wrapped = wrapper(client.get_work_lib, HASH)

    print('Computing {} works using the node.'.format(args.works))
    node_time = timeit(node_wrapped, number=args.works)

    print('Computing {} works using the library'.format(args.works))
    lib_time = timeit(lib_wrapped, number=args.works)

    print('\nRESULTS\n\tNode: {}\n\tLibrary: {}'.format(node_time, lib_time))
