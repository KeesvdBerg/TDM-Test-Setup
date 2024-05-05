%% Prototype_Test_Setup_Run_Code_v1
% Test setup code, run with Running_Prototype_Arduino.ino
clearvars;

% Put your coefficient values here:
coefficientForce = [3.24084663875094e-08	-5.05417629422797e-05	0.0702671146066072	-5.03053361241453;
                    2.75632150591172e-08	-4.56181275068721e-05	0.0693551056825697	-4.99545907076776;
                    2.64808243056049e-08	-4.14734647379336e-05	0.0659811688911797	-3.89082212199920;
                    2.72312032656982e-08	-4.32843204272339e-05	0.0669665953255445	-4.54517151950324];

coefficientDistance =  [3.50290417292936e-09	-5.79274970356723e-06	0.00727622263291274	-0.460339243022289;
                        4.47700942338885e-09	-7.37102379710051e-06	0.00787624732413173	-0.459029829639233;
                        2.83584233354367e-09	-4.56354563519806e-06	0.00642079334139770	-0.333671277683865;
                        3.46099506142580e-09	-5.64923083512776e-06	0.00712714216789962	-0.367032137163038];


% Setting up save location
% Get the current script's directory
scriptDir = fileparts(mfilename('fullpath'));

% Prompt the user for the new folder name
prompt = {'Enter folder name:'};
dlgtitle = 'Input';
dims = [1 35];
folderName = inputdlg(prompt, dlgtitle, dims);

% Check if the user provided a name and then create the folder
if ~isempty(folderName)
     % Get the current date in the format 'year-month-day'
    currentDate = datestr(now, 'yyyy-mm-dd');
    
    % Combine the folder name with the current date
    folderName = [folderName{1}, '_', currentDate];
    
    % Create the full path for the new folder
    newFolderPath = fullfile(scriptDir, folderName);

    if ~exist(newFolderPath, 'dir')
        mkdir(newFolderPath);
    end
else
    error('No folder name was provided.');
end

% Define serial port name (adjust 'COM3' to match your system)
port = "COM3";
% Define baud rate
baudrate = 2000000; % Make sure this matches the baud rate in your Arduino code

% Define actuator properties
motors = 4; % Number of motors/sensors
pitch = 1.27; % Pitch in mm
stepper_resolution = 200; % Resolution of stepper WITHOUT microstepping in steps per revolution
microstep = 2; % Microstepping in whole numbers, i.e. half step is 2, quarter step is 4, etc.


% Define prompts for popup message
prompt = {'Enter the radius (in mm) at which the cables are located:', 'Enter the desired deflection (in degrees):'};
dlgtitle = 'Input for Manipulator';
dims = [1 35]; % Defines the dimensions of the input dialog box
definput = {'8','100'}; % Default inputs if you have any

% Display input dialog box
answer = inputdlg(prompt, dlgtitle, dims, definput);

% Parse responses
if ~isempty(answer) % Check if the user provided input or pressed cancel
    radius = str2double(answer{1});
    deflection = str2double(answer{2});
    
    % Check for valid numeric input
    if isnan(radius) || isnan(deflection)
        disp('Invalid input. Please enter numeric values.');
    else
        disp(['Radius: ', num2str(radius), ' mm']);
        disp(['Deflection: ', num2str(deflection), ' degrees']);
        
        % Proceed with displacement and steps calculations using radius and deflection
        displacement = radius * deflection / 180 * pi;
        conversionFactor = microstep * stepper_resolution / pitch;
        steps = round(displacement * conversionFactor);
    end
else
    disp('User cancelled the input dialog.');
end


% Create serialport object with a specified timeout
s = serialport(port, baudrate, "Timeout", 60); % 60-second timeout

% Wait for "Arduino ready" message
disp('Waiting for Arduino to be ready...');
arduinoMessage = "";
while true
    if s.NumBytesAvailable > 0
        arduinoMessage = readline(s);
        disp(['Received: ', arduinoMessage]); % For debugging: show received message
        if strtrim(arduinoMessage) == "Arduino ready"
            disp('Arduino is ready. Sending "Hello".');
            break;
        else
            disp('Waiting for "Arduino ready" message...');
        end
    end
    pause(0.1); % Small pause to prevent flooding the output
end

% Send "Hello" message to Arduino
writeline(s, "Hello");

% Wait for handshake confirmation from Arduino
disp('Waiting for handshake confirmation...');
confirmMessage = readline(s);
if strtrim(confirmMessage) == "Handshake"
    disp('Handshake confirmed. Proceeding with program.');
else
    disp(['Unexpected response: ', confirmMessage]);
end

% Command Arduino to home
writeline(s, "Home");

disp('Waiting for Homing confirmation...');
confirmMessage = readline(s);
if strtrim(confirmMessage) == "Homed"
    disp('Homing confirmed. Proceeding with program.');
else
    disp(['Unexpected response: ', confirmMessage]);
end

% Define the question and dialog title for the distal end connection
question = 'Is the distal end properly connected?';
dlg2_title = 'Connection Confirmation';
% Define the button names
btns = {'Confirm', 'Cancel'};
% Specify a default button
default = 'Confirm';
% Create the questdlg with a default choice
choice = questdlg(question, dlg2_title, btns{:}, default);

% Check the user's choice
if strcmp(choice, 'Confirm')
    % User pressed 'Confirm', send "connected" to Arduino
    writeline(s, "connected");
    disp('Confirmation sent to Arduino.');
    disp('Hold the manipulator straight while a pretension is applied');
    %pause(0.1);
else
    % User pressed 'Cancel' or closed the dialog
    disp('Connection confirmation canceled.');
end
pause(2);
% get confirmation that the "connected" message is received by arduino
message = readline(s);
disp(message);

% Wait for Arduino to pretension the cables
disp('Waiting for Pretension confirmation...');
confirmMessage = readline(s);
if strtrim(confirmMessage) == "Pretension"
    disp('Pretension confirmed. Proceeding with program.');
else
    disp(['Unexpected response: ', confirmMessage]);
end

% Define the question and dialog title for the distal end connection
question = 'Ready to start? Remove the tube from the manipulator.';
title_dialog = 'Ready Confirmation';
% Define the button names
btns = {'Confirm', 'Cancel'};
% Specify a default button
default = 'Confirm';
% Create the questdlg with a default choice
choice = questdlg(question, title_dialog, btns{:}, default);

% Check the user's choice
if strcmp(choice, 'Confirm')
    % User pressed 'Confirm', send "connected" to Arduino
    writeline(s, "start");
    disp('Confirmation sent to Arduino.');
else
    % User pressed 'Cancel' or closed the dialog
    disp('Ready confirmation canceled.');
end


% Starting the motion sequence
stepsMessage = sprintf('Steps_%d', steps); % Format the message

% Send the formatted message to Arduino
writeline(s, stepsMessage);

% Optional: Wait for Arduino to acknowledge the receipt of steps
acknowledgment = readline(s);
disp(['Arduino says: ', acknowledgment]);

% Send begin command to arduino
writeline(s, "begin");
acknowledgment = readline(s);
disp(['Arduino says: ', acknowledgment]);


% Preallocate space for about 1000 data points (adjust based on expected data size)
estimatedDataPoints = 1000;
runtimeVec = zeros(1, estimatedDataPoints);
motorPositionsMat = zeros(4, estimatedDataPoints); % Assuming 4 motors for example
sensorValuesMat = zeros(4, estimatedDataPoints); % Assuming 4 sensors matching motors
dataCount = 0;

% Continuously read data from Arduino
while true
    dataStr = readline(s);
    disp(dataStr);
    % Check if the sequence is finished
    if strtrim(dataStr) == "Finished"
        disp('Sequence finished.');
        break; % Exit the loop
    else
        dataNum = str2double(strsplit(dataStr, ','));
        if ~isnan(dataNum(1)) % Check if the first element is numeric
            dataCount = dataCount + 1;
            % Save the data
            runtimeVec(dataCount) = dataNum(1);
            motorPositionsMat(:, dataCount) = dataNum(2:2:end).'; % Transpose to match dimensions
            sensorValuesMat(:, dataCount) = dataNum(3:2:end).'; % Transpose to match dimensions
            
            % Grow the matrices if necessary
            if dataCount == size(motorPositionsMat, 2)
                % Double the size of the matrices
                motorPositionsMat = [motorPositionsMat, zeros(4, estimatedDataPoints)];
                sensorValuesMat = [sensorValuesMat, zeros(4, estimatedDataPoints)];
                runtimeVec = [runtimeVec, zeros(1, estimatedDataPoints)];
            end
        end
    end
end

writeline(s, "zero");
acknowledgment = readline(s);
disp(['Moving back to home position']);


% delete serial 
delete(s);

% Trim the unused preallocated space
runtimeVec(:, dataCount+1:end) = [];
runtimeVec(:,1) = 0;
motorPositionsMat(:, dataCount+1:end) = [];
motorDisplacement = (motorPositionsMat - motorPositionsMat(:,1)) ./ conversionFactor;
sensorValuesMat(:, dataCount+1:end) = [];
sensorValueClean = sensorValuesMat - sensorValuesMat(:,1);

% Display or process your data here
% For example, to display the final matrices sizes:
disp(['Final data count: ', num2str(dataCount)]);
disp(['Size of motorPositionsMat: ', num2str(size(motorPositionsMat))]);
disp(['Size of sensorValuesMat: ', num2str(size(sensorValuesMat))]);

% Specify the file path for saving the variables
variablesFilePath = fullfile(newFolderPath, 'variables.mat');

% Save the variables in the .mat file
save(variablesFilePath, 'runtimeVec', 'motorDisplacement', 'sensorValuesMat');

%% Correct for displacement of force sensor and calculate force
cableDisplacement = zeros(motors, length(sensorValuesMat));
force = zeros(motors, length(sensorValuesMat));
for i = 1:motors
    distanceTosubtract = polyval(coefficientDistance(i,:), sensorValuesMat(i,:));
    cableDisplacement(i,:) = motorDisplacement(i,:) - distanceTosubtract;
    force(i,:) = polyval(coefficientForce(i,:), sensorValuesMat(i,:));
end


%% Plotting the data
sens_disp = figure;
plot(cableDisplacement', force');
grid on
name1 = 'Force vs displacement';
title(name1);
legend('Motor 1', 'Motor 2', 'Motor 3', 'Motor 4');
xlabel("Cable displacement (mm)");
ylabel("Raw sensor value (-)");

% Specify the file path for saving the figure
figureFilePath = fullfile(newFolderPath, [name1, '.svg']);
% Save the figure as an SVG file
saveas(sens_disp, figureFilePath, 'svg');

% create runtime matrix to match size of sensorValuesMat
runtime_4 = [runtimeVec; runtimeVec; runtimeVec; runtimeVec] ./ 1000;

sens_time = figure;
plot(runtime_4', force');
grid on
name2 = 'Force vs time';
title(name2);
legend('Motor 1', 'Motor 2', 'Motor 3', 'Motor 4');
xlabel("Runtime (s)");
ylabel("Raw sensor value (-)");

% Specify the file path for saving the figure
figureFilePath = fullfile(newFolderPath, [name2, '.svg']);
% Save the figure as an SVG file
saveas(sens_time, figureFilePath, 'svg');

disp_time = figure;
plot(runtime_4', cableDisplacement');
grid on
name3 = 'Displacement vs time';
title(name3);
legend('Motor 1', 'Motor 2', 'Motor 3', 'Motor 4');
xlabel("Runtime (s)");
ylabel("Displacement (mm)");

% Specify the file path for saving the figure
figureFilePath = fullfile(newFolderPath, [name3, '.svg']);
% Save the figure as an SVG file
saveas(disp_time, figureFilePath, 'svg');


