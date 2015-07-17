% SUMMARY: Run the model using the "dos" command
% INPUT: 
%   scenario_directory: directory containing the config.json and campaign.json files
%   command_line_text: command to execute
% OUTPUT: The model will run, producing output files

function status = run_executable_locally(scenario_directory, command_line_text)

current_directory = pwd;
cd(scenario_directory);
display('Running executable via DOS with command:');
display(command_line_text);
status = dos(command_line_text);
cd(current_directory)

end
