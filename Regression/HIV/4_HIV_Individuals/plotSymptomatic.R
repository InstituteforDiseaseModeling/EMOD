rm( list=ls( all=TRUE ) )
graphics.off()

library(reshape)
library(ggplot2)
library(jsonlite)

DAYS_PER_YEAR = 365

J = fromJSON('config.json')$parameters

mu = J$Days_Between_Symptomatic_And_Death_Weibull_Mean
inv_kap = J$Days_Between_Symptomatic_And_Death_Weibull_Heterogeneity

scale = mu/gamma(1+inv_kap) / DAYS_PER_YEAR
shape = 1/inv_kap


#dat.inf <- read.csv("output/ReportHIVInfection.csv", header=TRUE)
#dat.inf$Count = 1
#dat.inf.m = melt(dat.inf, id=c('Id','Prognosis'), measure='Count')
#dat.inf.c = cast(dat.inf.m, Id + Prognosis ~ variable, mean , subset=(Id<=10) )

dat.mort <- read.csv("output/HIVMortality.csv", header=TRUE)
dat.mort$Count = 1

dat.events <- read.csv("output/ReportEventRecorder.csv", header=TRUE)
#dat.events$Years_to_symptomatic = dat.events$Year - BASE_YEAR
dat.events$Count = 1

dat.mort.m = melt(dat.mort, id=c('id','Death_was_HIV_cause', 'Years_since_infection', 'Age'), measure='Count')
dat.mort.c = cast(dat.mort.m, id + Years_since_infection ~ variable, mean, subset=(Death_was_HIV_cause==1 & Age>=15)) # 

dat.events.m = melt(dat.events, id=c('Individual_ID','Year', 'CD4', 'Event_Name'), measure='Count')
dat.sym_events.c = cast(dat.events.m, Individual_ID + Year~ variable, mean, subset=(Event_Name == 'HIVSymptomatic')) # 
dat.inf_events.c = cast(dat.events.m, Individual_ID + Year~ variable, mean, subset=(Event_Name == 'NewInfectionEvent')) # 

dat.events.c = merge(dat.inf_events.c, dat.sym_events.c, by='Individual_ID')
dat.events.c$Years_to_symptomatic = dat.events.c$Year.y - dat.events.c$Year.x

dat.events.cd4.c = cast(dat.events.m, Individual_ID + CD4 ~ variable, mean, subset=(Event_Name == 'HIVSymptomatic')) # 

dat.merge = merge(dat.mort.c, dat.events.c, by.x='id', by.y='Individual_ID')
# Year is death year
dat.merge$symptomatic_years_before_death = dat.merge$Years_since_infection - dat.merge$Years_to_symptomatic

### Plot ###
dev.new()
# NOTE: Assuming all HIV+ die in scaling CDF
p <- ggplot() + 
    geom_histogram(data=dat.merge, aes(x=symptomatic_years_before_death, y=cumsum(..count..))) +
    stat_function(data=data.frame(x=c(0,3)), aes(x), fun=function(x) nrow(dat.merge)*pweibull(x, shape=shape, scale=scale), geom="line", colour="red", size=1.5)

#pdf('name')
print( p )
#dev.off()

dev.new()
p <- ggplot(dat.events.cd4.c, aes(x=CD4, y=..count..)) + geom_histogram()
#pdf('name')
print( p )
#dev.off()

#dev.new()
#p <- ggplot(dat.mort.c, aes(x=Years_since_infection, y=..count..)) + geom_histogram()

#pdf('name')
#print( p )
#dev.off()

#dev.new()
#p <- ggplot(dat.events.c, aes(x=Years_to_symptomatic, y=..count..)) + geom_histogram()
#pdf('name')
#print( p )
#dev.off()
