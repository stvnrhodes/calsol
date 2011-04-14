import sys, math

route = None
MASS_CAR = 20 #kg
GRAVITY = 9.8 #m/s2
C1 = 0 #rolling constant
C2 = 0
C3 = 0
CD = 0
A = 0
RHO = 0
HV_CURRENT = 0
HV_RESISTANCE = 0
MOTOR_CURRENT = 0
MOTOR_VOLTAGE = 0

def powerConsumption(startPos, speed, time):
    # startPos format (lat, lon, alt)
    if route is None:
        load_data()

    targetDist = speed * time

    pti = closestPointIndex(route, startPos)
    dE = 0
    
    startDist = route[pti][3]
    currentDist = startDist
    
    firstCoord = startPos[0:2] +\
        (route[pti][2],) +\
        (route[pti][3] + distance(startPos, route[pti]),)
    #print 'firstCoord:', firstCoord    
    #print 'closestPt:', route[pti]
    distStep = distance(firstCoord, route[pti])
    dE += energyStep(firstCoord, route[pti], distStep, speed)
    currentDist += distStep

    lastCoord = route[pti]
    pti += 1
    while (currentDist - startDist < targetDist) and (not (pti == len(route))):
        currentDist += route[pti][3]
        
        distStep = route[pti][3] - lastCoord[3]
        dE += energyStep(route[pti], lastCoord, distStep, speed)
        #print 'dE:', dE
        lastCoord = route[pti]
        pti += 1
    #print 'distance:', currentDist-startDist
    #return (dE, route[pti-1])
    return dE

def iter_consumption(latitude, longitude, velocity, dt):
    """
    Assuming that we start at the point on the course nearest to the specified
    latitude and longitude, compute the energy consumed if we maintain the
    specified average velocity in (TODO: UNITS SHOULD GO HERE)

    Yields the computed dE on the final iteration.
    """

    if route is None:
        load_data()

    targetDist = velocity * dt

    pt_index = closestPointIndex(route, (latitude, longitude, 0))
    dE = 0.0

    startDist = currentDist = route[pt_index][3]
    firstCoord = route[pt_index]
    lastCoord = route[pt_index]
    pt_index += 1
    while (currentDist - startDist < targetDist) and (pt_index < len(route)):
        currCoord = route[pt_index]
        currentDist += currCoord[3]
        distStep = currCoord[3] - lastCoord[3]
        dE += energyStep(currCoord, lastCoord, distStep, velocity)
        lastCoord = currCoord
        pt_index += 1
        yield None
    yield dE
    

def energyStep(start, end, dist, speed):
    dE = 0
    currentSpeed = speed
    time = dist / currentSpeed
    
    # altitude
    dE += MASS_CAR * GRAVITY * (start[3] - end[3])
    
    # rolling
    dE += -1 * dist * (C1 + C2*currentSpeed + C3*math.pow(currentSpeed, 2))

    # drag
    dE += -.5 * CD * A * RHO * math.pow(currentSpeed, 3)

    # breaking??? this formula from the wiki makes no sense
    #dE += -.5 * MASS_CAR * math.pow(currentSpeed, 2)
    
    # HV losses
    dE += -1 * math.pow(HV_CURRENT, 2) * HV_RESISTANCE * time

    # Motor losses
    dE += -1 * MOTOR_CURRENT * MOTOR_VOLTAGE * time

    # Battery losses
    # ???

    # Low Voltage losses
    # ???

    return dE

    

def distance(c1, c2):
    import math
    lon1, lat1 = map(math.radians, c1[0:2])
    lon2, lat2 = map(math.radians, c2[0:2])
    R = 6371000
    dLat = lat2-lat1
    dLon = lon2-lon1
    a = math.sin(dLat/2.) * math.sin(dLat/2.) + \
        math.cos(lat1) * math.cos(lat2) * \
        math.sin(dLon/2.) * math.sin(dLon/2.);
    c = 2. * math.atan2(math.sqrt(a), math.sqrt(1-a))
    return R * c

def closestPointIndex(route, coord):
    closestDist = float('-Inf')
    for i in range(0, len(route)):
        d = distance(route[i], coord)
        if d > closestDist:
            closestDist = d
        else:
            return i

def load_data():
    """
    Load Route Data and put it in route as groups of coordinate points
    """
    global route
    if route is not None:
        return
    
    route = [] #format [ pos1, pos2, ... ]
    input = open("./data/course.csv")
    
    line = input.readline()
    while line:
        if line[0] == '#':
            # lines w/ #'s are for comments in my csv's
            line = input.readline()
            continue
        
        coord = map(float, line.strip().split(","))
        #print 'coord1:', coord
        #coord[1], coord[0] = coord[0], coord[1]
        #print 'coord2:', coord
        coord = tuple(coord)
        #print 'coord:', coord

        route += [coord]
        line = input.readline()
    input.close()
    
    #test that the data is in the correct format
    if route[0][1] < 120 or route[0][1] > 140:
        print 'route[0][0]:', route[0][1]
        print 'make sure you did not switch longitude and lattitude in the csv!!!'
        sys.exit(1)
    if route[0][0] > -10 or route[0][0] < -40:
        print 'route[0][1]:', route[0][0]
        print 'make sure you did not switch longitude and lattitude in the csv!!!'
        sys.exit(1)
    #end load_data()

if __name__ == '__main__':
    print 'started'
    
    time = 60*60 #seconds left in race
    start_pos = (-12, 130) #longitude, latitude, meters above sea level
    speed = 10 #m/s speed parametrized on something

    
    load_data()
    dE = powerConsumption(start_pos, speed, time)
    print 'dE:', dE
    print 'done'
