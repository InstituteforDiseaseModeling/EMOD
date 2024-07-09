#! /usr/bin/env python3

import numpy as np


if __name__ == "__main__":
    #data =[
    #    [1,0,1,0,0,1],
    #    [0,1,0,1,1,0]
    #]
    #
    #np.save( "numpy-filesTest.npy", data )
    
    data = np.load( "tmp.npy" )
    print( data )