# SUMMARY: Plot partnership age gaps as a function of risk group and relationship type
# INPUT: output/RelationshipStart.csv
# OUTPUT: figs/PartnerChoiceByAgeAndType.png


rm( list=ls( all=TRUE ) )
graphics.off()

library(reshape)
library(ggplot2)

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

start.m = melt(start, id=c("A_age", "B_age", "A_Risk", "B_Risk", "Rel_Name", "Rel_start_time"), measure="Count")
start.c = cast(start.m, A_age + B_age + A_Risk + B_Risk + Rel_Name ~ variable, sum) #, subset=(Rel_start_time>=2000)

p <- ggplot(start.c, aes(x=A_age, y=B_age, fill=Count)) +
    geom_raster() +
    xlab( "Male Age" ) +
    ylab( "Female Age" ) +
    ggtitle( "Relationships Formed" ) +
    facet_grid( Rel_Name ~ A_Risk+B_Risk) +
    scale_fill_gradientn(colours=rev(rainbow(4)))+
    geom_abline(slope=1, intercept=0, colour="red") +
    coord_fixed(ratio = 1, xlim=c(0,100)) # , xlim = NULL, ylim = NULL, wise = NULL

png( file.path(fig_dir,"PartnerChoiceByAgeAndType.png"), width=600, height=400)
print( p )
dev.off()


dev.new()
print(p)
