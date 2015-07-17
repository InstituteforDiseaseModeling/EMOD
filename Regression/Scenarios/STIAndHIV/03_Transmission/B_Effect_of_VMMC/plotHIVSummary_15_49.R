# SUMMARY: Plot HIV prevalence, incidence rate, and mortality rate for ages 15-49 by gender
# INPUT: output/ReportHIVByAgeAndGender.csv
# OUTPUT: figs/HIV_Summary.png

rm( list=ls( all=TRUE ) )
graphics.off()

library(reshape)
library(ggplot2)
library(gridExtra)

VMMC_YEAR = 2025

rel_names <- c('Transitory', 'Informal', 'Marital')
fig_dir = 'figs'
if( !file.exists(fig_dir) ) {
    dir.create(fig_dir)
}

dat = read.csv(file.path("output", "ReportHIVByAgeAndGender.csv"))

prevalent.m = melt(dat, 
    id=c("Year", "Gender", "Age"),
    measure=c("Population", "Infected", "On_ART"))

incident.m = melt(dat, 
    id=c("Year", "Gender", "Age"),
    measure=c("Died", "Died_from_HIV", "Newly.Infected"))

Years = unique(prevalent.m$Year)
nYears = 2*floor(length(Years)/2)   # Make it even
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
both.c$IncidenceRate[ is.nan(both.c$IncidenceRate) ] = 0
both.c$HIVCauseMortalityRate = both.c$Died_from_HIV / both.c$Population
both.c$Gender = factor(both.c$Gender, labels=c("Male", "Female"))

p.prevalence = ggplot(both.c, aes(x=Year, y=Prevalence, colour=Gender)) +
    geom_line() +
    geom_vline(xintercept=VMMC_YEAR, colour="black", linetype="dashed", linewidth=2) + 
    scale_color_manual(values=c("darkblue", "darkred")) +
    theme(axis.text.x = element_text(angle = 45, hjust = 1)) +
    theme(legend.position=c(0.7,0.1)) +
    theme(legend.title=element_blank()) +
    xlab( "Year" ) +
    ylab( "Prevalence 15-49 (%)" ) +
    ggtitle( "Prevalence" )

p.incidence = ggplot(both.c, aes(x=Year, y=IncidenceRate, colour=Gender)) +
    geom_line() +
    geom_vline(xintercept=VMMC_YEAR, colour="black", linetype="dashed", linewidth=2) + 
    scale_color_manual(values=c("darkblue", "darkred")) +
    theme(axis.text.x = element_text(angle = 45, hjust = 1)) +
    theme(legend.position="none") +
    theme(legend.title=element_blank()) +
    xlab( "Year" ) +
    ylab( "Incidence Rate 15-49 (Infections/Susceptible-PY)" ) +
    ggtitle( "Incidence" )

p.deaths = ggplot(both.c, aes(x=Year, y=HIVCauseMortalityRate, colour=Gender)) +
    geom_line() +
    geom_vline(xintercept=VMMC_YEAR, colour="black", linetype="dashed", linewidth=2) + 
    scale_color_manual(values=c("darkblue", "darkred")) +
    theme(axis.text.x = element_text(angle = 45, hjust = 1)) +
    theme(legend.position="none") +
    theme(legend.title=element_blank()) +
    xlab( "Year" ) +
    ylab( "HIV-Cause Mortality Rate 15-49 (Deaths/PY)" ) +
    ggtitle( "Mortality" )

p = arrangeGrob(p.prevalence, p.incidence, p.deaths, ncol=3)

png( file.path(fig_dir,"HIV_Summary.png"), width=600, height=400)
print( p )
dev.off()

print(p)
