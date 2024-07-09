# convert_txt_to_bin.py
# -----------------------------------------------------------------------------
# This script converts a CSV formated txt file to an EMOD binary-formatted migration file.
# It also creates the required metadata file.
#
# The CSV file has three columns
#    From_Node_ID, To_Node_ID, Rate (Average # of Trips Per Day)
# where the node ID's are the external ID's found in the demographics file.
# Each node ID in the migration file must exist in the demographics file.
# One can have node ID's in the demographics that don't exist in the migration file.
#
# The CSV file does not have to have the same number of entries for each From_Node.
# The script will find the From_Node that has the most and use that for the 
# DestinationsPerNode.  The binary file will have DestinationsPerNode entries
# per node.
# -----------------------------------------------------------------------------

import sys, os, json, collections, struct, datetime

MigrationType = []
MigrationType.append( "LOCAL_MIGRATION"    )
MigrationType.append( "AIR_MIGRATION"      )
MigrationType.append( "REGIONAL_MIGRATION" )
MigrationType.append( "SEA_MIGRATION"      )


def ShowUsage():
    print ('\nUsage: %s [input-migration-csv] [output-bin] [migration-type] [idreference]' % os.path.basename(sys.argv[0]))

if __name__ == "__main__":
    if len(sys.argv) != 5:
        ShowUsage()
        exit(0)

    filename   =  sys.argv[1]
    outfilename = sys.argv[2]
    mig_type    = sys.argv[3]
    id_ref      = sys.argv[4]

    mig_type_found = False
    for mig in MigrationType:
        mig_type_found |= (mig == mig_type)

    if( not mig_type_found ):
        print ("Invalid MigrationType = " + mig_type)
        exit(-1)

    max_destinations_per_node = 0
    destinations_per_node = 0

    fopen=open(filename)
    fout=open(outfilename,'wb')
    net={}
    net_rate={}

    # ----------------------------
    # collect data from CSV file
    # ----------------------------
    node_id_list = []
    prev_id = -1
    for line in fopen:
        s=line.strip().split(',')
        ID1=int(float(s[0]))
        ID2=int(float(s[1]))
        rate=float(s[2])
        if ID1 not in net:
            net[ID1]=[]
            net_rate[ID1]=[]
        net[ID1].append(ID2)
        net_rate[ID1].append(rate)
        if prev_id != ID1:
            if( destinations_per_node > max_destinations_per_node ):
                max_destinations_per_node = destinations_per_node
            node_id_list.append(ID1)
            print (prev_id, max_destinations_per_node)
            prev_id = ID1
            destinations_per_node = 0
        destinations_per_node += 1

    # ---------------
    # Write bin file
    # ---------------
    for ID in net:
        ID_write=[]
        ID_rate_write=[]
        for i in range(max_destinations_per_node):
            ID_write.append(0)
            ID_rate_write.append(0)
        for i in range(len(net[ID])):
            ID_write[i]=net[ID][i]
            ID_rate_write[i]=net_rate[ID][i]
        #The type needs to be 'I' because Linux handles 'L' differently than Windows.
        s_write=struct.pack('I'*len(ID_write), *ID_write)
        s_rate_write=struct.pack('d'*len(ID_rate_write),*ID_rate_write)
        fout.write(s_write)
        fout.write(s_rate_write)

    fopen.close()
    fout.close()

    # -------------------------------------------------------------------
    # Create NodeOffsets string
    # This contains the location of each From Node's data in the bin file
    # -------------------------------------------------------------------
    offset_str = ""
    nodecount = 0

    for ID in net:
        offset_str += '%0.8X' % ID
        offset_str += '%0.8X' % (nodecount * max_destinations_per_node * 12) # 12 -> sizeof(uint32_t) + sizeof(double) 
        nodecount += 1

    # -------------------
    # Write Metadata file
    # -------------------
    migjson = collections.OrderedDict([])
    migjson['Metadata'] = {}
    
    if os.name == "nt":
        migjson['Metadata']['Author'] = os.environ['USERNAME']
    else:
        migjson['Metadata']['Author'] = os.environ['USER']
        
    migjson['Metadata']['NodeCount'      ] = len(node_id_list)
    migjson['Metadata']['IdReference'    ] = id_ref
    migjson['Metadata']['DateCreated'    ] = datetime.datetime.now().ctime()
    migjson['Metadata']['Tool'           ] = os.path.basename(sys.argv[0])
    migjson['Metadata']['DatavalueCount' ] = max_destinations_per_node
    migjson['Metadata']['MigrationType'  ] = mig_type
    migjson['NodeOffsets'] = offset_str

    with open(outfilename+".json", 'w') as file:
        json.dump(migjson, file, indent=4)


