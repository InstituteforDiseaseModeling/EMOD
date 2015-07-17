import sys, os, json, collections, re, msvcrt

def ShowUsage():
	print ('\nUsage: %s [compiled-demographics-file] [num-years] [climate-update-resolution]' % os.path.basename(sys.argv[0]))

def CheckFiles(infilename, outfilename):
	print('Source file:  %s' % infilename)
	print('Destination file: %s' % outfilename)

	if not os.path.exists(infilename):
		print('Source file doesn\'t exist!')
		return False
	
	if os.path.exists(outfilename):
		fileExist = raw_input ('Destination file already exists!  Overwrite the existing file? (y/n) [n] ')

		if fileExist.upper() != 'Y' :
			return False

		try:
			os.remove(outfilename);
		except:
			print('Error while trying to delete file: %s' % sys.exc_info()[1])
			return False

	return True

def OrderedJsonLoad(in_dict):
	out_dict = collections.OrderedDict([])
	for pair in in_dict:
		out_dict[pair[0]] = pair[1]
	
	return out_dict

def GetValueCountPerYear(updateres):
	return {
		'CLIMATE_UPDATE_YEAR' : 1,
		'CLIMATE_UPDATE_MONTH' : 12,
		'CLIMATE_UPDATE_WEEK' : 52,
		'CLIMATE_UPDATE_DAY' : 365,
		'CLIMATE_UPDATE_HOUR' : 8760,
	}.get(updateres, -1.0)


if __name__ == "__main__":
	if len(sys.argv) != 4:
		ShowUsage()
		exit(0)
  
	infilename = sys.argv[1]
	outfilename = os.path.join(os.path.dirname(infilename), "climateheader.json")
	numyears = sys.argv[2]
	climupdateres = sys.argv[3]
	valcountperyear = GetValueCountPerYear(climupdateres)
	numvals = valcountperyear * int(numyears)
	
	if valcountperyear <= 0:
		print('Invalid climate_update_resolution string: %s' % climupdateres)
		exit(-1)

	if not CheckFiles(infilename, outfilename):
		exit(-1)

	with open(infilename, 'r') as file:
		demogjson = json.load(file, object_pairs_hook=OrderedJsonLoad) # TODO: should add error-messaging around this loading of the JSON

	climatejson = collections.OrderedDict([])
	climatejson['Metadata'] = demogjson['Metadata']
	climatejson['Metadata']['Tool'] = os.path.basename(sys.argv[0])
	climatejson['Metadata']['DatavalueCount'] = numvals
	climatejson['Metadata']['UpdateResolution'] = climupdateres
	
	demogoffsets = demogjson['NodeOffsets']
	nodecount = int(demogjson['Metadata']['NodeCount'])
	
	climoffsets = ''
	for i in range(nodecount):
		climoffsets += demogoffsets[i*16: i*16 + 8] + '%0.8X' % (i * numvals * 4) # 4 -> sizeof(float)

	climatejson['NodeOffsets'] = climoffsets
	
	# climatejsonstr = json.dumps(climatejson)
	
	# with open(outfilename, 'w') as file:
		# file.write(climatejsonstr)
		
	with open(outfilename, 'w') as file:
		json.dump(climatejson, file, indent=5)
