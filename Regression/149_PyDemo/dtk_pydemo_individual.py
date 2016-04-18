#!/usr/bin/python

import pdb
import math
import csv
import os
import sys
import json
import random

print( "\nDTK_PythonFever_INDIVIDUAL\n" )
population = {}
# reporting variables
tracking_sample = [] # holds list of ids of people we decide to track and report on
tracking_sample_rate = 0 # sampling rate for tracking/reporting: 0.1 means track 10% of population
tracking_report = {} # the report itself: a map of ids to lists of states, 1 per timestep
"""
Our IndividualPythonFever class goes here. It should match up to the C++ functions that it mirrors
"""

random.seed(20)

class PyIndividual:

    _incubation_period = 14
    _prob_infection = 0.0001
    _maleInfectiousnessByAge = { 0 : 1, 5: 0.8, 10: 0.3, 15: 0.5, 20: 0.1, 50: 1, 125: 0.05 }
    _femaleInfectiousnessByAge = { 0 : 1, 5: 0.8, 10: 0.3, 15: 0.5, 20: 0.1, 50: 1, 125: 0.05 }
    
    def __init__(self, new_id_in, new_mcw_in, new_age_in, new_sex_in ):
        self._id = new_id_in
        self._mcw = new_mcw_in
        self._age = new_age_in
        self._sex = new_sex_in
        self.infectious_timer = -1

    def __del__(self):
        pass

    def ageInYearsAsInt(self):
        return int((round(self._age/365)))

    def AcquireInfection(self):
        self.infectious_timer = self._incubation_period

    def Update( self, dt ):
        """
        Update timers, including age. Return state.
        """
        state_change = False
        state = "S"
        self._age += dt
        if self.infectious_timer > -1:
            self.infectious_timer = self.infectious_timer - dt
            state = "I"
            if self.infectious_timer + dt == self._incubation_period:
                state_change = True # S->I
            if self.infectious_timer <= 0:
                state = "S"
                state_change = True # I->S
                #print( "Someone just cleared their infection: returning {0}, {1}.".format( state, state_change ) )

        return state, state_change

    def Expose( self, contagion_population, dt, route ):
        infected = 0
        draw = random.random()
        if draw < self._prob_infection:
            infected = 1
        return infected

    def getInfectiousnessByAgeAndSex( self ):
        retValue = 0
        table = None
        if self._sex == 'MALE':
            table = self._maleInfectiousnessByAge
        else:
            table = self._femaleInfectiousnessByAge
        ageInYrs = self.ageInYearsAsInt()
        for ageBoundary in table:
            if ageInYrs <= ageBoundary:
                return table[ ageBoundary ];

    def GetInfectiousnessByRoute( self, route ):
        # NOTE: route is contact or environmental
        deposit = 0
        if self.infectious_timer > 0:
            deposit = self.getInfectiousnessByAgeAndSex()
        return deposit

class PythonFeverIndividual(PyIndividual):
    def __init__(self, new_id_in, new_mcw_in, new_age_in, new_sex_in ):
        PyIndividual.__init__( self, new_id_in, new_mcw_in, new_age_in, new_sex_in )

"""
This is the 'shim' layer between the C++ and Python code. It connects by individual id.
"""
def create( new_id, new_mcw, new_age, new_sex ):
    #print( "py: creating new individual: " + str(new_id) )
    population[new_id] = PythonFeverIndividual( new_id, new_mcw, new_age, new_sex )
    #population[new_id] = PyIndividual( new_id, new_mcw, new_age, new_sex )

def destroy( dead_id ):
    del population[ dead_id ]

def update( update_id, dt ):
    #pdb.set_trace()
    #print( "py: updating individual: " + str(update_id) )
    return population[update_id].Update( dt )

def update_and_return_infectiousness( update_id, route ):
    #pdb.set_trace()
    #print( "py: getting individual infectiousness by route for individual " + str(update_id) + " and route " + route )
    return population[ update_id ].GetInfectiousnessByRoute( route )

def acquire_infection( update_id ):
    #pdb.set_trace()
    #print( "py: acquire_infection: " + str(update_id) )
    population[ update_id ].AcquireInfection( )

def expose( update_id, contagion_population, dt, route ):
    #pdb.set_trace()
    #print( "py: expose: " + str( update_id ) )
    return population[ update_id ].Expose( contagion_population, dt, route )

def start_timestep():
    for i in population:
        population[ i ]._age = population[ i ]._age + 1

# Show how to read config.json params directly into python layer
if os.path.exists( "config.json" ) == False:
    print( "Failed to find or open config.json. Exiting." )
    sys.exit()
config_json = json.loads( open( "config.json" ).read() )["parameters"]

#for param in ["PythonFever_Acute_Infectivity","PythonFever_Prepatent_Infectivity","PythonFever_Subclinical_Infectivity","PythonFever_Chronic_Infectivity"]:
#    if param not in config_json:
#        print( "Failed to find key (parameter) " + param + " in config.json." )
#        sys.exit()

#PythonFeverIndividual._acute_infectivity = config_json["PythonFever_Acute_Infectivity"]
