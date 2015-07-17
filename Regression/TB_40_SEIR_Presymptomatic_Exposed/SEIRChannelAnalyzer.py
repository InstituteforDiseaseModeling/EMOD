import matplotlib.pyplot as plt
import numpy as np
import os
import json
import math


class SEIRChannelAnalyzer():

    def __init__(self, plot=False, alpha=0.01, verbose=False, output_dir='output'):
        self.plot = plot
        self.alpha = alpha
        self.verbose = verbose
        self.output_dir = output_dir
        self.filenames = ['config.json', os.path.join( self.output_dir, 'InsetChart.json')]
        self.data = {}  # observed circumcision
        self.fun = {}   # truth function
        self.results = {} # Place to store the boolean validity and other statistics of sub-tests

    def ztest(self, x, n, p, alpha):

        if n*p < 10 or n*(1-p) < 10:
            print "WARNING: not enough samples to use a Z-Test, marking as Fail!"
            return {'Valid': False, 'Test_Statistic': None, 'P_Value': None}

        p_hat = x / float(n)
        std = math.sqrt( p*(1-p)/float(n) )
        z_score = (p_hat - p) / std
        p_val = 2 * sps.norm.cdf(-abs(z_score))

        if self.verbose:
            print p_val

        if p_val < alpha:
            return {'Valid': False, 'Test_Statistic': z_score, 'P_Value': p_val}

        return {'Valid': True, 'Test_Statistic': z_score, 'P_Value': p_val}


    def map(self, output_data):
        emit_data = {}

        config_json = output_data[ self.filenames[0] ]  # config.json has 'TB_Base_Infectivity_Presymptomatic' parameter
        inset_chart_json = output_data[ self.filenames[1] ] # InsetChart.json has SEIRW and TB channel data

        mapped_data = {}

        mapped_data['TB_Base_Infectivity_Presymptomatic'] = config_json['parameters']['TB_Base_Infectivity_Presymptomatic']

        mapped_data['Susceptible'] = inset_chart_json['Channels']['Susceptible Population']['Data']
        mapped_data['Exposed'] = inset_chart_json['Channels']['Exposed Population']['Data']
        mapped_data['Infectious'] = inset_chart_json['Channels']['Infectious Population']['Data']
        mapped_data['Recovered'] = inset_chart_json['Channels']['Recovered Population']['Data']
        mapped_data['Waning'] = inset_chart_json['Channels']['Waning Population']['Data']

        mapped_data['Latent'] = inset_chart_json['Channels']['Latent TB Prevalence']['Data']
        mapped_data['Presymptomatic'] = inset_chart_json['Channels']['Active Presymptomatic Prevalence']['Data']
        mapped_data['SmearPositive'] = inset_chart_json['Channels']['Active Sx Smear pos Prevalence']['Data']
        mapped_data['SmearNegative'] = inset_chart_json['Channels']['Active Sx Smear neg Prevalence']['Data']
        mapped_data['ExtraPulmonary'] = inset_chart_json['Channels']['Active Sx Extrapulm Prevalence']['Data']

        key = id(self)
        emit_data[key] = mapped_data

        return emit_data


    def validate_seirw(self, extracted_data):
        susceptible = np.array(extracted_data['Susceptible'])
        exposed = np.array(extracted_data['Exposed'])
        infectious = np.array(extracted_data['Infectious'])
        recovered = np.array(extracted_data['Recovered'])
        waning = np.array(extracted_data['Waning'])

        accumulator = np.array(susceptible)
        accumulator += exposed
        accumulator += infectious
        accumulator += recovered
        accumulator += waning

        ones = np.ones(len(accumulator))
        diff = ones - accumulator

        def test_validity(previous, value):
            current = (value < self.alpha)
            return (previous and current)

        valid = reduce(test_validity, diff, True)   # note, this is _not_ self.reduce

        return valid


    def validate_exposed(self, extracted_data, exposed_channels):
        exposed = np.array(extracted_data['Exposed'])
        accumulator = np.zeros(len(exposed))
        for channel in exposed_channels:
            accumulator += np.array(extracted_data[channel])

        diff = exposed - accumulator

        def test_diff(previous, values):
            (expected, difference) = values
            difference = abs(difference)
            current = (difference < (expected * self.alpha)) if (expected > 0.0) else (difference < self.alpha)
            if not current and self.verbose:
                print 'validate_exposed: {0}, {1} failed'.format(expected, difference)
            return (previous and current)

        valid = reduce(test_diff, zip(exposed, diff), True) # note, this is _not_ self.reduce

        return valid


    def validate_infectious(self, extracted_data, infectious_channels):
        infectious = np.array(extracted_data['Infectious'])
        accumulator = np.zeros(len(infectious))
        for channel in infectious_channels:
            accumulator += np.array(extracted_data[channel])

        diff = infectious - accumulator

        def test_diff(previous, values):
            (expected, difference) = values
            difference = abs(difference)
            current = (difference < (expected * self.alpha)) if (expected > 0.0) else (difference < self.alpha)
            if not current and self.verbose:
                print 'validate_infectious: {0}, {1} failed'.format(expected, difference)
            return (previous and current)

        valid = reduce(test_diff, zip(infectious, diff), True)  # note, this is _not_ self.reduce

        return valid


    def reduce(self, parsers):
        # Accumulate grouped data over parsers
        for k,p in parsers.items():
            my_items = [ val for (key,val) in p.emit_data.items() if key == id(self) ]

        extracted_data = my_items[0]
        presymptomatic_infectivity = extracted_data['TB_Base_Infectivity_Presymptomatic']

        self.results['seirw'] = self.validate_seirw(extracted_data)

        exposed_channels = [ 'Latent' ]
        if presymptomatic_infectivity == 0.0:
           exposed_channels.append( 'Presymptomatic' )
        self.results['exposed'] = self.validate_exposed(extracted_data, exposed_channels)

        infectious_channels = [ 'SmearPositive', 'SmearNegative', 'ExtraPulmonary' ]
        if presymptomatic_infectivity > 0.0:
           infectious_channels.append( 'Presymptomatic' )
        self.results['infectious'] = self.validate_infectious(extracted_data, infectious_channels)

        pass

    def finalize(self):
        test_results = [ self.results[test] for test in self.results ]

        if self.verbose:
            print test_results

        all_valid = all( test_results )

        return all_valid
