function [data] = motorTeam(file)
% [data] = telemetry(file)
% Reads in a datalogger parser output file and turns it into a struct
% array.
fid = fopen(file, 'r');
data = struct('name', {}, 'desc', {}, 'times', {}, 'values', {});
structSide = 2;
while ~feof(fid)
    line = fgetl(fid);
    line = regexp(line, ';', 'split');
    numTime = str2double(line{1});
    attribute = line{2};
    description = line{3};
    value = str2double(line{4});
    if isempty(data)
        data(1).name = attribute;
        data(1).desc = description;
        data(1).times = numTime;
        data(1).values = value;
        continue
    end % if
    found = false;
    for i = 1:length(data)
        if strcmpi(data(i).desc, description)...
            && strcmpi(data(i).name, attribute)
            attributeLoc = i;
            found = true;
            break;
        end % if
    end % for
    if found
        data(attributeLoc).times = [data(attributeLoc).times numTime];
        data(attributeLoc).values = [data(attributeLoc).values value];
    else
        data(structSide).name = attribute;
        data(structSide).desc = description;
        data(structSide).times = numTime;
        data(structSide).values = value;
        structSide = structSide + 1;
    end % if
end % while
while(~strcmpi(data(4).desc, ...
        ' Vehicle Velocity m/s Vehicle velocity in metres / second.'))
    temp = data(4);
    data(4) = data(5);
    data(5) = data(6);
    data(6) = temp;
end % while
while(~strcmpi(data(5).desc, ...
        [' Motor Velocity rpm Motor angular '...
        'frequency in revolutions per minute.']))
    temp = data(5);
    data(5) = data(6);
    data(6) = temp;
end % while
end % function
