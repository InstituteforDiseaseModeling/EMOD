% SUMMARY: Plot survival on ART for various age groups
% INPUT: output\ReportHIVByAgeAndGender.csv
% OUTPUT: Five figures displayed on screen


clear;clc;

%% options
actually_run_the_executable = false;
scenario_name = 'HIV_progression_cohort_2f';
config_filename = ['config_',scenario_name,'.json'];
exe_filename = 'Eradication';
output_dir = ['output\'];

%% choose baseline scenario directory and run baseline executable locally

scenario_directory = '.';
command_line_text = ...
    ['..\..\..\..\..\Eradication\x64\Release\'...
    exe_filename,...
    '.exe -C ',...
    config_filename,...
    ' -I ..\inputs -O ',output_dir];% -D local_plugins\'];


% add path to helper functions
addpath ../helper_functions_for_Matlab_plots/

if actually_run_the_executable
    run_executable_locally(scenario_directory,command_line_text);
end

%% number infected and on ART


age_gender_table = readtable([output_dir,'\ReportHIVByAgeAndGender.csv']);

by_year = grpstats(age_gender_table,{'Year'},'sum','DataVars',{'Population','Infected','On_ART'});

figure(1);clf;set(gcf,'color','w');hold on;
plot(by_year.Year, by_year.sum_Population,'b','LineWidth',4);
bar(by_year.Year, by_year.sum_Infected,'FaceColor',[1 .5 .5],'EdgeColor',[1 .5 .5]);
bar(by_year.Year, by_year.sum_On_ART,'FaceColor',[.7 1 .7],'EdgeColor',[.7 1 .7]);
xlabel('Year')
ylabel('Number of Individuals')
legend('Total Population Size','Number Infected','Number on ART')
xlim([2000 2100])

received_ART_at_age_21 = by_year.sum_On_ART(find(by_year.Year>2021,1,'first'));
received_ART_at_age_30 = by_year.sum_On_ART(find(by_year.Year>2030,1,'first'));
received_ART_at_age_51 = by_year.sum_On_ART(find(by_year.Year>2051,1,'first'));


%% survival time on ART
% event sequence:  born..........infected.....ART.....death

HIVMortality = readtable([output_dir,'HIVMortality.csv']);

if false
    received_ART_1_yr_post_infection = ...
        -1*HIVMortality.Years_since_first_ART_initiation + ...
        HIVMortality.Years_since_infection >=0 & ...
        -1*HIVMortality.Years_since_first_ART_initiation + ...
        HIVMortality.Years_since_infection<9;
    received_ART_10_yrs_post_infection = ...
        HIVMortality.Years_since_first_ART_initiation>9.5 & ...
        HIVMortality.Years_since_first_ART_initiation<10.5;
    age_at_infection =  HIVMortality.Death_time/365 - HIVMortality.Years_since_infection;
    was_infected_at_age_20 = age_at_infection>19.5 & age_at_infection<20.5;
    was_infected_at_age_50 = age_at_infection>49.5 & age_at_infection<50.5;
    
    male_age20_ART1yrPostInfection = ever_received_ART & was_infected_at_age_20 & ...
        received_ART_1_yr_post_infection & ~is_female;
    female_age20_ART1yrPostInfection = ever_received_ART & was_infected_at_age_20 & ...
        received_ART_1_yr_post_infection & is_female;
    
    male_age50_ART1yrPostInfection = ever_received_ART & was_infected_at_age_50 & ...
        received_ART_1_yr_post_infection & ~is_female;
    female_age50_ART1yrPostInfection = ever_received_ART & was_infected_at_age_50 & ...
        received_ART_1_yr_post_infection & is_female;
    
    male_age20_ART10yrPostInfection = ever_received_ART & was_infected_at_age_20 & ...
        received_ART_10_yrs_post_infection & ~is_female;
    female_age20_ART10yrPostInfection = ever_received_ART & was_infected_at_age_20 & ...
        received_ART_10_yrs_post_infection & is_female;
end

% instead, I'll calculate it this way, since I happen to know people are
% all age zero on day zero:

is_female = HIVMortality.Gender;
ever_received_ART = HIVMortality.Ever_in_ART == 1;

died_after_receiving_ART_at_age_21 = ever_received_ART & ...
    (HIVMortality.Death_time/365 - HIVMortality.Years_since_first_ART_initiation)>20.5 & ...
    (HIVMortality.Death_time/365 - HIVMortality.Years_since_first_ART_initiation)<21.5;
died_after_receiving_ART_at_age_30 = ever_received_ART & ...
    (HIVMortality.Death_time/365 - HIVMortality.Years_since_first_ART_initiation)>29.5 & ...
    (HIVMortality.Death_time/365 - HIVMortality.Years_since_first_ART_initiation)<30.5;
died_after_receiving_ART_at_age_51 = ever_received_ART & ...
    (HIVMortality.Death_time/365 - HIVMortality.Years_since_first_ART_initiation)>50.5 & ...
    (HIVMortality.Death_time/365 - HIVMortality.Years_since_first_ART_initiation)<51.5;

%% 20 yo vs. 50 yo
figure(2);clf;set(gcf,'color','w');hold on;
lw=2;
plot(HIVMortality.Death_time/365-21, cumsum(died_after_receiving_ART_at_age_21)/received_ART_at_age_21,'linewidth',lw,'color',[.4 0 .5],'linestyle','-')
plot(HIVMortality.Death_time/365-51, cumsum( died_after_receiving_ART_at_age_51)/received_ART_at_age_51,'linewidth',lw,'color',[.4 .5 .5],'linestyle','-')

xlim([0 50])
legend('Infected at age 20, ART 1 year after infection',...
    'Infected at age 50, ART 1 year after infection')

xlabel ('Time since ART initiation (years)')
ylabel('Proportion died since starting ART')


%% 1 yr vs. 10 yr after infection
figure(3);clf;set(gcf,'color','w');hold on;
lw=2;
plot(HIVMortality.Death_time/365-21, cumsum(died_after_receiving_ART_at_age_21)/received_ART_at_age_21,'linewidth',lw,'color',[.4 0 .5],'linestyle','-')
plot(HIVMortality.Death_time/365-30, cumsum( died_after_receiving_ART_at_age_30)/received_ART_at_age_30,'linewidth',lw,'color',[.4 .5 .5],'linestyle','-')

xlim([0 50])
legend('Infected at age 20, ART 1 year after infection',...
    'Infected at age 20, ART 10 years after infection')

xlabel ('Time since ART initiation (years)')
ylabel('Proportion died since starting ART')



%% 1 yr vs. 10 yr after infection
figure(4);clf;set(gcf,'color','w');hold on;
lw=2;
plot(HIVMortality.Death_time/365-21, cumsum(~is_female & died_after_receiving_ART_at_age_21)/received_ART_at_age_21,'linewidth',lw,'color',[.4 .6 .5],'linestyle','--')
plot(HIVMortality.Death_time/365-21, cumsum(is_female & died_after_receiving_ART_at_age_21)/received_ART_at_age_21,'linewidth',lw,'color',[.4 .5 .5],'linestyle','-')

xlim([0 50])
legend('Male, infected at age 20, ART 1 yr after infection',...
    'Female, infected at age 20, ART 1 yr after infection')

xlabel ('Time since ART initiation (years)')
ylabel('Proportion died since starting ART')


%% all groups 

figure(5);clf;set(gcf,'color','w');hold on;
lw=2;
plot(HIVMortality.Death_time/365-21, cumsum(~is_female & died_after_receiving_ART_at_age_21)/received_ART_at_age_21,'linewidth',lw,'color',[.4 0 .5],'linestyle','--')
plot(HIVMortality.Death_time/365-21, cumsum(is_female & died_after_receiving_ART_at_age_21)/received_ART_at_age_21,'linewidth',lw,'color',[.4 0 .5],'linestyle','-')

plot(HIVMortality.Death_time/365-30, cumsum(~is_female & died_after_receiving_ART_at_age_30)/received_ART_at_age_30,'linewidth',lw,'color',[.4 .5 .5],'linestyle','--')
plot(HIVMortality.Death_time/365-30, cumsum(is_female & died_after_receiving_ART_at_age_30)/received_ART_at_age_30,'linewidth',lw,'color',[.4 .5 .5],'linestyle','-')

plot(HIVMortality.Death_time/365-51, cumsum(~is_female & died_after_receiving_ART_at_age_51)/received_ART_at_age_51,'linewidth',lw,'color',[.3 .9 .2],'linestyle','--')
plot(HIVMortality.Death_time/365-51, cumsum(is_female & died_after_receiving_ART_at_age_51)/received_ART_at_age_51,'linewidth',lw,'color',[.3 .9 .2],'linestyle','-')

xlim([0 50])
legend('Male, age 20, ART 1 year after infection',...
    'Female, age 20, ART 1 year after infection',...
    'Male, age 20, ART 10 years after infection',...
    'Female, age 20, ART 10 years after infection',...
    'Male, age 50, ART 1 year after infection',...
    'Female, age 50, ART 1 year after infection')

xlabel ('Time since ART initiation (years)')
ylabel('Proportion died since starting ART')


%% remove path to helper functions

rmpath ../helper_functions_for_Matlab_plots/


