# SUMMARY: Demonstrate coital dilution by plotting the a heat map of the number of coital acts between each pair of individuals.  You should see lots of coital acts (red) when there is no concurrency, and few coital acts (blue) whe
# INPUT: output/RelationshipConsummated.csv
# OUTPUT: figs/RelationshipsConsummated.png

rm( list=ls( all=TRUE ) )
graphics.off()

library(reshape)
library(ggplot2)

DAYS_PER_YEAR = 365

rel_names <- c('Transitory', 'Informal', 'Marital')
fig_dir = 'figs'
if( !file.exists(fig_dir) ) {
    dir.create(fig_dir)
}

output_dir = 'output'

rels = read.csv(file.path(output_dir, "RelationshipConsummated.csv"))
rels$Count = 1
rels$A_ID = factor(rels$A_ID)
rels$B_ID = factor(rels$B_ID)

rels.m = melt(rels, id=c("A_ID", "B_ID"), measure="Count")
rels.c = cast(rels.m, A_ID + B_ID ~ variable, sum)

p <- ggplot(rels.c, aes(x=A_ID, y=B_ID, fill=Count)) +
    geom_raster() +
    xlab( "Male ID" ) +
    ylab( "Female ID" ) +
    ggtitle( "Relationships Consummated" ) +
    scale_fill_gradient2(low="blue", midpoint=mean(rels.c$Count), high="red", name="Coital\nActs") +
    coord_fixed(ratio = 1)

png( file.path(fig_dir,"RelationshipsConsummated.png"), width=600, height=500)
print( p )
dev.off()

dev.new()
print(p)
