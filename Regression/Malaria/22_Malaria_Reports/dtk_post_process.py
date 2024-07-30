# compare_spatial_reports.py
# -----------------------------------------------------------------------------
# DMB 3/29/2017
# PURPOSE: This script is used to verify that the data created by SpatialReportMalariaFiltered
# is valid.  It compares different time and node filters.  The data by this custom report
# is also compared to the built in report and InsetChart.json.
# -----------------------------------------------------------------------------

import sys, os, json, collections, struct, datetime

# ---------------------------------------------------------------------------------------
# Spatial - Channels to output via SpatialReportMalariaFiltered and built-in
# InsetChart - The names of the channels in InsetChart.json that match those in the spatial reports.
# ---------------------------------------------------------------------------------------

channel_name_map_spatial_to_inset = collections.OrderedDict()
channel_name_map_spatial_to_inset[ "Adult_Vectors"           ] = "Adult Vectors"
channel_name_map_spatial_to_inset[ "New_Clinical_Cases"      ] = "New Clinical Cases"
channel_name_map_spatial_to_inset[ "New_Infections"          ] = "New Infections"
channel_name_map_spatial_to_inset[ "Population"              ] = "Statistical Population"
channel_name_map_spatial_to_inset[ "PCR_Parasite_Prevalence" ] = "PCR Parasite Prevalence"
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
                    #print( str(t)+","+str(n)+","+ str(ch_data) )
                    self.data_per_node[n].append( ch_data )

        CompareValues( messages, "num_nodes", exp_num_nodes, self.num_nodes )
        CompareValues( messages, "exp_num_timesteps", exp_num_timesteps, self.num_timesteps )

# ---------------------------------------------------------------------------------------
# Compare two spatial files / channel.
# This assumes that the actual file is a subset of the expected file.
# ---------------------------------------------------------------------------------------
def CompareData( messages,
                 exp_data, exp_num_timesteps, exp_num_nodes,
                 act_data, act_num_timesteps, act_num_nodes,
                 time_offset, node_offset ):

    messages.append( "============= CompareData ===============" )
    success = True
    success = success and CompareValues( messages, "exp_num_nodes",     exp_num_nodes,     exp_data.num_nodes     )
    success = success and CompareValues( messages, "act_num_nodes",     act_num_nodes,     act_data.num_nodes     )
    success = success and CompareValues( messages, "exp_num_timesteps", exp_num_timesteps, exp_data.num_timesteps )
    success = success and CompareValues( messages, "act_num_timesteps", act_num_timesteps, act_data.num_timesteps )

    if not success:
        return success
        
    act_inode = 0
    exp_inode = node_offset
    
    exp_data.fn = exp_data.fn.replace('\\','/')
    act_data.fn = act_data.fn.replace('\\','/')

    messages.append( "exp_fn="+exp_data.fn+"  act_fn="+act_data.fn )
    while( act_inode < act_num_nodes ):
        act_itime = 0
        exp_itime = time_offset

        while( act_itime < act_num_timesteps ):
            var = "data[exp(" + str(exp_inode) +","+str(exp_itime) +"),act("+str(act_inode)+","+str(act_itime) + ")]" 
            exp = exp_data.data_per_node[exp_inode][exp_itime]
            act = act_data.data_per_node[act_inode][act_itime]
            success = CompareValues( messages, var, exp, act )
            if not success:
                return success
            exp_itime += 1
            act_itime += 1
        exp_inode += 1
        act_inode += 1
    messages.append( "!!!!!!!!!!!!!!!!!! PASSED !!!!!!!!!!!!!!!!!!!!!!!" )
    return success


# ---------------------------------------------------------------------------------------
# This class is used to represent the channel data in an InsetChart
# NOTE: Adult Vectors and Infected are averages for the nodes versus a total count.
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
                if( (channel_name_inset == "Infected") or 
                    (channel_name_inset == "PCR Parasite Prevalence") ):
                    pop = spatial_data_pop.data_per_node[inode][itime]
                    prev = spatial_data.data_per_node[inode][itime]
                    val = val + (pop*prev)
                    total_pop += pop
                else:
                    val += spatial_data.data_per_node[inode][itime]
            if( channel_name_inset == "Adult Vectors" ):
                val = val / spatial_data.num_nodes
            elif( (channel_name_inset == "Infected") or 
                (channel_name_inset == "PCR Parasite Prevalence") ):
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
    
    builtin_fn_pop = os.path.join( output_path, "SpatialReport_Population.bin" )
    builtin_data_pop = SpatialData()
    builtin_data_pop.ReadSpatialFile( messages, builtin_fn_pop, False, 1, 1095 )
    
    success = True
    for ch in channel_name_map_spatial_to_inset:
        test_fn = os.path.join( output_path, "SpatialReportMalariaFiltered_" )

        # -------------------------------------------------------------------------
        # Test that the file generated by the custom report is the same as that by the built-in
        # -------------------------------------------------------------------------
        test_data = SpatialData() # File H
        test_data.ReadSpatialFile( messages, test_fn + ch + ".bin", True, 1, 1095 )

        builtin_fn = os.path.join( output_path, "SpatialReport_" ) + ch + ".bin"
        builtin_data = SpatialData()
        builtin_data.ReadSpatialFile( messages, builtin_fn, False, 1, 1095 )

        success = CompareData( messages, builtin_data, 1095, 1, test_data,  1095, 1, 0, 0 )
        if not success:
            break

        # ------------------------------------------------------------------------------
        # Save the data from the spatial report in a look-a-like inset chart data class
        # ------------------------------------------------------------------------------
        inset_chart_data_from_spatial.Convert( ch, builtin_data, builtin_data_pop )

    # -------------------------------------------------------------------------
    # Test that the data from the Spatial Report is the same as that in the InsetChart
    # -------------------------------------------------------------------------
    inset_chart_fn = os.path.join( output_path, "InsetChart.json" )
    inset_chart = InsetChart()
    inset_chart_data = inset_chart.Read( inset_chart_fn, 1095 )

    CompareInsetChart( messages, inset_chart_data, inset_chart_data_from_spatial )
    
    output = {}
    output["messages"] = []
    for line in messages:
        output["messages"].append(line)
    
    report_fn = os.path.join( output_path, "spatial_report_check.json" )
    with open(report_fn, 'w') as report_file:
        json.dump(output, report_file, indent=4)
    
    print("!!!!! Done checking spatial reports !!!!!")

# ---------------------------------------------------------------------------------------
# ---------------------------------------------------------------------------------------
if __name__ == "__main__":
    if len(sys.argv) != 2:
        ShowUsage()
        exit(0)

    application( sys.argv[1] )
    
# ---------------------------------------------------------------------------------------
# ---------------------------------------------------------------------------------------
