% SUMMARY: Plot population and number infected over time to demonstrate disease progression
% INPUT: output\ReportHIVByAgeAndGender.csv
% OUTPUT: Figure displayed on screen


clear;clc;

%% options
actually_run_the_executable = false;
scenario_name = 'HIV_progression_cohort_2a';
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

%% import model output file

age_gender_table = readtable([output_dir,'\ReportHIVByAgeAndGender.csv']);
by_year = grpstats(age_gender_table,{'Year'},'sum','DataVars',{'Population','Infected','NewlyInfected'});

%% plot

figure(1);clf;set(gcf,'color','w');hold on;
plot(by_year.Year, by_year.sum_Population,'b','LineWidth',4); 
bar(by_year.Year, by_year.sum_Infected,'FaceColor',[1 .5 .5],'EdgeColor',[1 .5 .5]);
bar(by_year.Year, by_year.sum_NewlyInfected,'k');
xlabel('Year')
ylabel('Number of Individuals')
legend('Total Population Size','Number Infected','Number of New Infections')
xlim([2000 2070])

%% remove path to helper functions

rmpath ../helper_functions_for_Matlab_plots/

