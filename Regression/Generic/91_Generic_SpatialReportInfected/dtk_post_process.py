# -----------------------------------------------------------------------------
# DMB 8/3/2020
# PURPOSE: This script is used to verify the SparialReport Infected channel.
# It will compare it to InsetChart.  It will also compare Population and Prevalence.
# Assume 10x the nodeID infected and 100x the nodeID is the population.
# This implies that the prevalence should be 10%.
# -----------------------------------------------------------------------------

import sys, os, json, collections, struct, datetime

# ---------------------------------------------------------------------------------------
# Spatial - Channels to output via SpatialReportMalariaFiltered and built-in
# InsetChart - The names of the channels in InsetChart.json that match those in the spatial reports.
# ---------------------------------------------------------------------------------------

channel_name_map_spatial_to_inset = collections.OrderedDict()
channel_name_map_spatial_to_inset[ "Population"              ] = "Statistical Population"
channel_name_map_spatial_to_inset[ "Infected"                ] = "Infected"
channel_name_map_spatial_to_inset[ "Prevalence"              ] = "Infected"

# ---------------------------------------------------------------------------------------
# Convert the channel name in the spatial report to that used in InsetChart.json
# ---------------------------------------------------------------------------------------

def ConvertChannelName( channel_name_spatial ):
    if channel_name_spatial not in channel_name_map_spatial_to_inset:
        print( "Unknown channel name=" + channel_name_spatial )
        exit(-1)
    return channel_name_map_spatial_to_inset[channel_name_spatial]

# ---------------------------------------------------------------------------------------
# Compare two values - My unit test like feature
# ---------------------------------------------------------------------------------------

def CompareValues( messages, var, exp, act ):
    success = True
    #print(var+" exp="+str(exp)+" act="+str(act))
    if( abs(exp - act) > 0.0000001 ):
        messages.append( var + ": Expected " + str(exp) +" but got " + str(act) )
        success = False
    return success

# ---------------------------------------------------------------------------------------
# This class is used to read and contain the data in a spatial data file / channel.
# The class can be used to read the files from the built-in report as well as the
# custom reports (i.e. filtered).
# ---------------------------------------------------------------------------------------
class SpatialData:
    def __init__( self ):
        self.fn = ""
        self.num_nodes     = 0
        self.num_timesteps = 0
        self.start_time    = 0.0 # only in custom/filtered file format
        self.interval_time = 0.0 # only in custom/filtered file format
        self.node_id_list  = []
        self.data_per_node = []

    def ReadSpatialFile( self, messages, filename, is_file_filtered, exp_num_nodes, exp_num_timesteps ):
        self.fn = filename
        print(filename)
        
        success = True
        with open( filename, 'rb' ) as file:
            int_data = file.read(4)
            self.num_nodes, = struct.unpack( "i", int_data )

            int_data = file.read(4)
            self.num_timesteps, = struct.unpack( "i", int_data )

            # is_file_filtered implies that it is from a custom report and has these two extra fields
            if( is_file_filtered ):
                float_data = file.read(4)
                self.start_time, = struct.unpack( "f", float_data )

                float_data = file.read(4)
                self.interval_time, = struct.unpack( "f", float_data )

            for n in range(self.num_nodes):
                int_data = file.read(4)
                node_id, = struct.unpack( "i", int_data )
                self.node_id_list.append( node_id )

            for n in range(self.num_nodes):
                self.data_per_node.append([])

            for t in range(self.num_timesteps):
                for n in range(self.num_nodes):
                    float_data = file.read(4)
                    ch_data, = struct.unpack( "f", float_data )
                    self.data_per_node[n].append( ch_data )
                    if filename == os.path.join( "output", "SpatialReport_Population.bin" ):
                        exp_pop = (n+1)*100
                        success = success and CompareValues( messages, "SpatialReport_Population", exp_pop, ch_data )
                    elif filename == os.path.join( "output", "SpatialReport_Infected.bin" ):
                        exp_inf = (n+1)*10
                        success = success and CompareValues( messages, "SpatialReport_Infected", exp_inf, ch_data )

        success = success and CompareValues( messages, "num_nodes", exp_num_nodes, self.num_nodes )
        success = success and CompareValues( messages, "exp_num_timesteps", exp_num_timesteps, self.num_timesteps )
        
        return success

# ---------------------------------------------------------------------------------------
# This class is used to represent the channel data in an InsetChart
# NOTE: Infected is average for the nodes versus a total count.
# ---------------------------------------------------------------------------------------
class InsetChartData:
    def __init__( self ):
        self.channel_data = {}

    def Convert( self, channel_name_spatial, spatial_data, spatial_data_pop ):
        channel_name_inset = ConvertChannelName( channel_name_spatial)
        self.channel_data[ channel_name_inset ] = []
        for itime in range(spatial_data.num_timesteps):
            val = 0
            total_pop = 0
            for inode in range(spatial_data.num_nodes):
                if( channel_name_inset == "Infected" ):
                    pop = spatial_data_pop.data_per_node[inode][itime]
                    prev = spatial_data.data_per_node[inode][itime]
                    val = val + (pop*prev)
                    total_pop += pop
                else:
                    val += spatial_data.data_per_node[inode][itime]
            if( channel_name_inset == "Adult Vectors" ):
                val = val / spatial_data.num_nodes
            elif( channel_name_inset == "Infected" ):
                val = val / total_pop
                
            self.channel_data[ channel_name_inset ].append( val )


# ---------------------------------------------------------------------------------------
# A class for reading an InsetChart.json file.
# ---------------------------------------------------------------------------------------
class InsetChart:
    def __init__( self ):
        self.fn = ""

    def Read( self, filename, exp_num_timesteps ):
        self.fn = filename

        with open( filename, 'r' ) as file:
            json_data = json.load( file )

        data = InsetChartData()

        for spatial_ch, inset_ch in channel_name_map_spatial_to_inset.items():
            data.channel_data[ inset_ch ] = json_data["Channels"][inset_ch]["Data"]
        return data

# ---------------------------------------------------------------------------------------
# Compare two inset charts.  Assume they have the same channel names.
# ---------------------------------------------------------------------------------------

def CompareInsetChart( messages, exp_data, act_data ):
    messages.append("!!!!!!!!!!!!!!!!!! Compare InsetChartData !!!!!!!!!!!!!!!!!!!!!!!")
    success = True
    for spatial_ch, inset_ch in channel_name_map_spatial_to_inset.items():
        var = inset_ch+"-num_values"
        success = CompareValues( messages, var, len( exp_data.channel_data[inset_ch] ), len( act_data.channel_data[inset_ch] ) )
        if not success:
            return success
        for i in range(len(exp_data.channel_data[inset_ch]) ):
            exp = exp_data.channel_data[inset_ch][i]
            act = act_data.channel_data[inset_ch][i]
            var2 = inset_ch+"["+str(i)+"]"
            success = CompareValues( messages, var2, exp, act )
            if not success:
                return success
                
    messages.append( "!!!!!!!!!!!!!!!!!! PASSED !!!!!!!!!!!!!!!!!!!!!!!")
    return success
    
# ---------------------------------------------------------------------------------------
# ---------------------------------------------------------------------------------------
def ShowUsage():
    print ('\nUsage: %s [output directory]' % os.path.basename(sys.argv[0]))

# ---------------------------------------------------------------------------------------
# ---------------------------------------------------------------------------------------
def application(output_path="output"):
    print("!!!!! Check spatial reports !!!!!")
    
    inset_chart_data_from_spatial = InsetChartData()

    messages = []
    
    success = True
    builtin_fn_pop = os.path.join( output_path, "SpatialReport_Population.bin" )
    builtin_data_pop = SpatialData()
    success = success and builtin_data_pop.ReadSpatialFile( messages, builtin_fn_pop, False, 5, 10 )
    
    success = True
    for ch in channel_name_map_spatial_to_inset:
        test_fn = os.path.join( output_path, "SpatialReport_" )

        builtin_fn = os.path.join( output_path, "SpatialReport_" ) + ch + ".bin"
        builtin_data = SpatialData()
        success = success and builtin_data.ReadSpatialFile( messages, builtin_fn, False, 5, 10 )

        # ------------------------------------------------------------------------------
        # Save the data from the spatial report in a look-a-like inset chart data class
        # ------------------------------------------------------------------------------
        inset_chart_data_from_spatial.Convert( ch, builtin_data, builtin_data_pop )

    # -------------------------------------------------------------------------
    # Test that the data from the Spatial Report is the same as that in the InsetChart
    # -------------------------------------------------------------------------
    inset_chart_fn = os.path.join( output_path, "InsetChart.json" )
    inset_chart = InsetChart()
    inset_chart_data = inset_chart.Read( inset_chart_fn, 10 )

    success = success and CompareInsetChart( messages, inset_chart_data, inset_chart_data_from_spatial )
    
    output = {}
    output["messages"] = []
    for line in messages:
        output["messages"].append(line)
    
    report_fn = os.path.join( output_path, "spatial_report_check.json" )
    with open(report_fn, 'w') as report_file:
        json.dump(output, report_file, indent=4)
    
    print("!!!!! Done checking spatial reports !!!!!")
    if success:
        print("Test SUCCEEDED")
    else:
        print("Test FAILED")

# ---------------------------------------------------------------------------------------
# ---------------------------------------------------------------------------------------
if __name__ == "__main__":
    if len(sys.argv) != 2:
        ShowUsage()
        exit(0)

    application( sys.argv[1] )
    
# ---------------------------------------------------------------------------------------
# ---------------------------------------------------------------------------------------
