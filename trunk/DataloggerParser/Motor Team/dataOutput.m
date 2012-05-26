function [] = dataOutput(struct)
% [] = telemetryOutput(struct)
% Expects a struct array, presumably of CalSol datalogger data.
% Outputs to however many files there are
% Elements in the struct array.
for i = 1:length(struct)
    outTime = struct(i).times;
    name = [struct(i).str ' ' struct(i).id];
    text = [name '.csv'];
    output = [outTime struct(i).values'];
    dlmwrite(text, output, 'delimiter', ',', 'precision', '%.11f');
end % for
end % function