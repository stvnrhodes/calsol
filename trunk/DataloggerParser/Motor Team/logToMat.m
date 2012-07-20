function [data] = logToMat(file)
% [data] = telemetry(file)
% Reads in a datalogger parser output file and turns it into a struct
% array.
fid = fopen(file, 'r');
line = '';
while ~feof(fid)
    line = [line fgetl(fid)];
end % while
fclose(fid);
pat1 = '(?<time>..:..:..:...)';
pat2 = '(?<MPPT>MPPT..)';
pat3 = '(?<value>\s\S+W)';
dat1 = regexp(line, pat1, 'names');
dat2 = regexp(line, pat2, 'names');
dat3 = regexp(line, pat3, 'names');
data = {dat1, dat2, dat3};
end % function