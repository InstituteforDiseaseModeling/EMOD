from struct import unpack
import getopt,sys,json

sourcedir = ""
filename = ""
outfile = ""
demographicsJson = ""

try:
   opts, args = getopt.getopt(sys.argv[1:],"",["migrationfile=","demographicsjson=","migrationtype=","outputfile="])
except getopt.GetoptError:
   print ''
   print 'usage:'
   print 'get_migration_from_bin_file.py --migrationfile= --demographicsjson= --migrationtype= --outputfile= '
   sys.exit(2)
for opt, arg in opts:
   if opt in '--migrationfile':
      migrationfile = arg
   elif opt in '--demographicsjson':
      demographicsJson = arg
   if opt in '--migrationtype':
      migrationtype = arg
   elif opt in '--outputfile':
       outputfile = arg

if migrationtype=='air':
    number_destinations = 60
elif migrationtype=='regional':
    number_destinations = 30
elif migrationtype=='local':
    number_destinations = 8

ID=[]
rate=[]

#read the demographics file
flat=json.loads(open(demographicsJson).read())

coordinate={}

IDlist = []

for i in xrange(len(flat["Nodes"])):
    vID=flat["Nodes"][i]["NodeID"]
    vlat=flat["Nodes"][i]["NodeAttributes"]["Latitude"]
    vlon=flat["Nodes"][i]["NodeAttributes"]["Longitude"]
    coordinate[vID]=(vlat,vlon)
    IDlist.append(vID)

#read the binary migration file
fopen=open(migrationfile,"rb")
byte="a"
count = 0
while byte!="":
    for i in xrange(number_destinations):
        byte=fopen.read(4)
        if byte!="":
            ID.append(int(unpack('L',byte)[0]))
    for i in xrange(number_destinations):
        byte=fopen.read(8)
        if byte!="":
            rate.append(float(unpack('d',byte)[0]))

fout=open(outputfile,'w')
IDseq=0
count=0
for i in xrange(len(ID)):
    if ID[i]!=0:
       print >>fout, IDlist[IDseq], ID[i], rate[i]
    count+=1
    if count % number_destinations == 0:
       IDseq+=1
