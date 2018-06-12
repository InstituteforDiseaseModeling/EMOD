/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#define VECTOR_AGENTS_TRACK_ALL (1)
#define VECTOR_AGENTS_SAMPLED   (2)

#define MICROLITERS_PER_BLOODMEAL (2.0)
#define LARVAL_CONSTANT_FACTOR    (22675736961.5)
#define ONE_POINT_TWO             (1.2)
#define MM_PER_METER              (1000.0)

#define CELSIUS_TO_KELVIN   (273.15)

// these parameters govern the evaporation of water based on local humidity and temperature
// details and equations in vector model manuscript

/*
** Temporary habitat Htemp in a grid of diameter Dcell increases with rainfall Prain
** and decays with a rate TAUtemp proportional to the evaporation rate driven by
** temperature Tk (Kelvin) and humidity RH:
**
** Htemp += Prain * Ktemp * Dcell^2 - Htemp * (deltaT / TAUtemp)
**
** 1/TAUtemp = (5.1e10^11Pa) * exp(-5628.1/Tk) * ktempdecay * sqrt(0.018kg/mol / (2 * pi * R * Tk)) * (1 - RH)
** where R is the ideal gas constant
**
** Thus,
** FACTOR1 = -5628.1    // multiplicative factor divided by temperature in exponential in C-C calculation of saturated vapor pressure
** FACTOR2 = 5.1e11     // integration constant in C-C calculation of saturated vapor pressure
** FACTOR3 = (0.018 / (2 * pi * R)) = 0.00034457    // terms based on molecular weight of water to get evaporation rate
*/

#define LARVAL_HABITAT_FACTOR1  (-5628.1)
#define LARVAL_HABITAT_FACTOR2  (5.1E11)
#define LARVAL_HABITAT_FACTOR3  (0.00034457)

#define DEFAULT_VECTOR_POPULATION_SIZE  (10000)
#define DEFAULT_VECTOR_COHORT_SIZE      (100)

#define DEFAULT_AQUATIC_ARRHENIUS1              (1500000.0)
#define DEFAULT_AQUATIC_ARRHENIUS2              (5000.0)
#define DEFAULT_INFECTED_ARRHENIUS1             (6500.0)
#define DEFAULT_INFECTED_ARRHENIUS2             (3500.0)
#define DEFAULT_CYCLE_ARRHENIUS1                (4.090579E10f)
#define DEFAULT_CYCLE_ARRHENIUS2                (7.740230E3f)
#define DEFAULT_IMMATURE_DURATION               (4.0)
#define DEFAULT_DAYS_BETWEEN_FEEDS              (3.0)
#define DEFAULT_ANTHROPOPHILY                   (0.9f)
#define DEFAULT_EGGBATCH_SIZE                   (100.0)
#define DEFAULT_INFECTED_EGGBATCH_MODIFIER      (0.8f)
#define DEFAULT_EGG_SURVIVAL_RATE               (0.99f) 
#define DEFAULT_INFECTIOUS_MORTALITY_MODIFIER   (1.5)
#define DEFAULT_AQUATIC_MORTALITY_RATE          (0.2f)
#define DEFAULT_ADULT_LIFE_EXPECTANCY           (21.0)
#define DEFAULT_TRANSMISSION_MODIFIER           (0.02f)
#define DEFAULT_ACQUIRE_MODIFIER                (0.5)
#define DEFAULT_HUMAN_FEEDING_MORTALITY         (0.1f)
#define DEFAULT_INFECTIOUS_HUMAN_FEEDING_MORTALITY_MODIFIER (1.5)
