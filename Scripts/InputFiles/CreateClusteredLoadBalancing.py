import sys, os, json, numpy, struct, array
import getopt
import matplotlib.pyplot as plt
import matplotlib.cm as cm
from scipy.cluster.vq import *

nclusters = 8
niterations = 50
max_equal_clusters_iterations = 5000
write_binary_output = True
geography = 'the_geography_name'
demogfilename = 'some_base_layer_demographics.json'
lbfilename = 'output_file_load_balancing_name_' + str(nclusters) + 'procs.bin'
max_marker_size = 500
cluster_max_over_avg_threshold = 1.6
showcharts = True
notagsoutfile = False

try:
   opts, args = getopt.getopt(sys.argv[1:],"",["nclusters=","niterations=","maxiter=","writebin=","geography=","demogfilename=","lbfilename=","maxmarker=","maxloadratio=","showcharts=","notagsoutfile="])
except getopt.GetoptError:
   print ''
   print 'usage:'
   print 'CreateClusteredLoadBalancing.py --nclusters= --niterations= --maxiter= --writebin= --geography= --demogfilename= --lbfilename= --maxmarker= --maxloadratio= --showcharts= --notagsoutfile='
   sys.exit(2)
for opt, arg in opts:
   if opt in '--nclusters':
      nclusters = int (arg)
   elif opt in '--niterations':
      niterations = int (arg)
   elif opt in '--maxiter':
      max_equal_clusters_iterations = int (arg)
   elif opt in '--writebin':
      if int(arg) == 1: write_binary_output = True
      else: write_binary_output = False
   elif opt in '--geography':
      geography = arg
   elif opt in '--demogfilename':
      demogfilename = arg
   elif opt in '--lbfilename':
      lbfilename = arg
   elif opt in '--maxmarker':
      max_marker_size = int(arg)
   elif opt in '--maxloadratio':
      cluster_max_over_avg_threshold = arg
   elif opt in '--showcharts':
      if int(arg) == 1: showcharts = True
      else: showcharts = False
   elif opt in '--notagsoutfile':
      if int(arg) == 1: notagsoutfile = True
      else: notagsoutfile = False

if not notagsoutfile:
    lbfilename = lbfilename + str(nclusters) + 'procs.bin'

with open(demogfilename, 'r') as file:
    demogjson = json.load(file) #TODO: object_pairs_hook??

numnodes = demogjson['Metadata']['NodeCount']

default_population = 0
if 'Defaults' in demogjson:
    if 'NodeAttributes' in demogjson['Defaults']:
        if 'InitialPopulation' in demogjson['Defaults']['NodeAttributes']:
            default_population = demogjson['Defaults']['NodeAttributes']['InitialPopulation']
        else:
            print("Demographics file has no property ['Defaults']['NodeAttributes']['InitialPopulation']")
    else:
        print("Demographics file has no property ['Defaults']['NodeAttributes']")
else:
    print("Demographics file has no property ['Defaults']")

print('There are ' + str(numnodes) + ' nodes in this demographics file')

lats  = []
longs = []
node_ids = []
node_pops = []
for node in demogjson['Nodes']:
#    print( 'Node ID: ' + str(node['NodeID']) + 
#           '\tLat: ' + str(node['NodeAttributes']['Latitude']) + 
#           '\tLong: ' + str(node['NodeAttributes']['Longitude']) )

    lats.append(node['NodeAttributes']['Latitude'])
    longs.append(node['NodeAttributes']['Longitude'])
    node_ids.append(node['NodeID'])
    if 'InitialPopulation' in node['NodeAttributes']:
        node_pops.append(node['NodeAttributes']['InitialPopulation'])
    else:
        node_pops.append(default_population)

# cluster node IDs by lat/long
# TODO: post-processing to require equal number of nodes in each cluster??
#       find few nearest neighbors of most populous cluster; give least populous neighbor closest node; iterate??
# OR:   use centroids as seeds for next kmeans??  or is that what iterations is already doing??
# OR:   scrap kmeans and use some other 'districting' algorithm like Voronoi Tesselation or whatever??
# FOR NOW: brute force repeat of kmeans until equality threshold is passed
iterations = 0
while(True):
    res, idx = kmeans2(numpy.array(zip(longs,lats)), nclusters, niterations, 1e-05, 'points')

    #print('kmeans centroids: ' + str(res))
    #print('kmeans indices: ', idx)
    #print('unique indices: ' + str(numpy.unique(idx)))

    counts, binedges = numpy.histogram(idx, bins=nclusters, weights=node_pops)
    print('Population per node-cluster: ' + str(counts))
    biggest_over_avg = float(nclusters)*max(counts)/sum(node_pops)
    print('Population of largest cluster (' + str(max(counts)) + ') is ' + str(int(100*(biggest_over_avg-1))) + '% bigger than average (' + str(int(sum(node_pops)/float(nclusters))) + ')')
    iterations = iterations + 1
    if biggest_over_avg < cluster_max_over_avg_threshold:
        break
    if iterations > max_equal_clusters_iterations:
        print('SORRY I FAILED to find a clustering solution that passed the cluster-size-similarity threshold :(')
        break

# get a colormap for cluster coloring
# TODO: is there a better way to avoid adjacent colors being nearly indistinguishable?
colors = ( [ cm.jet(i*256/nclusters) for i in idx ] )

max_node_pop = max(node_pops)
sizes = ( [max_marker_size*node_pop/float(max_node_pop) for node_pop in node_pops] )

# get node IDs in order of cluster ID in preparation for writing load-balancing file
node_ids_by_index = zip(idx, node_ids, lats, longs)
node_ids_by_index.sort()
#for node_id_index_pair in node_ids_by_index:
#    print('Cluster idx = ' + str(node_id_index_pair[0]) + '\t Node ID = ' + str(node_id_index_pair[1]))

# TODO: order clusters according to some simple lat/long grouping
#       this way, a 32 cluster file will be still pretty good for an 8 core simulation, etc.

# prepare arrays for writing binary file
sorted_node_ids = [ int(x[1]) for x in node_ids_by_index ]
cum_load_list = list( numpy.arange(0,1,1.0/numnodes) )
#print(cum_load_list)

# write binary load-balancing file
if write_binary_output:
    with open(lbfilename, 'wb') as newfile:
        newfile.write(struct.pack('I',numnodes))
        node_id_array = array.array('I')
        node_id_array.fromlist(sorted_node_ids)
        node_id_array.tofile(newfile)
        cum_load_array = array.array('f')
        cum_load_array.fromlist(cum_load_list)
        cum_load_array.tofile(newfile)
else:
    print('Not writing a binary load-balancing file as output...')
    print(sorted_node_ids)
    print(cum_load_list)

if showcharts:
    # plot lat/long of nodes, colored by cluster
    plt.scatter(longs, lats, s=sizes, c=colors)
    plt.title('Lat/Long scatter of %s nodes' % geography)
    plt.axis('equal')
    plt.show()

