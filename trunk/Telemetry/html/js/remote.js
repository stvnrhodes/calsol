function date_to_compact_iso(date) {
    var iso = "";
    iso += date.getUTCFullYear();
    if (date.getUTCMonth() < 9) //months are stored from 0-11
        iso += '0';
    iso += (date.getUTCMonth() + 1);
    if (date.getUTCDate() < 10)
        iso += '0';
    iso += date.getUTCDate();
    if (date.getUTCHours() < 10)
        iso += '0';
    iso += date.getUTCHours();
    if (date.getUTCMinutes() < 10)
        iso += '0';
    iso += date.getUTCMinutes();
    if (date.getUTCSeconds() < 10)
        iso += '0';
    iso += date.getUTCSeconds();
    
    //iso += '.';
    //if (date.getUTCMilliseconds() < 100)
    //    iso += '0';
    //if (date.getUTCMilliseconds() < 10)
    //    iso += '0';
    //iso += date.getUTCMilliseconds();
    //iso += '000';
    return iso;
}
function DataInterval(id, name, start, end, on_update) {
    this.id = id;
    this.name = name;
    this.start = start;
    this.end = end;
    this.points = [];
    this.on_update = on_update;

    interval = this;

    var query = "?filter=between";
    query += "&after=" + date_to_compact_iso(this.start);
    query += "&before=" + date_to_compact_iso(this.end);

    function on_success(data, status, xhr) {
        for (var i=0; i < data.length; i++) {
            var ts = new Date(data[i][0] + "UTC");
            interval.points.push([ts, data[i][1]]);
        }
        if (interval.on_update) {
            interval.on_update(interval);
        }
    }
    $.ajax('/data/' + this.id + '/' + encodeURIComponent(this.name) + query, {
            success: on_success
    });
}

DataInterval.prototype.extendLeft = function extendLeft(start) {
    if (start) {
        var query = "?filter=between&before=" + date_to_compact_iso(this.start);
        query += "&after=" + date_to_compact_iso(start);
    } else {
        var query = "?filter=before&before=" + date_to_compact_iso(this.start);
    }
    function on_success(data, status, xhr) {
        for (var i=0; i < data.length; i++) {
            var ts = new Date(data[i][0] + "UTC");
            interval.points.push([ts, data[i][1]]);
        }
        if (interval.points.length > 0) {
            interval.end = interval.points[interval.points.length - 1][0];
        }
        if (interval.on_update) {
            interval.on_update(interval)
        }
    }
    $.ajax('/data/' + this.id + '/' + encodeURIComponent(this.name) + query, {
            success: on_success
    });
};

DataInterval.prototype.extendRight = function extendRight(end) {
    if (end) {
        var query = "?filter=between&after=" + date_to_compact_iso(this.end);
        query += "&before=" + date_to_compact_iso(end);
    } else {
        var query = "?filter=after&after=" + date_to_compact_iso(this.end);
    }
    function on_success(data, status, xhr) {
        for (var i=0; i < data.length; i++) {
            var ts = new Date(data[i][0] + "UTC");
            interval.points.push([ts, data[i][1]]);
        }
        if (interval.points.length > 0) {
            interval.end = interval.points[interval.points.length - 1][0];
        }
        if (interval.on_update) {
            interval.on_update(interval)
        }
    }
    $.ajax('/data/' + this.id + '/' + encodeURIComponent(this.name) + query, {
            success: on_success
    });
};
