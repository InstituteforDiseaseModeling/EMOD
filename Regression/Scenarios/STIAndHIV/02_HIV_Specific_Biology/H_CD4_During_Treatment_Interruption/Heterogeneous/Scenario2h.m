% SUMMARY: Plot CD4 progression over time with reconstitution from ART
% INPUT: output\ReportHIVByAgeAndGender.csv
% OUTPUT: Two figures displayed on screen


clear;clc;

%% options
actually_run_the_executable = false;
scenario_name = 'HIV_progression_cohort_2h';
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
addpath ../../helper_functions_for_Matlab_plots/

if actually_run_the_executable
    run_executable_locally(scenario_directory,command_line_text);
end

%% process and plot populations

age_gender_table = readtable([output_dir,'\ReportHIVByAgeAndGender.csv']);
by_year = grpstats(age_gender_table,{'Year'},'sum','DataVars',{'Population','Infected','On_ART'});

figure(1);clf;set(gcf,'color','w');hold on;
plot(by_year.Year, by_year.sum_Population,'b','LineWidth',4); 
bar(by_year.Year, by_year.sum_Infected,'FaceColor',[1 .5 .5],'EdgeColor',[1 .5 .5]);
bar(by_year.Year, by_year.sum_On_ART,'FaceColor',[.7 1 .7],'EdgeColor',[.7 1 .7]);
xlabel('Year')
ylabel('Number of Individuals')
legend('Total Population Size','Number Infected','Number on ART')
xlim([2000 2080])


%% process and plot CD4

infection_table  =  readtable([output_dir,'\ReportHIVInfection.csv']);
CD4_long = infection_table(:,{'Year','Id','CD4count'});
[CD4_pivot,Years,Ids,Settings] = Pivot(table2cell(CD4_long));
CD4_pivot = CD4_pivot(2:end,2:end); % drop column and row headings

figure(2);clf;set(gcf,'color','w');hold on;
plot(Years, CD4_pivot');
xlim([2020 2060])
ylabel('CD4 count')

%% remove path to helper functions

rmpath ../../helper_functions_for_Matlab_plots/


