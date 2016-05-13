import json
from collections import OrderedDict
from ScalingMatrixFromMigration import *
from struct import unpack


def GenerateDemoPoolFiles(demographicFilename, totalPopulation, demoIdList, populationList, migrationlinks, returnRate, outputFilename):

    fdemo = open(demographicFilename)
    inputDemographics = json.load(fdemo, object_pairs_hook = OrderedDict)
    fdemo.close()

    pools = demoIdList
    print "generating movement fraction"

    movementPoolFraction = calculateScalingMatrixFromMovementLinks(pools, migrationlinks, returnRate)

    #prepare scalingmatrix
    scalingmatrix = []
    for i in xrange(len(movementPoolFraction)):
        scalingmatrix.append([])
        for j in xrange(len(movementPoolFraction)):
            scalingmatrix[i].append(0.0)

    for idx1 in xrange(len(demoIdList)):
        for idx2 in xrange(len(demoIdList)):
            ID1 = demoIdList[idx1]
            ID2 = demoIdList[idx2]
            scalingmatrix[idx1][idx2] = movementPoolFraction[ID1][ID2]

    #here, we assume the fraction of each population group stays the same
    betamatrix = calculateBetaMatrixFromScalingMatrix(populationList, scalingmatrix)
    #add leading zeros
    for i in xrange(len(demoIdList)):
        demoIdList[i] = demoIdList[i].zfill(3)
    
    #generate individual properties for HINT
    mixingpool = OrderedDict()
    mixingpool["Property"] = "Geographic"
    mixingpool["Values"] = demoIdList
    mixingpool["Initial_Distribution"] = populationList
    mixingpool["Transitions"] = []
    mixingpool["TransmissionMatrix"] = OrderedDict()
    mixingpool["TransmissionMatrix"]["Route"] = "Contact"
    mixingpool["TransmissionMatrix"]["Matrix"] = []
    for Id in xrange(len(scalingmatrix)):
        mixingpool["TransmissionMatrix"]["Matrix"].append(betamatrix[Id])

    #generate new node including all populations
    newnode = OrderedDict()
    newnode["NodeID"] = 1
    newnode["NodeAttributes"] = OrderedDict()
    newnode["NodeAttributes"]["Latitude"] = 0.0
    newnode["NodeAttributes"]["Longitude"] = 0.0
    newnode["NodeAttributes"]["Altitude"] = 0
    newnode["NodeAttributes"]["InitialPopulation"] = int(totalPopulation)


    #write demographic file with 1 node, multiple pools with movement fractions)
    outputDemographics = OrderedDict()
    outputDemographics["Metadata"] = inputDemographics["Metadata"]
    outputDemographics["Metadata"]["NodeCount"] = 1
    outputDemographics["Defaults"] = OrderedDict()
    outputDemographics["Defaults"]["NodeAttributes"] = inputDemographics["Defaults"]["NodeAttributes"]
    outputDemographics["Defaults"]["IndividualAttributes"] = inputDemographics["Defaults"]["IndividualAttributes"]
    outputDemographics["Defaults"]["IndividualProperties"] = []
    outputDemographics["Defaults"]["IndividualProperties"].append(mixingpool)
    outputDemographics["Nodes"] = []
    outputDemographics["Nodes"].append(newnode)

    fout = open(outputFilename,"w")
    fout.write(json.dumps(outputDemographics, indent=4))
    fout.close()

def ReadDemoFromTxt(inputDemo, inputMiglinks):

    demoIdList = []
    populationList = []
    totalPopulation = 0.0
    
    fdemo = open(inputDemo)
    migrationlinks = []
    fromtotuples = []
    
    for line in fdemo:
        s=line.strip().split()
        demoIdList.append(s[0])
        populationList.append(float(s[1]))
        totalPopulation += float(s[1])

    for inputMiglink in inputMiglinks:
        fmig = open(inputMiglink)
        for line in fmig:
            s=line.strip().split()
            fromId = s[0]
            toId = s[1]
            rate = float(s[2])
            newMigrationLink = {}
            newMigrationLink["from"] = fromId
            newMigrationLink["to"] = toId
            newMigrationLink["rate"] = rate
            fromtotuple = (fromId, toId)
            if fromtotuple in fromtotuples:
                print "existing link from-to pair",fromtotuple," with rate", destinationRate[i],", this link is not considered."
            else:
                migrationlinks.append(newMigrationLink) 

    #normalize
    for i in xrange(len(populationList)):
        populationList[i] = populationList[i]/ totalPopulation

    return totalPopulation, demoIdList, populationList, migrationlinks

def ReadDemoFromDTKInput(demographicFilename, migrationFilenames, numberMigrationDestinations):
    fdemo = open(demographicFilename)
    inputDemographics = json.load(fdemo, object_pairs_hook = OrderedDict)
    fdemo.close()

    demoIdList = []
    populationList = []
    totalPopulation = 0.0

    for node in inputDemographics["Nodes"]:
        nodeId = node["NodeID"]
        population = float(node["NodeAttributes"]["InitialPopulation"])
        demoIdList.append(str(nodeId))
        populationList.append(population)
        totalPopulation += population

    #normalize
    for i in xrange(len(populationList)):
        populationList[i] = populationList[i]/ totalPopulation

    migrationlinks = []
    fromtotuples = []
    for idx in xrange(len(migrationFilenames)):
        migrationFilename = migrationFilenames[idx]
        numberMigrationDestination = numberMigrationDestinations[idx]
        print "opening file",migrationFilename
        fmig = open(migrationFilename,"rb")

        byte='' #initialize
        count = 0
        while count < len(demoIdList):
            originId = demoIdList[count]
            destinationId = []
            destinationRate = []
            for i in xrange(numberMigrationDestination):
                byte = fmig.read(4)
                if byte != "":
                    destinationId.append(str(int(unpack('L',byte)[0])))
            for i in xrange(numberMigrationDestination):
                byte = fmig.read(8)
                if byte != "":
                    destinationRate.append(float(unpack('d',byte)[0]))

            for i in xrange(len(destinationId)):
                if destinationId[i] != 0:
                    newMigrationLink = {}
                    newMigrationLink["from"] = originId
                    newMigrationLink["to"] = destinationId[i]
                    newMigrationLink["rate"] = destinationRate[i]
                    fromtotuple = (originId, destinationId[i])
                    if fromtotuple in fromtotuples:
                        print "existing link from-to pair",fromtotuple," with rate", destinationRate[i],", this link is not considered."
                    else:
                        migrationlinks.append(newMigrationLink) 
                    #print originId, destinationId[i], destinationRate[i]
            count+=1
        fmig.close()
    return totalPopulation, demoIdList, populationList, migrationlinks

if __name__=="__main__":
    demographictemplate = "Seattle_template.json"
    #demographictemplate = "Seattle_30arcsec_demographics.json"

    #generate demographic input files from previous DTK multi-node input files
    demographicFilename = "Seattle_30arcsec_demographics.json"
    migrationFilenames = ["Seattle_30arcsec_regional_migration.bin","Seattle_30arcsec_local_migration.bin"]
    numberMigrationDestinations = [30, 8]
    [totalPopulation, demoIdList, populationList, migrationlinks] =ReadDemoFromDTKInput(demographicFilename, migrationFilenames, numberMigrationDestinations)

    #generate demographic input files from a txt file
    #demographicFilename = "demographics.txt"
    #migrationFilenames = ["migration.txt"]
    #[totalPopulation, demoIdList, populationList, migrationlinks] =ReadDemoFromTxt(demographicFilename, migrationFilenames)
    
    returnRate = 3.0 # return rate in days, e.g. 8 hours spent at destination = (1/(1/3 day) = 3.0
    outputFilename = "Seattle_singlenode_mixingpools.json"
    
    GenerateDemoPoolFiles(demographictemplate, totalPopulation, demoIdList, populationList, migrationlinks, returnRate, outputFilename)
