import sys
import re
import commands
from urllib import urlencode
import requests

# Microbench storage export tool for nosqlbench
# requirements: requests
# Usage:
# 1. Get auth token in microbench service
# 2. Create auth.conf file contains: [uri]:[token]
# 3. Run nosqlbench
# 5. export:
# python export.py [auth.conf] [benchmark output file] [metric name] [metric group]
#
# results example: http://bench.farm.tarantool.org

def credentals(filename):
    conf = open(sys.argv[1], 'r')
    auth_data = conf.readline()[:-1]
    conf.close()
    return auth_data.split(':')

def parse_bench(filename):
    vag_val = None
    for line in open(sys.argv[2], 'r'):
        if re.search('\| req/s', line):
            raw = line.split('|')
            avg_val = raw[3].strip()
            break
    return avg_val

def push(server, token, name, value, version, unit='rps', tab='nosqlbench'):
    uri = 'http://%s/push?%s' % (server, urlencode(dict(
        key=token, name=name, param=value,
        v=version, unit=unit, tab=tab
    )))
    print 'Exporting result into microb storage:'
    r = requests.get(uri)
    if r.status_code == 200:
        print 'Export complete'
    else:
        print 'Export error http: %d' % r.status_code

def get_version():
    ret = commands.getoutput('tarantool --version')
    return ret.split('\n')[0].split(' ')[1]

def main():
    if len(sys.argv) < 4:
        print 'Usage:\n./export.py [auth.conf] [output.file] [bench_name] [tab]'
        return 0

    server, token = credentals(sys.argv[1])
    value = parse_bench(sys.argv[2])
    name = sys.argv[3]
    version = get_version()
    tab = sys.argv[4]

    # push bench data to result server
    push(server, token, name, value, version, tab=tab)
    return 0

if __name__ == '__main__':
    main()
