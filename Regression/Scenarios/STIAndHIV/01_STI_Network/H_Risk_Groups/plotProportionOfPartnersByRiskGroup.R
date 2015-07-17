# SUMMARY: Plot the percentage of individuals paired with a high risk partner as a function of risk group and relationship type
# INPUT: output/RelationshipStart.csv
# OUTPUT: figs/PercentPairedWithHighRisk.png


rm( list=ls( all=TRUE ) )
graphics.off()

library(reshape)
library(ggplot2)
library(plyr)
library(scales)

rel_names <- c('Transitory', 'Informal', 'Marital')
fig_dir = 'figs'
if( !file.exists(fig_dir) ) {
    dir.create(fig_dir)
}

output_dir = 'output'

start = read.csv(file.path(output_dir, "RelationshipStart.csv"), header=TRUE)
names(start)[names(start)=='Rel_type..0...transitory.1...informal.2...marital.'] = 'Rel_Type'
binAge = function(x) min(100, 2.5*floor((x-15)/2.5)+15)
start$A_age = sapply( start$A_age, binAge)
start$B_age = sapply( start$B_age, binAge)

extractRisk = function(x) sub(".*Risk-", "", x)
start$A_Risk = sapply( start$A_IndividualProperties, extractRisk)
start$B_Risk = sapply( start$B_IndividualProperties, extractRisk)

start$Count = 1
start$Rel_Name = factor(rel_names[start$Rel_Type+1], levels=rel_names)

start.m = melt(start, id=c("A_Risk", "B_Risk", "Rel_Name"), measure="Count")
start.c = cast(start.m, A_Risk + B_Risk + Rel_Name ~ variable, sum)

start.d = ddply(start.c, .(Rel_Name), transform, Total=sum(Count))
start.d$Percent = 100 * start.d$Count / start.d$Total


start.c$num_paired_with_hi <- 0
a_is_high <- start.c$A_Risk == 'HIGH'
start.c$num_paired_with_hi[a_is_high] <- start.c$num_paired_with_hi[a_is_high] + start.c$Count[a_is_high]
b_is_high <- start.c$B_Risk == 'HIGH'
start.c$num_paired_with_hi[b_is_high] <- start.c$num_paired_with_hi[b_is_high] + start.c$Count[b_is_high]

start.c$num_hi_with_hi <- 0
start.c$num_hi_with_hi[a_is_high & b_is_high] <- start.c$num_paired_with_hi[a_is_high & b_is_high]

start.c$num_paired_with_low <- 0
a_is_low <- start.c$A_Risk == 'LOW'
start.c$num_paired_with_low[a_is_low] <- start.c$num_paired_with_low[a_is_low] + start.c$Count[a_is_low]
b_is_low <- start.c$B_Risk == 'LOW'
start.c$num_paired_with_low[b_is_low] <- start.c$num_paired_with_low[b_is_low] + start.c$Count[b_is_low]

start.c$num_low_with_hi <- 0
start.c$num_low_with_hi[(a_is_high & !b_is_high) | (!a_is_high & b_is_high) ] <- 
  start.c$num_paired_with_low[(a_is_high & !b_is_high) | (!a_is_high & b_is_high) ]

start.e <- aggregate( cbind(num_paired_with_hi,num_paired_with_low,num_hi_with_hi, num_low_with_hi) ~ Rel_Name, data = start.c , FUN = sum )
start.e$pct_hi_with_hi <- start.e$num_hi_with_hi/start.e$num_paired_with_hi
start.e$pct_low_with_hi <- start.e$num_low_with_hi/start.e$num_paired_with_low
start.e$Rel_Name <- factor(start.e$Rel_Name)

start.f <- melt(start.e[c('Rel_Name','pct_hi_with_hi','pct_low_with_hi')],id.vars="Rel_Name")
start.f$variable <- revalue(start.f$variable, c("pct_hi_with_hi"="High Risk", "pct_low_with_hi"="Low Risk"))

p <- qplot(x=Rel_Name, y=value, fill=variable,
      data=start.f, geom="bar", stat="identity",
      position="dodge") +
  scale_y_continuous(labels = percent) +
  xlab( "Relationship Type" ) +
  ylab( "Percent Paired with a High Risk Partner" )

png( file.path(fig_dir,"PercentPairedWithHighRisk.png"), width=500, height=300)
print( p )
dev.off()


dev.new()
print(p)

