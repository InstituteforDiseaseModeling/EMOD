import numpy as np
from numpy import linalg as LA

#given sigma as outgoing rate and tau as return rate, the effective
#population at pool i is:

#N_effi=Ni/(1+sigma_i/tau_i)+Sum_j[Nj*(sigma_ji/tau_j)/(1+sigma_j/tau_j)]

#first term is people in pool i stays in i, second term is people in
#pool j temporarily visiting i

#so, looking at each individual, the effective fraction for people staying
#in the default pool i is: 1/(1+sigma_i/tau_i)

#for every other connected pool j, the equivalent fraction for people going
#from i to j is: (sigma_ij/tau_i)/(1+sigma_i/tau_i)

#the above calculation needs return_rate >0

def calculateScalingMatrixFromMovementLinks(pools, movementLinks, returnRate):
    #initialize
    movementPoolFraction = {}
    sumWeight = {}
    for i in pools:
        movementPoolFraction[i] = {}
        sumWeight[i] = 0.0
        for j in pools:
            movementPoolFraction[i][j] = 0.0

    #calculate sumweight
    for link in movementLinks:
        fromId = link["from"]
        sumWeight[fromId] += link["rate"]   

    #calculate fraction of people who are staying in the default pool
    if returnRate > 0.0:
        for i in pools:
            movementPoolFraction[i][i]=1.0 / (1.0 + sumWeight[i] / returnRate)
    else:
        pass #people do not return, they won't spend any time in their original pool

    #now calculate the fraction of people going to other pools
    if len(movementLinks)>0:
        for link in movementLinks:
            fromId = link["from"]
            toId = link["to"]
            outgoingRate = link["rate"]
            
            if returnRate > 0.0:
                movementPoolFraction[fromId][toId] = ( outgoingRate / returnRate ) / (1.0 + sumWeight[fromId] / returnRate)
            elif returnRate == 0.0:
                if sumWeight[fromId] > 0:
                    movementPoolFraction[fromId][toId] = outgoingRate / sumWeight[fromId]
                else:
                    print "error: when return rate = 0 (diffusion), sum of outgoing rates need to be larger than 0."
                    exit(1)
            else:
                print "error: return rate needs to be larger or equal to 0."
                exit(1)
    else:
        print "no movement links, nothing to generate."
        exit(1)

    return movementPoolFraction

def calculateBetaMatrixFromScalingMatrix(populationList, scalingmatrix):
    scalingmatrix = np.array(scalingmatrix)
    populationList = np.array(populationList)
    temp_matrix = np.dot(scalingmatrix.T, populationList)
    inversed_matrix = LA.inv(np.diag(temp_matrix))

    betamatrix = np.dot(np.dot(scalingmatrix, inversed_matrix), scalingmatrix.T)
    
    return betamatrix.tolist()
