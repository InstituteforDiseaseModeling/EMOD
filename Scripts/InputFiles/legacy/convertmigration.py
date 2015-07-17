import sys, os, json, collections, re, msvcrt, shutil, array

def ShowUsage():
	print ('\nUsage: %s [raw-demographics-file] [migration-file] [migration-type]' % os.path.basename(sys.argv[0]))

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

def GetMaxValueCountForMigrationType(mig_type):
	return {
		'sea' : 5,
		'local' : 8,
		'regional' : 30,
		'air' : 60
	}.get(mig_type, -1.0)


if __name__ == "__main__":
	if len(sys.argv) != 4:
		ShowUsage()
		exit(0)

	infilename = sys.argv[1]
	outfilename = sys.argv[2]
	mig_type = sys.argv[3]
	maxvalcount = GetMaxValueCountForMigrationType(mig_type)
	
	if maxvalcount <= 0:
		print('Invalid migration-type string: %s' % mig_type)
		exit(-1)

	oldmigfilename = '%s.old' % outfilename
	shutil.copyfile(outfilename, oldmigfilename)
	
	if not CheckFiles(infilename, outfilename):
		exit(-1)

	with open(infilename, 'r') as file:
		demogjson = json.load(file, object_pairs_hook=OrderedJsonLoad) # TODO: should add error-messaging around this loading of the JSON

	nodeidmap = [ 0 ]

	for node in demogjson['Nodes']:
		nodeidmap.append(node['NodeID'])

	with open(outfilename, 'wb') as newfile:
		if mig_type != "sea":
			with open(oldmigfilename, 'rb') as oldfile:
				for i in range(len(nodeidmap)-1):
					oldnodeids = array.array('I')
					oldnodeids.fromfile(oldfile, maxvalcount)
					oldrates = array.array('d')
					oldrates.fromfile(oldfile, maxvalcount)

					newnodeidlist = oldnodeids.tolist()
					newnodeids = array.array('I')
					newnodeids.fromlist([nodeidmap[x] for x in newnodeidlist])

					newnodeids.tofile(newfile)
					oldrates.tofile(newfile)
		else:
			with open(oldmigfilename, 'r') as oldfile:
				for line in oldfile.readlines():
					vals = str.split(line, '\t')
					
					srcnodeid = vals.pop(0)
					destnodeids = []
					destnodeid = int(vals.pop(0))
					destnoderates = []
					
					while destnodeid != 0:
						destnodeids.append(destnodeid)
						destnoderates.append(float(vals.pop(0)))
						destnodeid = int(vals.pop(0))
					
					while len(destnodeids) != maxvalcount:
						destnodeids.append(0)
						destnoderates.append(0.0)

					newnodeids = array.array('I')
					newnodeids.fromlist([nodeidmap[x] for x in destnodeids])
					newnoderates = array.array('d')
					newnoderates.fromlist(destnoderates)

					newnodeids.tofile(newfile)
					newnoderates.tofile(newfile)
