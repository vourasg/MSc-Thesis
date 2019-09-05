import urllib.request

i=0
# Initiate and open the URL request
req = urllib.request.Request('http://whitworth.cas.manchester.ac.uk/museum/index.html')
response = urllib.request.urlopen(req)

# At the 57th line of the HTML code, the time is displayd
while(i<57):
    response.readline()
    i+=1
time = response.readline().decode("utf-8")
time = time.split("at ")[1]
time = time[0:5]
print(time)

# At the 67th line, the current temperature is displayd
while(i<67):
    response.readline()
    i+=1
temperature = response.readline().decode("utf-8")
temperature = temperature.split(" deg")[0]
print(temperature)
