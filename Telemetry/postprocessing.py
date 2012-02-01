"""
A collection of subroutines for doing postprocessing on stored data before
sending it to a client for rendering/analysis.

In general, these methods are generators - invoking them will not cause
the entire sequence to be evaluated until they are in turn evaluated.
"""

import datetime
from itertools import groupby
from math import sqrt
from operator import itemgetter
from Queue import PriorityQueue

__all__ = ["full_outer_join", "simplify_by_extrema", "simplify_by_average",
           "transform_to_screen", "transform_to_sample",
           "simplify_ramer_douglas_peucker"]

first = itemgetter(0)
second = itemgetter(1)

def full_outer_join(series_list):
    """
    Given a list of data series [series1, series2, series3, ... seriesN]
    where each series is of the form [(t1, v1), (t2, v2), (t3, v3) ...]

    the output is a new series with exactly one row for each distinct time
    value from the union of all times in each series.

    This only works if within each series, for all ti, tj, i != j, ti != tj

    Each row contains exactly N+1 entries. The first entry is the time t, and
    the n+1th entry corresponds to the value at that time from the nth series.
    If there is no sample at time t in series n, then the value is None.

    The end result is a padded table suitable for use with graphing libraries
    that only support plotting multiple series along a real time axis if the
    series are indexed.
    """

    N = len(series_list)
    queue = PriorityQueue()

    def labeled_series(series, k):
        "Returns an iterator that indicates which series a sample is from"
        return ((time, value, k) for (time, value) in series)

    iterators = []
    for n, series in enumerate(series_list):
        iterators.append(labeled_series(series, n))
    for it in iterators:
        try:
            queue.put(it.next())
        except StopIteration:
            pass
            
    rows = []
    row = None

    while not queue.empty():
        time, value, n = queue.get()
        try:
            queue.put(iterators[n].next())
        except StopIteration:
            pass

        if row is None:
            row = [time] + [None]*N
        elif time != row[0]:
            yield tuple(row)
            row = [time] + [None]*N
        row[n+1] = value
        
    if row is not None:
        yield tuple(row)


def simplify_by_extrema(series):
    """
    Given a data series where multiple samples occur simultaneously,
    output a new series where all such samples are culled except for
    the maximum and minimum values at each instance in time. If all
    of the values at that time are the same, only one sample will be
    output for that time.
    """

    for (time, samples) in groupby(series, first):
        minimum = maximum = samples.next()[1]
        max_index = min_index = 0
        for i, (_, value) in enumerate(samples):
            if value < minimum:
                minimum = value
                min_index = i
            elif value > maximum:
                maximum = value
                max_index = i
        #Make sure to preserve the correct ordering of min/max values
        if min_index == max_index:
            yield (time, minimum)
        elif min_index < max_index:
            yield (time, minimum)
            yield (time, maximum)
        else:
            yield (time, maximum)
            yield (time, minimum)
        
def simplify_by_average(series):
    """
    Given a data series where multiple samples occur simultaneously,
    output a new series where all such samples are averaged into a single
    sample at each instance in time.
    """
    for (time, samples) in groupby(series, first):
        total = 0
        count = 0
        for (_, value) in samples:
            total += value
            count += 1
        if count == 1:
            yield (time, value)
        else:
            yield (time, total/float(count))

def to_millis(delta):
    seconds = delta.seconds + 86400 * delta.days
    return (delta.microseconds // 1000) + (seconds * 1000)

def to_delta(millis):
    return datetime.timedelta(milliseconds=millis)

def transform_to_screen(series, time_interval, value_range, width, height):
    """
    Given a series of (datetime-object, double) samples, map from sample space
    onto screen space so that we can do line simplification with tolerances
    specified in pixels.
    """


    time_start, time_end = time_interval
    value_start, value_end = value_range

    origin_width = to_millis(time_end - time_start)
    origin_height = value_end - value_start
    
    for (dt, value) in series:
        screen_x = width*to_millis(dt - time_start)/float(origin_width)
        screen_y = height*(value - value_start)/float(origin_height)
        yield (screen_x, screen_y)

def transform_to_sample(series, time_interval, value_range, width, height):
    time_start, time_end = time_interval
    value_start, value_end = value_range

    sample_width = to_millis(time_end - time_start)
    sample_height = value_end - value_start

    for (x, y) in series:
        dt = time_start + to_delta(sample_width*x/float(width))
        value = value_start + sample_height*y/float(height)
        yield (dt, value)

def point_line_distance(P, A, B):
    """
    Given points P, A, and B in R^2, find the distance between P and the
    line through A and B.
    """

    #If the line through A and B is defined by ax + by + c = 0,
    #then the distance is given by |a*Px + b*Py + c|/sqrt(a^2 + b^2)
    dy = B[1] - A[1]
    dx = B[0] - A[0]
##    a = dy
##    b = -dx
##    c = -(a*A[0] - b*A[1])
##
##    return abs(a*P[0] + b*P[1] + c)/sqrt(a**2 + b**2)
    y = P[1] - A[1]
    x = P[0] - A[0]
    num = float(x * dx + y * dy)
    denom = dx ** 2 + dy ** 2
    frac = num/denom
    perp_vector = (x - frac * dx, y - frac * dy)
    return (perp_vector[0] ** 2 + perp_vector[1] ** 2) ** 0.5
    

def simplify_ramer_douglas_peucker(polyline, epsilon, keep=None):
    """
    Given a sequence of points defining a piecewise linear curve in a 2D
    plane and an error tolerance value epsilon, output a subset of the points
    such that the maximum distance between the approximate polyline and any
    excluded points is epsilon.
    """
    if len(polyline) <= 2:
        return polyline
    keep = set()
    _ramer_douglas_peucker(polyline, epsilon, 0, len(polyline)-1, keep)
    return [pt for (i, pt) in enumerate(polyline) if i in keep]

##    if len(polyline) <= 2:
##        if keep is not None:
##            keep.extend(polyline)
##        return polyline
##
##    start = polyline[0]
##    end = polyline[-1]
##
##    max_dist = 0.0
##    index = 0
##    for i in xrange(1, len(polyline)-1):
##        d = point_line_distance(polyline[i], start, end)
##        if d > max_dist:
##            max_dist = d
##            index = i
##
##    if dest is None:
##        dest = []
##    
##    if max_dist >= epsilon:
##        simplify_ramer_douglas_peucker(polyline[:index], epsilon, dest)
##        dest.append(polyline[index])
##        simplify_ramer_douglas_peucker(polyline[index+1:], epsilon, dest)
##    else:
##        dest.append(start)
##        dest.append(end)
##
##    return dest

def _ramer_douglas_peucker(polyline, epsilon, i, j, keep):
    keep.add(i)
    keep.add(j)
    if j - i + 1 <= 2:
        return
    #print i,j
    start = polyline[i]
    end = polyline[j]
    max_dist = 0.0
    max_index = 0
    for index in xrange(i, j+1):
        d = point_line_distance(polyline[index], start, end)
        if d > max_dist:
            max_dist = d
            max_index = index

    if max_dist >= epsilon:
##        print "max dist %.2f exceeds threshold of %.2f" % (max_dist, epsilon)
##        print "at point #%d in the interval [#%d, #%d]" % (max_index, i, j)
##        print start, polyline[max_index], end
        #keep.add(max_index)
        _ramer_douglas_peucker(polyline, epsilon, i, max_index, keep)
        _ramer_douglas_peucker(polyline, epsilon, max_index, j, keep)
        
