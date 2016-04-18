import sys, os, json, collections

MAX_LOCAL_NODES    = 8
MAX_REGIONAL_NODES = 30
MAX_WORK_NODES     = 5   # Work = Sea

VECTOR_LOCAL_DAYS_BETWEEN_TRIPS     = 3.0   # num-adj-nodes / local-days
VECTOR_REGIONAL_DAYS_BETWEEN_TRIPS  = 100.0   # num-non-adj-nodes / regional-days

LOCAL_DAYS_BETWEEN_TRIPS     =  20.0   # num-adj-nodes / local-days
REGIONAL_DAYS_BETWEEN_TRIPS  =  40.0   # num-non-adj-nodes / regional-days
WORK_NODE_DAYS_BETWEEN_TRIPS = 100.0   # 1 / work-days

human_village_list = [[1,2,3,4,11,12,13,14,21,22,23,24,31,32,33,34],[7,8,9,10,17,18,19,20,27,28,29,30,37,38,39,40,47,48,49,50,57,58,59,60],[61,62,63,64,65,71,72,73,74,75,76,81,82,83,84,85,86,91,92,93,94,95,96]]
vector_village_list = [[1,2,3,4,5,11,12,13,14,15,21,22,23,24,25,31,32,33,34,35,45],[6,7,8,9,10,16,17,18,19,20,26,27,28,29,30,36,37,38,39,40,46,47,48,49,50,56,57,58,59,60,67,68,69,70],[61,62,63,64,65,66,71,72,73,74,75,76,77,81,82,83,84,85,86,87,91,92,93,94,95,96,97]]

def ShowUsage():
    print ('\nUsage: %s [DemogPrefix] [Node_Grid_Size_Deg] [WorkNode] [IsVectorMigration:0 or 1]' % os.path.basename(sys.argv[0]))

def IsInVillage( node_id, other_node_id, village_list ):
    for village in village_list:
        for id in village:
            if( id == node_id):
                village2 = village
                for oid in village2:
                    if( oid == other_node_id):
                        return True
    return False


def FindAdjacentNodes( node, demogjson, grid_size, work_node_id, is_vectors, adjacent_node_id_list, regional_node_id_list ):

    node_id = int(node['NodeID'])
    node_lon = float(node['NodeAttributes']['Longitude'])
    node_lat = float(node['NodeAttributes']['Latitude' ])

    min_lon = node_lon - grid_size
    max_lon = node_lon + grid_size
    min_lat = node_lat - grid_size
    max_lat = node_lat + grid_size

    #print '{0:e}'.format(min_lon) + "|" + '{0:e}'.format(max_lon) + " : " + '{0:e}'.format(min_lat) + "|" + '{0:e}'.format(max_lat)

    for other_node in demogjson['Nodes']:
        other_node_id  = int(other_node['NodeID'])
        other_node_lon = float(other_node['NodeAttributes']['Longitude'])
        other_node_lat = float(other_node['NodeAttributes']['Latitude' ])

        if( node['NodeID'] == other_node['NodeID'] ):
            continue
        if( int(node['NodeID']) == work_node_id ):
            continue
        if( is_vectors ):
            if( int(other_node['NodeAttributes']['InitialVectors']) == 0 ):
                continue
            if( int(node['NodeAttributes']['InitialVectors']) == 0 ):
                continue
        else:
            if( int(other_node['NodeAttributes']['InitialPopulation']) == 0 ):
                continue
            if( int(node['NodeAttributes']['InitialPopulation']) == 0 ):
                continue

        # --------------------------------------------------------------------------------------
        # --- When doing the equality operator, we compare strings so that it includes rounding.
        # --- Without it, things that looked equal were not
        # --------------------------------------------------------------------------------------
        village_list = human_village_list
        if( is_vectors ):
            village_list = vector_village_list

        if( IsInVillage( node_id, other_node_id, village_list ) ):
            # is on left side?
            if( (str(other_node_lon) == str(min_lon)) and (min_lat <= other_node_lat <= max_lat) ):
                adjacent_node_id_list.append( other_node_id )

            # is on top
            elif( (str(other_node_lat) == str(max_lat)) and (min_lon <= other_node_lon <= max_lon) ):
                adjacent_node_id_list.append( other_node_id )

            #is on right
            elif( (str(other_node_lon) == str(max_lon)) and (min_lat <= other_node_lat <= max_lat) ):
                adjacent_node_id_list.append( other_node_id )

            # is on bottom
            elif( (str(other_node_lat) == str(min_lat)) and (min_lon <= other_node_lon <= max_lon) ):
                adjacent_node_id_list.append( other_node_id )
        
            #not adjacent and not work so regional
            elif( (work_node_id != other_node_id) and (len(regional_node_id_list) < MAX_REGIONAL_NODES) ):
                regional_node_id_list.append( other_node_id )

def WriteToMigration( local_file, node, max_nodes, days_between_trips, per_node_factor, node_id_list ):
    rate = 0
    if( len( node_id_list ) > 0 ):
        rate = 1.0 / per_node_factor / days_between_trips

    for node_id in node_id_list:
        line = str(node['NodeID']) + "," + str(node_id) + "," + str(rate) + "\n"
        local_file.write( line )

    #num_empty = max_nodes - len( node_id_list )
    #for i in range( num_empty ):
    #    line = str(node['NodeID']) + "," + str(0) + "," + str( 0.0 ) + "\n"
    #    local_file.write( line )


def WriteToWorkMigration( work_file, node, work_node_id ):

    num_entries = 0
    if( (int( node["NodeID"] ) != work_node_id) and (int(node['NodeAttributes']['InitialPopulation']) > 0) ):
        work_rate = 1.0 / WORK_NODE_DAYS_BETWEEN_TRIPS
        line = str(node['NodeID']) + "," + str(work_node_id) + "," + str(work_rate) + "\n"
        work_file.write( line )
        num_entries += 1

    num_empty = MAX_WORK_NODES - num_entries
    for i in range( num_empty ):
        line = str(node['NodeID']) + "," + str(0) + "," + str( 0.0 ) + "\n"
        work_file.write( line )


if __name__ == "__main__":
    if len(sys.argv) != 5:
        ShowUsage()
        exit(0)

    prefix       = sys.argv[1]
    grid_size    = float(sys.argv[2] )
    work_node_id = int(  sys.argv[3] )
    is_vectors   = int(  sys.argv[4] ) == 1

    local_fn     = prefix + "_Local_"
    regional_fn  = prefix + "_Regional_"
    work_fn      = prefix + "_Work_"
    if( is_vectors ):
        local_fn    += "Vector_"
        regional_fn += "Vector_"
        work_fn     += "Vector_"

    local_fn     += "Migration.txt"
    regional_fn  += "Migration.txt"
    work_fn      += "Migration.txt"

    demog_fn     = prefix + "_Demographics.json"

    demog_file = open(demog_fn,'r')
    demogjson = json.load( demog_file )
    demog_file.close()

    local_days_between_trips    = LOCAL_DAYS_BETWEEN_TRIPS
    regional_days_between_trips = REGIONAL_DAYS_BETWEEN_TRIPS
    if( is_vectors ):
        local_days_between_trips    = VECTOR_LOCAL_DAYS_BETWEEN_TRIPS
        regional_days_between_trips = VECTOR_REGIONAL_DAYS_BETWEEN_TRIPS

    local_file = open(local_fn,'w')
    regional_file = open(regional_fn,'w')
    work_file = open(work_fn,'w')
    for node in demogjson['Nodes']:
        adjacent_node_id_list = []
        regional_node_id_list = []
        FindAdjacentNodes( node, demogjson, grid_size, work_node_id, is_vectors, adjacent_node_id_list, regional_node_id_list )        

        # -------------------------------------------------------------
        # For adjacent nodes, we want per_node_factor to always be 8.
        # This helps to keep the flow of migration constant between
        # edge/corner nodes and interior nodes.  The idea is that the entities 
        # in the edge/corner nodes virtually migrate by staying put.
        # -------------------------------------------------------------
        local_node_factor = 8

        # ----------------------------------------------------------------
        # DMB - I didn't like how high the regional vector migration rate
        #  was when dividing by the non-adjacent nodes.  I don't want to
        # update the human migration at this time, but we probably should.
        # ----------------------------------------------------------------
        regional_node_factor = len(regional_node_id_list)
        if( is_vectors ):
            regional_node_factor = len(regional_node_id_list) + len(adjacent_node_id_list)

        WriteToMigration( local_file,    node, MAX_LOCAL_NODES,    local_days_between_trips,    local_node_factor,    adjacent_node_id_list )
        WriteToMigration( regional_file, node, MAX_REGIONAL_NODES, regional_days_between_trips, regional_node_factor, regional_node_id_list )
        if( not is_vectors ):
            WriteToWorkMigration( work_file, node, work_node_id )

    local_file.close()
    regional_file.close()
    work_file.close()
