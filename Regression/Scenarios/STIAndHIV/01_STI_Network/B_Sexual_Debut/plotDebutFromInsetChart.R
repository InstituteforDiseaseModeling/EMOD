# SUMMARY: Demonstrate sexual debut by comparing the theoretic Weibull distribution to data from the InsetChart.  Also plot the age of entering the first relationship, which should lag debut.
# INPUT: 
#   1. config.json
#   2. output/RelationshipStart.csv
#   2. output/InsetChart.json
# OUTPUT: figs/PostDebut.png


rm( list=ls( all=TRUE ) )
graphics.off()

library(reshape)
library(ggplot2)
library(jsonlite)

DAYS_PER_YEAR = 365
fig_dir = 'figs'
if( !file.exists(fig_dir) ) {
    dir.create(fig_dir)
}

configJ = fromJSON('config.json')$parameters
insetJ = fromJSON(file.path('output', 'InsetChart.json'))
relstart = read.csv(file.path('output', 'RelationshipStart.csv'), header=TRUE)


debut.female.het = configJ[["Sexual_Debut_Age_Female_Weibull_Heterogeneity"]]
debut.female.scale = configJ[["Sexual_Debut_Age_Female_Weibull_Scale"]]
debut.male.het = configJ[["Sexual_Debut_Age_Male_Weibull_Heterogeneity"]]
debut.male.scale = configJ[["Sexual_Debut_Age_Male_Weibull_Scale"]]
debut.age.min = configJ[["Sexual_Debut_Age_Min"]]

stopifnot( debut.female.scale == debut.male.scale )
stopifnot( debut.female.het == debut.male.het )
stopifnot( debut.female.het > 0 )

scale = debut.female.scale
shape = 1/debut.female.het

num_debuts = tail(insetJ$Channels[['Post-Debut Population']]$Data, n=1)
debut_weibull = function(x) {
    y = vector(, length(x));
    for( i in seq(1, length(x))) {
        xx = x[i]
        if( xx <= debut.age.min ) {
            y[i] = 0
        } else {
            y[i] = num_debuts/pweibull(insetJ$Header$Timesteps/DAYS_PER_YEAR, shape, scale) * pweibull(xx, shape, scale)
        }
    }
    return(y)
}

id.A = unique(relstart$A_ID)
id.B = unique(relstart$B_ID)
rel.d <- data.frame(Age = numeric(0), RelationshipStart = integer(0))
for( id in id.A) {
    r = min(which( relstart$A_ID == id))
    rel.d = rbind(rel.d, data.frame(Age = relstart[r,]$A_age, RelationshipStart = 1))
}
for( id in id.B) {
    r = min(which( relstart$B_ID == id))
    rel.d = rbind(rel.d, data.frame(Age = relstart[r,]$B_age, RelationshipStart = 1))
}

rel.m = melt(rel.d, id.vars = 'Age', variable_name='Source')
rel.m = rel.m[order(rel.m$Age),]    # Sort
rel.m$value = cumsum(rel.m$value)

timesteps = insetJ$Header$Timesteps
sim_tstep = insetJ$Header$Simulation_Timestep
time = 1:sim_tstep:timesteps
post_debut = data.frame( time/DAYS_PER_YEAR, insetJ$Channels[['Post-Debut Population']]$Data, debut_weibull(time/DAYS_PER_YEAR) )
colnames(post_debut) = c("Age", "InsetChart", "Weibull")
d.m = melt(post_debut, id.vars = 'Age', variable_name = 'Source')

all.m = rbind( d.m, rel.m)

p <- ggplot(all.m, aes(x=Age, y=value, colour=Source)) +
    geom_line(size=2) +
    scale_x_continuous( limits = c(12.5,20) ) +
    ylab('Number Post Debut') +
    scale_colour_discrete(breaks=c("Weibull","InsetChart","RelationshipStart")) +
    theme(legend.position="bottom")

png(file.path(fig_dir, 'PostDebut.png'), width=600, height=400)
print( p )
dev.off()

print( p )
