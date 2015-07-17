% SUMMARY: Plot survival time for adults against expected curves
% INPUT: output\HIVMortality.csv
% OUTPUT: Figure displayed on screen


clear;clc;

%% options
actually_run_the_executable = false;
scenario_name = 'HIV_progression_cohort_2c';
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


%% process output

hist_bin_width = 1;
hist_bins_years_survived = 0:hist_bin_width:100;

HIVMortality = readtable([output_dir,'HIVMortality.csv']);
age_at_infection =  HIVMortality.Death_time/365 - HIVMortality.Years_since_infection;
% this only works because I know people were initialized as newborns

ages = [20 30 40 50];
age_mins = ages - 1;
age_maxes = ages + 1;
survived_years = nan(length(hist_bins_years_survived),length(ages));

% for expected distribution:
lambda_by_age_slope = -0.2717;
lambda_by_age_intercept = 21.182;
kappa = 2;
x = 0:.1:100;
expected_distribution = nan(length(x),length(ages));

for age_bin_iterator = 1:length(ages)
    
    curr_age_min = age_mins(age_bin_iterator);
    curr_age_max = age_maxes(age_bin_iterator);
    
    is_in_desired_infection_group = ...
        age_at_infection >= curr_age_min & age_at_infection <= curr_age_max;

    survived_years(:,age_bin_iterator) = ...
        hist(HIVMortality.Years_since_infection(is_in_desired_infection_group),...
        hist_bins_years_survived);
    
    % for expected distribution:
    number_of_observations = sum(survived_years(:,age_bin_iterator))*hist_bin_width
    lambda = lambda_by_age_intercept + lambda_by_age_slope*ages(age_bin_iterator);
    expected_distribution(:,age_bin_iterator) = ...
        wblpdf(x,lambda,kappa)*number_of_observations;
    
    
end

%% plot

figure(1);clf;set(gcf,'color','w');
hold on; 
plot(x,expected_distribution,':','linewidth',2);

% in newer versions of Matlab (2015), need to reset color order for lines
% to use the same colors:
ax = gca; ax.ColorOrderIndex = 1;

plot(hist_bins_years_survived,survived_years,'linewidth',2);
xlim([0 50]) % could set the data range limit, but sometimes it's nice to have the option to see extreme values

%legend(cellstr(num2str(ages(:))))
legend_strings_1 = cellstr(num2str(ages', 'Expected for age %-d'));
legend_strings_2 = cellstr(num2str(ages', 'Model output for age %-d'));
legend([legend_strings_1;legend_strings_2])
xlabel ('Time since infection (years)')
ylabel('Number of deaths per year')

%% remove path to helper functions

rmpath ../helper_functions_for_Matlab_plots/

