function [] = plots(struct)
% [] = plots(struct)
% Plot the telemetry data (ALL OF IT)
% Struct array of telemetry data expected
j = 1;
for i = 1:length(struct)
    if j > 2
        j = 1;
        figure
    end %if
    subplot(2,1,j);
    plot(struct(i).times, struct(i).values);
    title(sprintf([struct(i).id ' ' struct(i).name]));
    xlabel('Time (s)');
    ylabel(struct(i).desc)
    minimum = min(struct(i).values);
    maximum = max(struct(i).values);
    if minimum ~= maximum && minimum < maximum
        ylim([minimum maximum])
    end %if
    j = j + 1;
end % for
end % function