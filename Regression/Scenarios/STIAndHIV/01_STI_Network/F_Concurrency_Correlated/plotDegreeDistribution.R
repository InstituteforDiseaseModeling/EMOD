# SUMMARY: Plot the degree distribution for individuals in a relationship at the end of the simulation.  Compare Rate_Ratio = 1 to Rate_Ratio = 10 from the two corresponding sub-directories.  The main challenge is to deduce active relationsihps from RelationshipStart and RelationshipEnd.  Note that the campaign json has a STIIsPostDebut diagnostic that broadcasts a message for each individual that is logged by the event recorder.
# INPUT: 
#   1. RateRatio1/output/RelationshipStart.csv
#   2. RateRatio1/output/RelationshipEnd.csv
#   3. RateRatio1/output/ReportEventRecorder.csv
#   4. RateRatio10/output/RelationshipStart.csv
#   5. RateRatio10/output/RelationshipEnd.csv
#   6. RateRatio10/output/ReportEventRecorder.csv
# OUTPUT: 
#   1. figs/DegreeDistribution.png
#   2. figs/DegreeDistributionByRelType.png

rm( list=ls( all=TRUE ) )
graphics.off()

library(plyr)
library(reshape)
library(ggplot2)

DAYS_PER_YEAR = 365

rel_names <- c('Transitory', 'Informal', 'Marital')
fig_dir = 'figs'
if( !file.exists(fig_dir) ) {
    dir.create(fig_dir)
}


active_rels_at_end <- function(dir, Scenario)
{
    start= read.csv(file.path(dir, "output", "RelationshipStart.csv"), header=TRUE)
    end= read.csv(file.path(dir, "output", "RelationshipEnd.csv"), header=TRUE)
    names(start)[names(start)=='Rel_type..0...transitory.1...informal.2...marital.']='Rel_Type'
    names(end)[names(end)=='Rel_type..0...transitory.1...informal.2...marital.']='Rel_Type'
    start$Scenario = Scenario
    end$Scenario = Scenario
    Rel_IDs_Active_At_End= setdiff(start$Rel_ID, end$Rel_ID)
    active = data.frame( start[Rel_IDs_Active_At_End,] )

    return(active)
}

build_degree_mat <- function (events.postdebut, active) {
    degreeMatByRelType = array(0, dim=c(2, 10,10,10)) # Gender, Transitory, Informal, Marital
    degreeMat= array(0, dim=c(2, 10)) # Gender, frequency of all rel types
    for( i in 1:nrow(events.postdebut) ) {
        id = events.postdebut[i,]$Individual_ID

        gender = events.postdebut[i,]$Gender
        gender_idx = 1
        if( gender == 'F' ) { gender_idx = 2 }

        active.thisid = active[ active$A_ID == id | active$B_ID == id,]

        num_rels = nrow(active.thisid)
        degreeMat[ gender_idx, num_rels+1 ] = degreeMat[ gender_idx, num_rels+1 ] + 1

        count = c(1,1,1)
        if( nrow(active.thisid) > 0 ) {
            for( ri in 1:nrow(active.thisid) ) {
                rel_idx = active.thisid[ri,]$Rel_Type + 1
                count[rel_idx] = count[rel_idx] + 1
            }
        }

        degreeMatByRelType[gender_idx, count[1], count[2], count[3]] = degreeMatByRelType[gender_idx, count[1], count[2], count[3]] + 1
    }

    return( list(degreeMatByRelType = degreeMatByRelType, degreeMat = degreeMat) )
}

build_degree_distribution_by_rel_type <- function(degreeMatByRelType, Scenario) {

    degreeDistributionByRelType = data.frame( Gender = factor(), Rel_Type = factor(), Scenario = factor(), Degree=integer(), Count=integer())

    gender_names = c("Male", "Female")
    for( gender in 1:2 ) {
        for( rel_idx in 1:3 ) {
            counts = apply(degreeMatByRelType[gender,,,], rel_idx, sum)
            for( ci in 1:length(counts) ) {
                dd = data.frame( Gender = gender_names[gender], Rel_Type = rel_names[rel_idx], Scenario = Scenario, Degree = ci-1, Count = counts[ci] )
                degreeDistributionByRelType = rbind(degreeDistributionByRelType, dd)
            }
        }
    }

    # Remove zeros
    degreeDistributionByRelType = degreeDistributionByRelType[ degreeDistributionByRelType$Count != 0, ]

    return( degreeDistributionByRelType )
}


build_degree_distribution <- function(degreeMat, Scenario) {

    degreeDistribution = data.frame( Gender = factor(), Scenario = factor(), Degree=integer(), Count=integer())

    gender_names = c("Male", "Female")
    for( gender in 1:2 ) {
        counts = degreeMat[gender,]
        for( ci in 1:length(counts) ) {
            dd = data.frame( Gender = gender_names[gender], Scenario = Scenario, Degree = ci-1, Count = counts[ci] )
            degreeDistribution = rbind(degreeDistribution, dd)
        }
    }

    # Remove zeros
    degreeDistribution = degreeDistribution[ degreeDistribution$Count != 0, ]

    return( degreeDistribution )
}

active.rr1 = active_rels_at_end('RateRatio1', 'RateRatio=1')
events.rr1 = read.csv(file.path("RateRatio1", "output", "ReportEventRecorder.csv"), header=TRUE)
events.postdebut.rr1 = events.rr1[events.rr1$Event_Name == 'PostDebut',]
tmp = build_degree_mat(events.postdebut.rr1, active.rr1)
degreeMatByRelType.rr1 = tmp$degreeMatByRelType
degreeMat.rr1 = tmp$degreeMat
degreeDistributionByRelType.rr1 = build_degree_distribution_by_rel_type(degreeMatByRelType.rr1, 'RateRatio=1')
degreeDistribution.rr1 = build_degree_distribution(degreeMat.rr1, 'RateRatio=1')

active.rr10 = active_rels_at_end('RateRatio10', 'RateRatio=10')
events.rr10 = read.csv(file.path("RateRatio10", "output", "ReportEventRecorder.csv"), header=TRUE)
events.postdebut.rr10 = events.rr10[events.rr10$Event_Name == 'PostDebut',]
tmp = build_degree_mat(events.postdebut.rr10, active.rr10)
degreeMatByRelType.rr10 = tmp$degreeMatByRelType
degreeMat.rr10 = tmp$degreeMat
degreeDistributionByRelType.rr10 = build_degree_distribution_by_rel_type(degreeMatByRelType.rr10, 'RateRatio=10')
degreeDistribution.rr10 = build_degree_distribution(degreeMat.rr10, 'RateRatio=10')

degreeDistributionByRelType = rbind(degreeDistributionByRelType.rr1, degreeDistributionByRelType.rr10)
degreeDistribution = rbind(degreeDistribution.rr1, degreeDistribution.rr10)

p = ggplot(degreeDistributionByRelType, aes(x=Degree, y=Count, group=Gender, fill=Gender)) +
    geom_bar(position="dodge", stat="identity") +
    xlab( "Current Number of Relationships by Type" ) +
    ylab( "Count" ) +
    ggtitle( "Degree Distribution by Relationship Type: Correlated" ) +
    scale_fill_manual(breaks=c("Male", "Female"), name="Gender", values=c("Blue", "Red")) +
    facet_grid( Rel_Type ~ Scenario )


png( file.path(fig_dir,"DegreeDistributionByRelType.png"), width=600, height=400)
print( p )
dev.off()

dev.new()
print(p)


p = ggplot(degreeDistribution, aes(x=Degree, y=Count, group=Gender, fill=Gender)) +
    geom_bar(position="dodge", stat="identity") +
    xlab( "Current Number of Relationships" ) +
    ylab( "Count" ) +
    ggtitle( "Degree Distribution: Correlated" ) +
    scale_fill_manual(breaks=c("Male", "Female"), name="Gender", values=c("Blue", "Red")) +
    facet_grid( . ~ Scenario )


png( file.path(fig_dir,"DegreeDistribution.png"), width=600, height=400)
print( p )
dev.off()

dev.new()
print(p)

