import sys, json, collections, getpass, re, os, time, math, msvcrt, ast

attributes = ['NodeID', 'Latitude', 'Longitude', 'Altitude', 
					'Airport', 'Region', 'Seaport',
					'InitialPopulation', 'BirthRate',
					'AgeDistributionFlag', 'AgeDistribution1', 'AgeDistribution2',
					'PrevalenceDistributionFlag', 'PrevalenceDistribution1', 'PrevalenceDistribution2',
					'ImmunityDistributionFlag', 'ImmunityDistribution1', 'ImmunityDistribution2',
					'RiskDistributionFlag', 'RiskDistribution1', 'RiskDistribution2',
					'MigrationHeterogeneityDistributionFlag', 'MigrationHeterogeneityDistribution1', 'MigrationHeterogeneityDistribution2' ]

hierarchy = [ 'NodeID',
					( 'NodeAttributes',
						[ 
							'Latitude', 'Longitude', 'Altitude',
							'Airport', 'Region', 'Seaport',
							'InitialPopulation', 'BirthRate'
						]
					),
					( 'IndividualAttributes',
						[
							'AgeDistributionFlag', 'AgeDistribution1', 'AgeDistribution2',
							'PrevalenceDistributionFlag', 'PrevalenceDistribution1', 'PrevalenceDistribution2',
							'ImmunityDistributionFlag', 'ImmunityDistribution1', 'ImmunityDistribution2',
							'RiskDistributionFlag', 'RiskDistribution1', 'RiskDistribution2',
							'MigrationHeterogeneityDistributionFlag', 'MigrationHeterogeneityDistribution1', 'MigrationHeterogeneityDistribution2'
						]
					)
				]

def ShowUsage():
	print ('\nUsage: %s [infile] [idreference] [optional-default-value-file]' % os.path.basename(sys.argv[0]))
	print ('\n    valid values for idreference: \n\t legacy (uses old NodeIDs) \n\t grump2.5arcmin \n\t grump30arcsec \n\t grump1degree')

def CheckFiles(infilename, outfilename):
	print('Source file:  %s' % infilename)
	print('Destination file: %s' % outfilename)

	if not os.path.exists(infilename):
		print('Source file doesn\'t exist!')
		return False
	
	if os.path.exists(outfilename):
		print('Destination file already exists!  Overwrite the existing file? (y/n) ', end='')
		sys.stdout.flush()

		answer = msvcrt.getche()
		print('')
		if answer != b'y' :
			return False

		try:
			os.remove(outfilename);
		except:
			print('Error while trying to delete file: %s' % sys.exc_info()[1])
			return False

	return True

def GetResolution(idreference):
	return {
		'legacy' : None,
		'grump2.5arcmin' : 0.04166667,
		'grump30arcsec' : 0.00833333,
		'grump1degree' : 1.0
	}.get(idreference, -1.0)
	
def BuildDataHierarchy(values, hierarchy, defaults):
	data = collections.OrderedDict([])

	for item in hierarchy:
		if type(item) is tuple:
			newdata = BuildDataHierarchy(values, item[1], defaults)
			if newdata is not None:
				data[item[0]] = newdata
		else:
			idx = attributes.index(item)
			if values[idx] != None and (defaults is None or defaults[idx] is None or defaults[idx] != values[idx]):
				data[item] = values[idx]
	
	if len(data) == 0:
		return None

	return data

def CalcNodeIDFromLatLon(lat, lon, resolution):
	xpix = math.floor((lon + 180.0) / resolution)
	ypix = math.floor((lat + 90.0) / resolution)
	return (xpix << 16) + ypix + 1  # NOTE: we want to reserve NodeID of 0 to be a null-value, so we want to start IDs at 1...

def ReadDefaultValues(filename):
	values = []
	
	if not os.path.exists(filename):
		print('Specified default value file doesn\'t exist')
		return None

	with open(filename, 'r') as file:
		for line in file:
			if line == "-\n":
				values.append(None)
			else:
				values.append(ast.literal_eval(line))

	if len(values) != len(attributes):
		print('Error parsing default attributes file; number of attributes found didn\'t match number expected')
		return None

	return values

	
if __name__ == "__main__":
	if len(sys.argv) != 3 and len(sys.argv) != 4:
		ShowUsage()
		exit(0)

	infilename = sys.argv[1]
	outfilename = re.sub('\.\w+$', '.json', infilename)

	if not CheckFiles(infilename, outfilename):
		exit(-1)
	
	resolution = GetResolution(sys.argv[2])
	
	if resolution is not None:
		if resolution < 0.0:
			print('Invalid idreference')
			ShowUsage()
			exit(-1)

		print('Resolution: %s' % resolution)
	
	defaults = None
	defaultsinfo = None
	
	if len(sys.argv) == 4:
		defaultvalfile = sys.argv[3]
		print('Default value file: %s' % defaultvalfile)

		defaults = ReadDefaultValues(defaultvalfile)
		if defaults is None:
			print('Unable to read default values... continue? (y/n) ', end='')
			sys.stdout.flush()

			answer = msvcrt.getche()
			print('')
			if answer != b'y' :
				exit(-1)
		else:
			defaultsinfo = BuildDataHierarchy(defaults, hierarchy, None)
			print("Defaults:")
			print(json.dumps(defaultsinfo, indent=5))

	nodelist = []

	with open(infilename, 'r') as file:
		while 1:
			line = file.readline() + file.readline()	# TODO: do we want to just readline() until we have len(attributes) values?
			if line == "":
				break

			values = str.split(line)

			if(len(values) != len(attributes)):
				print ('Error parsing old demographics file; number of attributes found (%i) didn\'t match number expected (%i)' % (len(values), len(attributes)))
				exit(-1)
			
			# TODO: convert values to ints/floats - is this the best way to do it? do we care about strings?
			values = list(map(ast.literal_eval, values))
			
			if resolution is not None:
				newnodeid = CalcNodeIDFromLatLon(values[attributes.index('Latitude')], values[attributes.index('Longitude')], resolution)
				values[attributes.index('NodeID')] = newnodeid

			nodedata = BuildDataHierarchy(values, hierarchy, defaults)
			
			nodelist.append(nodedata)
	
	idref = 'Legacy' if resolution is None else 'Gridded world %s' % sys.argv[2]
	metadata = collections.OrderedDict( [ ('DateCreated', time.asctime()), ('Tool', 'convertdemog.py'), ('Author', getpass.getuser()), ('IdReference', idref), ('NodeCount', len(nodelist)) ] )
	jsonlist = [ ('Metadata', metadata), ('Nodes', nodelist) ]

	if defaultsinfo is not None:
		jsonlist.insert(1, ('Defaults', defaultsinfo))

	fulljson = collections.OrderedDict(jsonlist)
	
	with open(outfilename, 'w') as file:
		json.dump(fulljson, file, indent=5)
