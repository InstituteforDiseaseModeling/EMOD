# SUMMARY: Plot HIV prevalence, incidence rate, and mortality rate for ages 15-49 by gender
# INPUT ARGUMENTS: (1) Path to ReportHIVByAgeAndGender.csv
# OUTPUT: figs/HIV_Summary.png

library(reshape)
library(ggplot2)
library(gridExtra)

rm( list=ls( all=TRUE ) )
graphics.off()

args = commandArgs()

idx = which( args == '--args' )
if( length(idx) == 0 || length(args) < idx + 1) {
    stop('USAGE: Rscript plotHIVSummary_15_49.R ReportHIVByAgeAndGender.csv')
}
fn = args[idx+1]
dat = read.csv(fn)

fig_dir = 'figs'
if( !file.exists(fig_dir) ) {
    dir.create(fig_dir)
}

prevalent.m = melt(dat, 
    id=c("Year", "Gender", "Age"),
    measure=c("Population", "Infected", "On_ART"))

incident.m = melt(dat, 
    id=c("Year", "Gender", "Age"),
    measure=c("Died", "Died_from_HIV", "Newly.Infected"))

Years = unique(prevalent.m$Year)
nYears = 2*floor(length(Years)/2)   # Make it even
if( nYears == 0) {
    stop('Did not find enough years, please run for a longer period of time.');
}

midYears = Years[seq(0,nYears-1) %% 2 == 0]   # First is mid-year

# midYear function for stitching half years together
midYear = function(year) {
    if( any(midYears == year) ) {
        return(year)
    }

    idx = which(year == Years)
    stopifnot(idx > 0)
    return( Years[[idx-1]] )
}

prevalent.m = prevalent.m[prevalent.m$Year %in% midYears,]
prevalent.c = cast(prevalent.m, Year + Gender~ variable, sum, subset=(Age>=15 & Age<50))

incident.m$Year = sapply(incident.m$Year, midYear)
incident.c = cast(incident.m, Year + Gender ~ variable, sum, subset=(Age>=15 & Age<50))

both.c = merge(prevalent.c, incident.c, by=c("Year", "Gender"))
both.c$Prevalence = 100 * both.c$Infected / both.c$Population
both.c$IncidenceRate = both.c$Newly.Infected / (both.c$Population - both.c$Infected)
both.c$HIVCauseMortalityRate = both.c$Died_from_HIV / both.c$Population
both.c$ARTCoverage = 100 * both.c$On_ART / both.c$Infected
both.c$Gender = factor(both.c$Gender, labels=c("Male", "Female"))

# PLOTTING
p.prevalence = ggplot(both.c, aes(x=Year, y=Prevalence, colour=Gender)) +
    geom_line() +
    scale_color_manual(values=c("darkblue", "darkred")) +
    theme(axis.text.x = element_text(angle = 45, hjust = 1)) +
    theme(legend.position=c(0.75,0.9)) +
    theme(legend.title=element_blank()) +
    xlab( "Year" ) +
    ylab( "Prevalence 15-49 (%)" ) +
    ggtitle( "Prevalence" )

p.incidence = ggplot(both.c, aes(x=Year, y=IncidenceRate, colour=Gender)) +
    geom_line() +
    scale_color_manual(values=c("darkblue", "darkred")) +
    theme(axis.text.x = element_text(angle = 45, hjust = 1)) +
    theme(legend.position="none") +
    theme(legend.title=element_blank()) +
    xlab( "Year" ) +
    ylab( "Incidence Rate 15-49 (Infections/Susceptible-PY)" ) +
    ggtitle( "Incidence" )

p.deaths = ggplot(both.c, aes(x=Year, y=HIVCauseMortalityRate, colour=Gender)) +
    geom_line() +
    scale_color_manual(values=c("darkblue", "darkred")) +
    theme(axis.text.x = element_text(angle = 45, hjust = 1)) +
    theme(legend.position="none") +
    theme(legend.title=element_blank()) +
    xlab( "Year" ) +
    ylab( "HIV-Cause Mortality Rate 15-49 (Deaths/PY)" ) +
    ggtitle( "Mortality" )

p.ART = ggplot(both.c, aes(x=Year, y=ARTCoverage, colour=Gender)) +
    geom_line() +
    scale_color_manual(values=c("darkblue", "darkred")) +
    theme(axis.text.x = element_text(angle = 45, hjust = 1)) +
    theme(legend.position="none") +
    theme(legend.title=element_blank()) +
    xlab( "Year" ) +
    ylab( "Antiretroviral Therapy Coverage (%)" ) +
    ggtitle( "ART" )

p = arrangeGrob(p.prevalence, p.incidence, p.deaths, p.ART, ncol=4)

outfile = file.path(fig_dir,"HIV_Summary.png")
print(paste('Saving figure to', outfile))
png( outfile, width=800, height=400)
print( p )
dev.off()
