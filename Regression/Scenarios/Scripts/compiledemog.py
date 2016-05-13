import sys, os, json, collections, re, msvcrt

stringdict = collections.OrderedDict([])
nextstring = 'aa'

def ShowUsage():
	print ('\nUsage: %s <infile> [--forceoverwrite]' % os.path.basename(sys.argv[0]))

def CheckFiles(infilename, outfilename, forceoverwrite=False):
	print('Source file:  %s' % infilename)
	print('Destination file: %s' % outfilename)

	if not os.path.exists(infilename):
		print('Source file doesn\'t exist!')
		return False
	
	if os.path.exists(outfilename):
		if forceoverwrite:
			print('Destination file already exists!  Overwriting.' )
		else:
			print('Destination file already exists!  Overwrite the existing file? (y/n) ' )
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

def OrderedJsonLoad(in_dict):
	out_dict = collections.OrderedDict([])
	for pair in in_dict:
		out_dict[pair[0]] = pair[1]
	
	return out_dict

def CompressKeyStrings(datachunk):
	global nextstring
	newdatachunk = collections.OrderedDict([])

	for item in datachunk.items():
		if item[0] not in stringdict:
			stringdict[item[0]] = nextstring
			nextstring = GetNextString(nextstring)
		
		if type(item[1]) is collections.OrderedDict:
			newdatachunk[stringdict[item[0]]] = CompressKeyStrings(item[1])
		elif type(item[1]) is list:
			newdatachunk[stringdict[item[0]]] = list((CompressKeyStrings(x) if type(x) is collections.OrderedDict else x) for x in item[1])
		else:
			newdatachunk[stringdict[item[0]]] = item[1]
		
	return newdatachunk
	
def GetNextString(currentstring):
	for i in range(len(currentstring))[::-1]:
		if currentstring[i] != 'z':
			return currentstring[:i] + chr(ord(currentstring[i])+1) + currentstring[i+1:]

		currentstring = currentstring[:i] + ('a' * (len(currentstring) - i))

	print('ERROR! Ran out of string values for StringTable!')
	exit(-1)


if __name__ == "__main__":
	if len(sys.argv) != 2 and ( len(sys.argv) != 3 or sys.argv[2] != "--forceoverwrite" ):
		ShowUsage()
		exit(0)

	infilename = sys.argv[1]
	outfilename = re.sub('\.json$', '.compiled.json', infilename)

	if not CheckFiles(infilename, outfilename, len(sys.argv) == 3):
		exit(-1)

	with open(infilename, 'r') as file:
		fulljson = json.load(file, object_pairs_hook=OrderedJsonLoad) # TODO: should add error-messaging around this loading of the JSON

	compiledjson = collections.OrderedDict([])
	compiledjson['Metadata'] = fulljson['Metadata']
	
	newnodes = []
	offsets = []
	offset = 0

	for node in fulljson['Nodes']:
		newnode = CompressKeyStrings(node)
		newnodes.append(newnode)
		offsets.append(offset)
		offset += len(json.dumps(newnode, separators=(',',':'))) + 1

	compiledjson['StringTable'] = stringdict
	
	if 'Defaults' in fulljson:
		compiledjson['Defaults'] = CompressKeyStrings(fulljson['Defaults'])
		
	compiledjson['NodeOffsets'] = ''
	compiledjson['Nodes'] = []

	startoffset = len(json.dumps(compiledjson, separators=(',',':'))) - 2 + (16 * len(newnodes)) # remove the ']}' at the end of the file, and add padding for node-offsets
	offsets = [x + startoffset for x in offsets]
	
	offsetstr = ''	
	for node in newnodes:
		offsetstr = offsetstr + '%0.8X' % node[stringdict['NodeID']] + '%0.8X' % offsets.pop(0)

	compiledjson['NodeOffsets'] = offsetstr
	compiledjson['Nodes'] = newnodes
	
	compiledjsonstr = json.dumps(compiledjson, separators=(',',':'))
	
	with open(outfilename, 'w') as file:
		file.write(compiledjsonstr)
