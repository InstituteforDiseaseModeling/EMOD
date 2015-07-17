rm( list=ls( all=TRUE ) )
graphics.off()

library(reshape)
library(ggplot2)

DAYS_PER_YEAR = 365
ON_ART = 4

dat <- read.csv("output/HIVMortality.csv", header=TRUE)
dat$Count = 1
dat$ARTYears = dat$Death_time/DAYS_PER_YEAR - dat$Years_since_first_ART_initiation

dat.m = melt(dat, id=c('HIV_disease_state_just_prior_to_death', 'Death_time', 'ARTYears'), measure='Count')
dat.c = cast(dat.m, ARTYears ~ variable, mean, subset=(HIV_disease_state_just_prior_to_death==ON_ART))

### Plot ###
p <- ggplot(dat.c) + 
    geom_histogram(aes(x=ARTYears, y=..count..), binwidth=1)

pdf('HIVMortality.pdf')
print( p )
dev.off()
