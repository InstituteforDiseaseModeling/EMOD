# SUMMARY: Plot condom usage probability over time for transitory relationships
# INPUT: condoms_overlay.json
# OUTPUT: Figure on screen an figs/Simoid.png

rm( list=ls( all=TRUE ) )
graphics.off()

library(ggplot2)
library(jsonlite)

fig_dir = 'figs'
if( !file.exists(fig_dir) ) {
    dir.create(fig_dir)
}

P = fromJSON('condoms_overlay.json')

print( P$Defaults$Society$TRANSITORY$Relationship_Parameters$Condom_Usage_Probability )
early   = P$Defaults$Society$TRANSITORY$Relationship_Parameters$Condom_Usage_Probability$Min
late    = P$Defaults$Society$TRANSITORY$Relationship_Parameters$Condom_Usage_Probability$Max
midyear = P$Defaults$Society$TRANSITORY$Relationship_Parameters$Condom_Usage_Probability$Mid
rate    = P$Defaults$Society$TRANSITORY$Relationship_Parameters$Condom_Usage_Probability$Rate

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
