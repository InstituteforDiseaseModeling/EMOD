# SUMMARY: Plot the male-to-female relative infectivity as a function of female age
# INPUT: confg.json
# OUTPUT: Figure appears on screen if running in an interactive R session.  Call from Rscript to generate a pdf.

rm( list=ls( all=TRUE ) )
graphics.off()

# before running this for the first time, you will need to install the package jsonlite as follows:
# install.packages("jsonlite", repos="http://cran.r-project.org")

library(jsonlite)

configJ = fromJSON('config.json')$parameters
rate <- configJ$Base_Infectivity
x <- configJ$Male_To_Female_Relative_Infectivity_Ages
y <- configJ$Male_To_Female_Relative_Infectivity_Multipliers

x_linear <- seq(0,60,0.1)
y_linear <-  approx(x, y, x_linear, method="linear",  rule = 2, yleft=y[1], yright=tail(y,1))

plot(x_linear,y_linear$y,type="l",xlim=c(0,60),ylim=c(0,6),col=1, axes=FALSE, ann=FALSE)
par(new=T)
plot(x,y,type="p",xlim=c(0,60),ylim=c(0,6),col=2,axes=FALSE,ann=FALSE)

box()
axis(1, at=seq(0,60,5))
axis(2,at=seq(0,5,.5))
title(main="Male to Female Transmission Multiplier", col.main="black", font.main=1)

title(xlab="Female Age", col.lab='black')
title(ylab="Transmission Multiplier", col.lab='black')
