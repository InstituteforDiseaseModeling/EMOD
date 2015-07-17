import sys, os, json, collections, re, msvcrt, shutil, struct, array

def ShowUsage():
	print ('\nUsage: %s [input-koppen-file]' % os.path.basename(sys.argv[0]))

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

	
if __name__ == "__main__":
	if len(sys.argv) != 2:
		ShowUsage()
		exit(0)

	newfilename = sys.argv[1]
	oldfilename = '%s.old' % newfilename
	
	shutil.copyfile(newfilename, oldfilename)
	
	if not CheckFiles(oldfilename, newfilename):
		exit(-1)

	koppen_vals = array.array('i')
	
	with open(oldfilename, 'rb') as oldfile:
		byte_str = oldfile.read()

	while len(byte_str) != 0:
		currnodedata, byte_str = byte_str[4:8], byte_str[8:]
		koppen_vals.append(struct.unpack("i", currnodedata)[0])

	with open(newfilename, 'wb') as newfile:
		koppen_vals.tofile(newfile)
