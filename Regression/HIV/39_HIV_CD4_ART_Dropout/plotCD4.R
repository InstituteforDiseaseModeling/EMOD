rm( list=ls( all=TRUE ) )
graphics.off()

library(reshape)
library(ggplot2)

dat <- read.csv("output/ReportHIVInfection.csv", header=TRUE)

min_id = 50
max_id = 100

dat.m = melt(dat, id=c('Id','TimeSinceHIV', 'PrognosisCompletedFraction', 'ArtStatus'), measure='CD4count')
dat.c = cast(dat.m, Id + TimeSinceHIV ~ variable, mean, subset=(Id>=min_id & Id<max_id))
dat.d = cast(dat.m, Id + PrognosisCompletedFraction ~ variable, mean, subset=(Id>=min_id & Id<max_id))


dat.em = melt(dat, id=c('Id','TimeSinceHIV'), measure='ArtStatus')
dat.ec = cast(dat.em, Id + TimeSinceHIV ~ variable, mean, subset=(Id>=min_id & Id<max_id))

### Plot ###
p <- ggplot(dat.c, aes(x=TimeSinceHIV, y=CD4count, group=Id)) + geom_line(aes(colour=Id))

#pdf('CD4Progression.pdf')
dev.new()
print( p )
#dev.off()

dev.new()
print( ggplot(dat.c, aes(x=TimeSinceHIV, y=sqrt(CD4count), group=Id)) + geom_line(aes(colour=Id)) )

dev.new()
print( ggplot(dat.d, aes(x=PrognosisCompletedFraction, y=CD4count, group=Id)) + geom_line(aes(colour=Id)) )

dev.new()
print( ggplot(dat.ec, aes(x=TimeSinceHIV, y=ArtStatus, group=Id)) + geom_line(aes(colour=Id)) )
