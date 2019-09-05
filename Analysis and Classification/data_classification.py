# Returns the value in the appropriate format
def formatNum(data):
    temp = str(data)
    # Removes the first and last char, which are '[' and ']'
    num = temp[1:len(temp)-1]
    return float(num)



# An implementation of bubble sort, sorting amplitudes
def sort(data,indexes):
    n = len(data)
    for i in range(n):
        for j in range(0, n-i-1):
            if data[j] < data[j+1] :
                data[j], data[j+1] = data[j+1], data[j]
                indexes[j], indexes[j+1] = indexes[j+1], indexes[j]



# Checks for swarming-like distribution
def checkSwarming(indexes,temps):
    count = 0
    errors = []
    for i in range(6):
        # If there are requencies from 450-650Hz with high amplitude
        if(indexes[i]>=18 and indexes[i]<24):
            # count appearences and save the frequencies
            count +=1
            errors.append(indexes[i])
    result = "OK"
    # If more than 2 appearences, check the past 15 minutes temperatures
    if(count>=2):
        result = "Swarming\n"
        if(temps[len(temps)-1] - temps[0] > 2):
            result += "Temperature correlated"
        else:
            result += "Temperature NOT correlated"

    return result



# Checks for unhealthy status
def checkDisease(indexes):
    count = 0
    errors = []
    result = "OK"
    # If there are requencies <100 and >300 with high amplitude
    for i in range(6):
        if(indexes[i]<=3 and indexes[i]>11):
            count +=1
            errors.append(indexes[i])
    if(count>=2):
        result = "Unhealthy\n"
        result += "Abnormal distribution in frequencies: "
        for error in errors:
            result += translateBands(error)+", "
        result = result[:len(result)-2]
        result += "Hz"

    return result



# Calculates the average amplitude for +-25Hz with step 50 Hz
def createBands(data):
    bands = []
    c = 250
    while c < 9000:
        avg = float(0)
        for j in range(c-250,c+250):
            avg += formatNum(data[j])
        c = c + 250
        bands.append(avg/500)

    return bands



# Returns the corresponding frequencies for an index
def translateBands(index):
    lowFreq = index * 25
    highFreq = highFreq + 50
    return str(lowFreq) +"-"+ str(highFreq)



# The main functions, coordinates all the methods
def classifyData(data,temps):
    bands = createBands(data)
    indexes = []
    for i in range(len(bands)):
        indexes.append(i)
    sort(bands,indexes)
    swarming = checkSwarming(indexes,temps)
    disease = checkDisease(indexes)

    if(swarming!="OK" and disease!="OK"):
        return swarming + "\n" + disease
    elif(swarming=="OK" and disease!="OK"):
        return disease
    elif(swarming!="OK" and disease=="OK"):
        return swarming
    else:
        return "OK"
