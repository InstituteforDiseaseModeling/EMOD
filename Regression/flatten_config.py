#!/user/bin/python

import sys
import regression_utils as idm

if __name__ == '__main__':
    if len(sys.argv) > 1:
        idm.flattenConfig(sys.argv[1])
    else:
        print( 'usage:', sys.argv[0], 'configFile')
