# SUMMARY: Plot several key network statistics from the inset chart
# INPUT: output/InsetChart.json
# OUTPUT: figs/RelationshipStats.png


rm( list=ls( all=TRUE ) )
graphics.off()

library(reshape)
library(ggplot2)
library(jsonlite)
library(RColorBrewer)

DAYS_PER_YEAR = 365.0
fig_dir = 'figs'
if( !file.exists(fig_dir) ) {
    dir.create(fig_dir)
}


parse_json = function(dir, Scenario) {
    insetJ= fromJSON(file.path(dir, 'output', 'InsetChart.json'))


    data = data.frame( colname = factor(), Count = double() )
    data = rbind( data, data.frame(colname='Paired People', Count = insetJ$Channels[['Paired People']]$Data ) )
    data = rbind( data, data.frame(colname='Single Post-Debut Men', Count = insetJ$Channels[['Single Post-Debut Men']]$Data ) )
    data = rbind( data, data.frame(colname='Single Post-Debut Women', Count = insetJ$Channels[['Single Post-Debut Women']]$Data ) )
    data = rbind( data, data.frame(colname='Active Transitory Relationships', Count = insetJ$Channels[['Active Transitory Relationships']]$Data ) )
    data = rbind( data, data.frame(colname='Active Informal Relationships', Count = insetJ$Channels[['Active Informal Relationships']]$Data ) )
    data = rbind( data, data.frame(colname='Active Marital Relationships', Count = insetJ$Channels[['Active Marital Relationships']]$Data ) )


    timesteps = insetJ$Header$Timesteps
    sim_tstep = insetJ$Header$Simulation_Timestep
    data$Simulation_Year = (sim_tstep/DAYS_PER_YEAR) * (1:timesteps)

    data$Scenario = Scenario

    return( data )
}

data.rr1 = parse_json('RateRatio1', 'RateRatio=1')
data.rr10 = parse_json('RateRatio10', 'RateRatio=10')
data = rbind( data.rr1, data.rr10 )


p <- ggplot(data, aes(x=Simulation_Year, y=Count, colour=Scenario)) +
    geom_line(size=2) +
    facet_wrap( ~ colname, ncol=3) +
    xlab('Simulation Year') +
    theme(legend.position="bottom", legend.title=element_blank())

png(file.path(fig_dir, 'RelationshipStats.png'), width=600, height=400)
print( p )
dev.off()

print( p )
