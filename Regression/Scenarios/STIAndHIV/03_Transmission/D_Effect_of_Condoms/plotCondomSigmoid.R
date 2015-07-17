# SUMMARY: Plot condom usage probability over time for transitory relationships
# INPUT: config.json
# OUTPUT: Figure on screen an figs/Simoid.png

rm( list=ls( all=TRUE ) )
graphics.off()

library(ggplot2)
library(jsonlite)

fig_dir = 'figs'
if( !file.exists(fig_dir) ) {
    dir.create(fig_dir)
}

P = fromJSON('config.json')$parameters

early   = P$Condom_Usage_Probability_in_Transitory_Relationships_Early
late    = P$Condom_Usage_Probability_in_Transitory_Relationships_Late
midyear = P$Condom_Usage_Probability_in_Transitory_Relationships_Midyear
rate    = P$Condom_Usage_Probability_in_Transitory_Relationships_Rate

sigmoid = function(x) {
    return( 100*(early + (late-early) / (1+exp(-rate*(x-midyear)))) )
}

p = ggplot(data=data.frame(x=0), mapping=aes(x=x)) + 
    stat_function(fun = sigmoid) + 
    xlim(midyear-10, midyear+10) +
    ylim(0,100) +
    xlab('Year') +
    ylab('Probability (%)') +
    ggtitle('Condom Usage Probability')

png( file.path(fig_dir,"Sigmoid.png"), width=600, height=200)
print( p )
dev.off()

print(p)
