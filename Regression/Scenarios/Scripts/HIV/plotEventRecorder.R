# SUMMARY: Plot events that occur during a simulation, disaggregated by gender.  Note that common events, e.g. ARTStaging0 and ARTStaging1 will be displayed in separate rows in a common column with heading ARTStaging.
# INPUT ARGUMENTS: (1) Path to ReportEventRecorder.csv
# OUTPUT: figs/EventRecorder.png

library(reshape)
library(ggplot2)

rm( list=ls( all=TRUE ) )
graphics.off()
fig_dir = 'figs'
if( !file.exists(fig_dir) ) {
    dir.create(fig_dir)
}

args = commandArgs()

idx = which( args == '--args' )
if( length(idx) == 0 || length(args) < idx + 1) {
    stop('USAGE: Rscript plotEventRecorder ReportEventRecorder.csv')
}
fn = args[idx+1]

data <- read.csv(fn, header=TRUE)
data$Count <- 1

data.melt <- melt( data, id = c('Year', 'Event_Name', 'Gender'), measure='Count' )   # Type
data.cast <- cast(data.melt, Year + Gender ~ Event_Name, sum) 
data.melt.cast <- cast(data.melt, Year+Event_Name+Gender ~ value, sum)
data.melt.cast$Group <- sapply(data.melt.cast$Event_Name,gsub,pattern="\\d",replacement="")
data.melt.cast$Index <- sapply(data.melt.cast$Event_Name,gsub,pattern="\\D",replacement="")
colnames(data.melt.cast) <- c("Year", "Event_Name", "Gender", "Count", "Group", "Index")

p <- ggplot(data.melt.cast, aes(x=Year, y=Count, colour=Gender)) +
  geom_point(size=1) +
  theme( legend.position="bottom", legend.direction = "horizontal", legend.box="vertical" ) +
  guides(col = guide_legend(nrow = 8)) +
  theme(legend.title=element_blank()) +
  scale_y_continuous(trans = 'log10') +
  scale_colour_discrete(breaks=c('M', 'F'), labels=c('Male', 'Female')) +
  facet_grid(Index ~ Group) +
  theme(strip.text.x = element_text(size=10, angle=90)) +
  theme(axis.text.x = element_text(size=8, angle = 90, hjust = 1))


outfile = file.path(fig_dir,"EventRecorder.png")
print(paste('Saving figure to', outfile))
png( outfile, width=8, height=8, units='in', res=300)
print(p)
dev.off()
