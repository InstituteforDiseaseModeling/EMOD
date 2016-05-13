import sys, os, json, collections, re, msvcrt, shutil, array, struct

def ShowUsage():
	print ('\nUsage: %s [raw-demographics-file] [loadbalancing-file]' % os.path.basename(sys.argv[0]))

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

def OrderedJsonLoad(in_dict):
	out_dict = collections.OrderedDict([])
	for pair in in_dict:
		out_dict[pair[0]] = pair[1]
	
	return out_dict


if __name__ == "__main__":
	if len(sys.argv) != 3:
		ShowUsage()
		exit(0)

	demogfilename = sys.argv[1]
	lbfilename = sys.argv[2]
	
	oldlbfilename = '%s.old' % lbfilename
	shutil.copyfile(lbfilename, oldlbfilename)
	
	if not CheckFiles(demogfilename, lbfilename):
		exit(-1)

	with open(demogfilename, 'r') as file:
		demogjson = json.load(file, object_pairs_hook=OrderedJsonLoad) # TODO: should add error-messaging around this loading of the JSON

	newnodeids = [ ]

	for node in demogjson['Nodes']:
		newnodeids.append(node['NodeID'])

	with open(oldlbfilename, 'rb') as oldfile:
		nodecountbytes = oldfile.read(4)
		
		nodecount = struct.unpack('<I', nodecountbytes)[0]
		if nodecount != len(newnodeids):
			print("node count in load-balancing file doesn't match node count in demographics file!")
			exit(-1)

		with open(lbfilename, 'wb') as newfile:
			newfile.write(nodecountbytes)
		
			newnodeidarray = array.array('I')
			newnodeidarray.fromlist(newnodeids)
			newnodeidarray.tofile(newfile)
			
			junk = oldfile.read(4*nodecount) # don't care about the "balance rates"... they've never been used
			newfile.write(oldfile.read(4*nodecount)) # copy the "balance scale"

