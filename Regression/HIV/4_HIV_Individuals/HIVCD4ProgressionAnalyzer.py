import matplotlib.pyplot as plt
import numpy as np
import os
import json
import math
import scipy.stats as sps

# TODO: The model has changed!
# TODO: Get these parameters from the config.json file instead of hard coding
#"CD4_At_Death_LogLogistic_Heterogeneity": 0.0, 
#"CD4_At_Death_LogLogistic_Scale": 31.63, 
#"CD4_Post_Infection_Weibull_Heterogeneity": 0.0, 
#"CD4_Post_Infection_Weibull_Scale": 540.55, 

# OLD
#LINEAR_CD4_DECLINE_MODEL_SLOPE = -508.92
#LINEAR_CD4_DECLINE_MODEL_INTERCEPT = 540.55

# NEW - should read from config
CD4_POST_INFECTION = 540.55
CD4_AT_DEATH = 31.63

class HIVCD4ProgressionAnalyzer():

    def __init__(self, plot=False, tol_fun=1e-6, verbose=False, output_dir='output'):

        self.filenames = [os.path.join(output_dir, 'ReportHIVInfection.csv')]

        self.plot = plot
        self.tol_fun = tol_fun
        self.verbose = verbose
        self.output_dir = output_dir
        self.data = {}  # observed relationship duration data by type
        self.fun = {}   # truth function by type
        self.results = {} # Place to store the boolean validity and other statistics of sub-tests

        self.map_count = 0

        # Here is the truth function
        #self.fun = lambda frac_prog: [LINEAR_CD4_DECLINE_MODEL_INTERCEPT + fp * LINEAR_CD4_DECLINE_MODEL_SLOPE for fp in frac_prog]
        self.fun = lambda frac_prog: [ (math.sqrt(CD4_POST_INFECTION) + fp * (math.sqrt(CD4_AT_DEATH) -math.sqrt(CD4_POST_INFECTION)))**2  for fp in frac_prog]

    def map(self, output_data):
        emit_data = {}

        hiv_infections = output_data[ self.filenames[0] ]    # CSV, has 'data' and 'colMap'
        rows = hiv_infections['data']
        colMap = hiv_infections['colMap']

        iid = [ int(float(r[colMap['Id']])) for r in rows]
        uids = set(iid) # <-- get unique ids

        for uid in uids:
            mine = [ ( \
                float(r[colMap['PrognosisCompletedFraction']]), \
                float(r[colMap['CD4count']])        # SQRT CD4 should be linear \
            )  for r in rows if r[colMap['Id']] == str(uid)]

            # Add map_count to the key so that individuals from separate simulations do not get reduced
            key = ( id(self), uid, self.map_count )
            tmp = zip(*mine)
            emit_data[key] = {'PrognosisCompletedFraction': tmp[0], 'CD4count': tmp[1]}

        self.map_count += 1

        return emit_data

    def reduce(self, parsers):

        # Accumulate grouped data over parsers
        for k,p in parsers.items():

            my_items = [ (key,val) for (key,val) in p.emit_data.items() if key[0] == id(self) ]

            for (key, val) in my_items:
                if key not in self.data:
                    self.data[key] = val
                else:
                    self.data[key] = self.data[key] + val

        for idx, key in enumerate( self.data.keys() ):
            mine = self.data[key]

            (x,y) = zip(* sorted( zip(mine['PrognosisCompletedFraction'], mine['CD4count']), key = lambda x: x[0] ) )

            err2 = sum([(yy - yy_hat)**2 for (yy,yy_hat) in zip(y,self.fun(x))])
            err = math.sqrt(err2)

            if( self.verbose ):
                print (err, self.tol_fun)

            # Store results here
            self.results[key] = {'Valid': err < self.tol_fun, 'Error': err, 'X': x, 'Y': y}

    def finalize(self):
        if self.plot:
            fig = plt.figure(self.__class__.__name__, figsize=(6, 4))

        for val in self.results.values():
            plt.plot(val['X'],val['Y'])

        if self.plot:
            x = np.arange(0,1,1e-3)
            y_true = self.fun( x )
            plt.plot( x, y_true, linestyle="dashed", linewidth=2)

        each_valid = [ self.results[x]['Valid'] for x in self.results]
        all_valid = all( each_valid )

        return all_valid
