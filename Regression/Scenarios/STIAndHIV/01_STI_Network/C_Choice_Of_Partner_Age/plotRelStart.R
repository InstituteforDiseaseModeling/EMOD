# SUMMARY: Plot age gaps of partnerships formed for each relationship type
# INPUT: output/RelationshipStart.csv
# OUTPUT:
#   1. figs/RelationshipsFormed_Transitory.png
#   2. figs/RelationshipsFormed_Informal.png
#   3. figs/RelationshipsFormed_Marital.png
#   ** Note that figures are only created when at least one relationship of the corresponding type is observed

rm( list=ls( all=TRUE ) )
graphics.off()

library(reshape)
library(ggplot2)

rel_names <- c('Transitory', 'Informal', 'Marital')
fig_dir = 'figs'
if( !file.exists(fig_dir) ) {
    dir.create(fig_dir)
}

dat <- read.csv("output/RelationshipStart.csv", header=TRUE)
dat$Count = 1

ageBin = function(age) {
    if( age < 40 )
        return("<40")
    return("40+")
}

dat$A_agebin = sapply(dat$A_age, ageBin)
dat$B_agebin = sapply(dat$B_age, ageBin)
dat$Rel_type = dat$Rel_type..0...transitory.1...informal.2...marital.

dat.m = melt(dat, id=c('Rel_type','A_agebin', 'B_agebin', 'A_age', 'B_age'), measure='Count')

if( !file.exists(fig_dir) ) {
    dir.create(fig_dir)
}

for(rel_type in 0:2)
{
    rel_name = rel_names[rel_type+1]

    if( length(which(dat.m$Rel_type == rel_type)) == 0 ) {
        next
    }

    dat.c = cast(dat.m, A_agebin + B_agebin ~ variable, sum, subset=(Rel_type == rel_type))
    tot = sum(dat.c$Count)
    dat.c$Percent = dat.c$Count / tot

    m = min( rbind(dat.m$A_age, dat.m$B_age) )

    text_color = "black"

    ### Plot ###
    p <- ggplot() + 
        #geom_tile(data=dat.c, aes(x=A_agebin, y=B_agebin, fill=Percent)) +
        #stat_bin2d(data=dat.m, aes(x=A_age, y=B_age), breaks=list(x=c(m,40,55),y=c(m,40,55)) ) +
        geom_point(data=dat.m, aes(x=A_age, y=B_age), color="orange", size=1) +
        #scale_fill_continuous(name="Relationships\nFormed") +
        coord_fixed() +
        annotate("text", x=25, y=25, label=paste(round(100*dat.c[dat.c$A_agebin=="<40" & dat.c$B_agebin=="<40",]$Percent), '%',sep=''), color=text_color, size=10) +
        annotate("text", x=50, y=25, label=paste(round(100*dat.c[dat.c$A_agebin=="40+" & dat.c$B_agebin=="<40",]$Percent), '%',sep=''), color=text_color, size=10) +
        #annotate("text", x=25, y=50, label=paste(round(100*dat.c[dat.c$A_agebin=="<40" & dat.c$B_agebin=="40+",]$Percent), '%',sep=''), color=text_color, size=10) +
        annotate("text", x=50, y=50, label=paste(round(100*dat.c[dat.c$A_agebin=="40+" & dat.c$B_agebin=="40+",]$Percent), '%',sep=''), color=text_color, size=10) +
        ggtitle( rel_name ) +
        xlab( 'Male Age' ) +
        ylab( 'Female Age' ) +

    #dev.new()
    png( file.path(fig_dir,paste('RelationshipsFormed_',rel_name, '.png',sep="")), width=600, height=400)
    print( p )
    dev.off()

    dev.new()
    print(p)

}
