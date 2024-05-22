%%  Force_test_logger.m
%   use with Force_test_code_v2.ino on Arduino
clearvars
close all
format shortg

% Arduino settings, !!!CHECK COM NUMBER!!!
BAUD_RATE = 1000000;
s = serialport("COM3", BAUD_RATE); 
s.Timeout = 60; % set serial time out to one minute (60 seconds)
timeout = 60;   % 10 second timeout, for initializing
homing = false;

%dirName = 'G:\Mijn Drive\Graduation project\Thesis project\Matlab'; % where to save data
dirName = pwd; % where to save data
    
%prompt = {'Test name:', 'Number of cycles'}; % prompt asking for test name and the amount of cycles you want to test
prompt = {'Test name:'}; % prompt asking for test name
dlgtitle = 'Input';     % Title of the popup window
fieldsize = [1, 40];    % Size of the fields
userInputs = inputdlg(prompt,dlgtitle,fieldsize);   % Create input window
testName = userInputs{1};
%cycles = userInputs{2};

% Create the folder name 'Test name' and the number of cycles and date
folderName = sprintf('%s %s', datetime('now','Format',"y-M-d"), testName);
if ~exist(folderName, 'dir')
    mkdir(folderName);
end

%% Initialize
%do whatever to initialize/handsake
% get confirmation that arduino is ready to go
pause(1);
confirmationReceived = false;
write(s, "hello", "string");
tic;
while confirmationReceived == false
    data = readline(s);
    if startsWith(data, "Arduino is ready")
        confirmationReceived = true;
        disp(data);
    end
    % If it takes too long to receive ready confirmation, display an error
    % box and terminate the rest of the script
    if toc >= timeout
        % Error box setup
        CreateStruct.Interpreter = 'tex';
        CreateStruct.WindowStyle = 'non-modal';
        msgbox('\fontsize{15} No confirmation received from Arduino, check your setup',"Error","error", CreateStruct);
        error('No confirmation received from Arduino, check your setup')
    end
end

% Create a message box with a "Confirm" button
msg = 'Ready to home?';
dlg_title = 'Confirmation';
button = questdlg(msg, dlg_title, 'Confirm', 'Cancel', 'Cancel');

% Check the user's choice
if strcmp(button, 'Confirm')
    disp('Starting the homing sequence');
    % Add your code to continue here
else
    disp('Operation canceled.');
    error('User did not confirm ready to home')
    % Add code to handle the cancellation here
end

% Start homing
%CreateStruct.Interpreter = 'tex';
%CreateStruct.WindowStyle = 'non-modal';
%msgbox('\fontsize{15} Starting homing sequence', CreateStruct);
pause(1);
write(s, "skip", "string");
%disp('Commando verzonden: Home');

%disp('homing');
%pause(0.2);
%data = readline(s);
%disp(data);

%{
while homing == false
    data = readline(s);
    disp(data);

    if isempty(data)
        continue;  % Skip to the next iteration if no data is received
    end

    if startsWith(data, "Homed")
        homing = true;
        disp(data);
    end
end
%}
%forces = {[0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]};
%positions = {[0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]};

forces = cell(1,4);
positions = cell(1,4);
timestamp = cell(1,4);
flush(s);

for i = 1:4
    done = false;
    % msg set motor 1
    % Create a message box with a "Confirm" button
    msg = sprintf('Is motor %d connected to the load cell?', i);
    dlg_title = 'Confirmation';
    button = questdlg(msg, dlg_title, 'Confirm', 'Cancel', 'Cancel');
    
    % Check the user's choice
    if strcmp(button, 'Confirm')
        disp('Starting the sequence');
        % Add your code to continue here

        write(s, "begin", "string"); % Start the requested procedure
        pause(1.1);
        % Send command for which motor to run
        write(s, sprintf('F%d', i), "string");
        fprintf('F%d code send\n', i);
        flush(s);
        pause(1.1);
        tic;
        % While the motor is moving, read incomming data and store it
        while done == false
            
            data = readline(s);
            
            % Check if data is empty (timeout or no data)
            if isempty(data)
                continue;  % Skip to the next iteration if no data is received
            end
                
            if startsWith(data,"done")
                done = true;
            end
        
            % Split the data using the comma as a separator
            values = str2double(strsplit(data, ','));
        
            % Check if the data has the expected format
            if numel(values) ~= 9
                disp('Invalid data format');
                continue;  % Skip to the next iteration if the data format is invalid
            end
        
            % Extract and store the data into the vectors
            %retrievedPosition = [values(1), values(3), values(5), values(7)];   % i*2-1
            %retrievedForce = [values(2), values(4), values(6), values(8)];      % i*2
            
            retrievedPosition = values(i*2-1);  % i*2-1
            retrievedForce = values(i*2);       % i*2
            time = values(9) / 1000;                   % 9th value is time value from arduino
            
            timestamp{i} = [timestamp{i}; time];                % Store time value since start of measurement 
            forces{i} = [forces{i}; retrievedForce];            % Store force value
            positions{i} = [positions{i}; retrievedPosition];   % Store motor position value    
        end

    else
        disp('Operation canceled.');
        error('User cancelled operation')
    end    
end

write(s, "stop", "string");

% Save the data
filename = sprintf("sensorData_%s.mat", datetime('now','Format',"y-M-d_HH-mm-ss"));
filepath = fullfile(folderName, filename);
save(filepath, "positions", "forces", "timestamp");

delete(s);
clear s;

%% Plot values
%{
figure;
plot(timestamp{i}, forces{i}, 'LineWidth', 3);
title(sprintf('Raw Sensor Data vs Time, Motor #%d',i));
grid on; 
xlabel('Time (s)');
ylabel('Raw Sensor Value (-)');

figure
plot(positions{i}, forces{i}, 'LineWidth', 2);
title(sprintf('Raw Sensor Data vs Stepper position, Motor #%d',i));
grid on; 
xlabel('Position in steps (-)');
ylabel('Raw Sensor Value (-)');

figure
plot(timestamp{i}, positions{i}, 'LineWidth', 2);
title(sprintf('Stepper Position vs Time, Motor #%d',i));
grid on; 
xlabel('Position in steps (-)');
ylabel('Raw Sensor Value (-)');
%}







