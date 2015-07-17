% SUMMARY: Plot survival of vertically infected children against expected curves
% INPUT: output\HIVMortality.csv
% OUTPUT: Figure displayed on screen


clear;clc;

%% options
actually_run_the_executable = false;
scenario_name = 'HIV_progression_cohort_2b';
config_filename = 'config_HIV_progression_cohort_2b.json';
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

%% produce CDF plot

HIVMortality = readtable([output_dir,'HIVMortality.csv']);

% for expected distribution:
child_beta = 1.52;
child_s = 2.7;
child_mu = 16.0;
child_alpha = 0.57;

figure(3);clf;set(gcf,'color','w');
hold on;
x = 0:0.01:100;
area(x,1-Ferrand_CDF(x,child_alpha,child_beta,child_mu,child_s),'facecolor',[.7 .7 .7]);
h = cdfplot(HIVMortality.Years_since_infection);
set(h,'color',[0 0 0.5]);
set(h,'linewidth',4);
xlim([-0.5,35])
xlabel ('Time since infection (years)')
ylabel('Cumulative proportion died of HIV')
legend('Expected distribution','Model output','location','northwest')
title('')

%% remove path to helper functions

rmpath ../helper_functions_for_Matlab_plots/

