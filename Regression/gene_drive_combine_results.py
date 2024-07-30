import json
import sys, os
import argparse
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

def getRunNumberFromFilename( filename ):
    fn_parts_1 = filename.split(".")
    fn_parts_2 = fn_parts_1[0].split("_")
    run_number = int(fn_parts_2[-1])
    return run_number
    

if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument('directory', help='Name of directory containing allele frequency files')
    args = parser.parse_args()

    input_filenames = []
    for file in os.listdir( args.directory ):
        if file.endswith(".csv"):
            input_filenames.append( file )
    
    column_names = []
    df = pd.DataFrame()
    for fn in input_filenames:
        run_number = getRunNumberFromFilename(fn)
        df2 = pd.read_csv( os.path.join( args.directory, fn  ) )
        column_names = df2.columns.values.tolist()
        df2["Run_Number"] = run_number
        df2.to_csv("tmp_rn.csv")
        df = pd.concat([df,df2])

    #df.to_csv("tmp.csv")
    
    column_names.remove("Time")
    pvt = df.pivot_table( index=["Time"],
                          values=column_names,
                          columns=["Run_Number"] )
    ax = pvt.plot()
    ax.set_yticks([0.05,0.10,0.15,0.2,0.25,0.30,0.35,0.40,0.45,0.50],minor=True)
    ax.grid(which='major', linestyle='-', linewidth='1.0')
    ax.grid(b=True, which='minor', linestyle=':', linewidth='0.5')
    
    pvt_average = df.pivot_table( index=["Time"],
                                  values=column_names,
                                  aggfunc=["mean","median"] )
    ax2 = pvt_average.plot()
    ax2.set_yticks([0.05,0.10,0.15,0.2,0.25,0.30,0.35,0.40,0.45,0.50],minor=True)
    ax2.grid(which='major', linestyle='-', linewidth='1.0')
    ax2.grid(b=True, which='minor', linestyle=':', linewidth='0.5')
    
    plt.show()
