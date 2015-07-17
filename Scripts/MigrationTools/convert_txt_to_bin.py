from struct import pack
from sys import argv

filename=argv[1]
outfilename=argv[2]

fopen=open(filename)
fout=open(outfilename,'wb')
net={}
net_rate={}

for line in fopen:
    s=line.strip().split()
    ID1=int(float(s[0]))
    ID2=int(float(s[1]))
    rate=float(s[2])
    if ID1 not in net:
        net[ID1]=[]
        net_rate[ID1]=[]
    net[ID1].append(ID2)
    net_rate[ID1].append(rate)

for ID in net:
    ID_write=[]
    ID_rate_write=[]
    for i in xrange(8):
        ID_write.append(0)
        ID_rate_write.append(0)
    for i in xrange(len(net[ID])):
        ID_write[i]=net[ID][i]
        ID_rate_write[i]=net_rate[ID][i]
    s_write=pack('L'*len(ID_write), *ID_write)
    s_rate_write=pack('d'*len(ID_rate_write),*ID_rate_write)
    fout.write(s_write)
    fout.write(s_rate_write)

fopen.close()
fout.close()
    
