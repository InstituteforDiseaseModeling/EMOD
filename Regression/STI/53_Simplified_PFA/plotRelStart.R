#rm( list=ls( all=TRUE ) )
graphics.off()

library(reshape)
library(ggplot2)

rel_names <- c('Transitory', 'Informal', 'Marital')
fig_dir = 'figs'

dat <- read.csv("output/RelationshipStart.csv", header=TRUE)
dat$Count = 1
dat$A_age_floor = floor(dat$A_age)
dat$B_age_floor = floor(dat$B_age)
dat$Rel_type = dat$Rel_type..0...transitory.1...informal.2...marital.

dat.m = melt(dat, id=c('Rel_type','A_age_floor', 'B_age_floor'), measure='Count')

dir.create(fig_dir)

for(rel_type in 0:2)
{
    rel_name = rel_names[rel_type+1]
    dat.c = cast(dat.m, A_age_floor + B_age_floor ~ variable, sum, subset=(Rel_type == rel_type))

    ### Plot ###
    p <- ggplot(dat.c, aes(x=A_age_floor, y=B_age_floor, fill=Count)) + geom_tile() +
        ggtitle( rel_name )

    #dev.new()
    pdf( file.path(fig_dir,paste('RelationshipsFormed_',rel_name, '.pdf',sep="")))
    print( p )
    dev.off()
}

