# SUMMARY: Plot percent of births that are HIV+ over time to show the impact of PMTCT and ART
# INPUT: 
#   1. config.json
#   2. output/RelationshipStart.csv
# OUTPUT: figs/VerticalTransmission.png

rm( list=ls( all=TRUE ) )
graphics.off()

library(reshape)
library(ggplot2)
library(plyr)
library(jsonlite)

PMTCT_YEAR = 2022
ART_YEAR = 2024

fig_dir = 'figs'
if( !file.exists(fig_dir) ) {
    dir.create(fig_dir)
}

config <- fromJSON("config.json")$parameters
dat <- read.csv("output/ReportEventRecorder.csv", header=TRUE)
dat$Count = 1

# Bin years
dat$Year = floor(5*dat$Year)/5

Years = unique(dat$Year)
nYears = length(Years)
base = data.frame( Year=rep(Years, each=2), HasHIV=rep(c('Y', 'N'), nYears), Count=rep(0, 2*nYears) )

dat.m = melt(dat, id=c('Year','HasHIV'), measure='Count')
dat.c = cast(dat.m, Year + HasHIV ~ variable, sum)

dat.cm = merge(base, dat.c, by=c("Year","HasHIV"), all=TRUE)
dat.cm$Count = dat.cm$Count.x + dat.cm$Count.y
dat.cm$Count[is.na(dat.cm$Count)] = 0

# Year tot
dat.cmp = ddply(dat.cm, .(Year), transform, Total=sum(Count))
dat.cmp$Percent = 100* dat.cmp$Count / dat.cmp$Total
dat.cmp$HasHIV = factor(dat.cmp$HasHIV, labels=c("No", "Yes"))

dat.hashiv = dat.cmp[dat.cmp$HasHIV == 'Yes',]


### Plot ###
p = ggplot(dat.hashiv, aes(x=Year, y=Percent)) +
    #geom_histogram(stat="identity") +
    geom_line(colour='Red', size=1.5) +
    geom_vline(xintercept = PMTCT_YEAR, colour='black', linetype='dashed', size=1) +
    annotate( "text", label="PMTCT: 60% Efficacy", x=PMTCT_YEAR-0.25, y=9 ,angle=90) +
    geom_vline(xintercept = ART_YEAR, colour='black', linetype='dashed', size=1) +
    annotate( "text", label="ART: 90% Efficacy", x=ART_YEAR-0.25, y=25 ,angle=90) +
    xlab( 'Year' ) +
    ylab( 'Percent of Births HIV+' ) + 
    ggtitle( 'Vertical Transmission' )

png( file.path(fig_dir,"VerticalTransmission.png"), width=600, height=400)
print( p )
dev.off()

dev.new()
print(p)

