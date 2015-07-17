# SUMMARY: Plot the population by age group
# INPUT ARGUMENTS: (1) Path to BinnedReport.json
# OUTPUT: figs/PopulationByAge.png

rm( list=ls( all=TRUE ) )
graphics.off()

library(jsonlite)
library(reshape)
library(ggplot2)

fig_dir = 'figs'
if( !file.exists(fig_dir) ) {
    dir.create(fig_dir)
}

args = commandArgs()

idx = which( args == '--args' )
if( length(idx) == 0 || length(args) < idx + 1) {
    stop('USAGE: Rscript plotBinnedPopulation.R BinnedReport.json')
}
fn = args[idx+1]
J = fromJSON(fn)

mat = t(J$Channels$Population$Data)
colnames(mat) = J$Header$Subchannel_Metadata$MeaningPerAxis
m = melt(mat)

p = ggplot( m, aes(x=X1, y=value, colour=factor(X2), group=factor(X2)) ) + 
    geom_line() +
    scale_colour_discrete(breaks=J$Header$Subchannel_Metadata$MeaningPerAxis, name="Age Group") +
    xlab( "Simulation Day" ) +
    ylab( "Population" ) +
    ggtitle( "Population by Age" )

outfile = file.path(fig_dir,"PopulationByAge.png")
print(paste('Saving figure to', outfile))
png( outfile, width=640, height=480)
print(p)
dev.off()
