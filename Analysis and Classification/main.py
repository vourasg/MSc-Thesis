import sys
import subprocess
import matlab.engine
import ftplib
from datetime import date
import os
import data_classification
from multiprocessing import Process
from time import sleep
import mysql.connector


# Analyze the audio data using MATLAB engine to call script
def soundAnalysis(filename):
    engine = matlab.engine.start_matlab()
    x=engine.sound_analysis(filename)
    return(x)


# Call the classification.py script
def classify(data,temperatures):
    result = data_classification.classifyData(data,temperatures)
    return result


# A thread that every minute saves to a file the current time and 
# temperature using the TimeTemp.py script
def timeTempThread():
    while(True):
        returned_bin =  subprocess.check_output('python TimeTemp.py'.split())
        returned_val = returned_bin.decode('utf-8')
        time = returned_val.split("\n")[0]
        time = time[:5]
        temperature = returned_val.split("\n")[1]

        today = str(date.today()).split("-")[2] + "_" + str(date.today()).split("-")[1]
        try:
            os.mkdir("data/"+today)
        except:
            pass

        lf = open("data/"+today+"/temperatues.txt", "a+")
        lf.write(str(time + "\t" + temperature))
        lf.close()

        sleep(60)


# Reads from the temperature file the last 20 temperature values
def getTemperatues():
    today = str(date.today()).split("-")[2] + "_" + str(date.today()).split("-")[1]
    file = open("data/" + today + "/temperatues.txt", "r")
    temp=file.readlines()
    if(len(temp)<20):
        temp = temp[0:len(temp)-1]
    else:
        temp = temp[len(temp)-20:len(temp)-1]
    file.close()
    temperatures = []
    for i in range(len(temp)):
        temp[i] = (temp[i].split("\t"))[1]
        temp[i] = (temp[i])[:4]
        temperatures.append(float(temp[i]))
    return temperatures


# Used for downloading lines from FTP and append them to file
class writeFile:
    def __init__(self, filename):
        self.filename = filename

    def writeline(self,line):
        lf = open(self.filename, "a+")
        lf.write(line + "\n")
        lf.close()


#FTP method, downloads the available files from the FTP server
def FTP(thread):
    #Login to FTP server
    ftp = ftplib.FTP('nl1-ss10.a2hosting.com', 'gvourasa','ck89D28sWTk];V')
    print("Logged in to FTP server...")
    #get the current date
    today = str(date.today()).split("-")[2] + "_" + str(date.today()).split("-")[1]

    try:
        os.mkdir("data/"+today)
    except:
        pass

    #change direnctory
    ftp.cwd("data/"+today)
    print("Current FTP directory is: data/31_07...")

    #Download a list with the available files
    filelist = []
    ftp.retrlines("LIST", filelist.append)

    #For each record in the list, download the file
    for filename in filelist:
        #format the file's name
        filename = filename.split(None, 8)[-1].lstrip()
        #check if it is a file
        if(filename != "." and filename != ".." and filename!="log.txt"):
            # download the file
            print("Downloading file: " + filename)
            file = writeFile("data/"+today+"/"+filename)
            ftp.retrlines('RETR ' + filename, file.writeline)
            #delete it from FTP server
            ftp.delete(filename)
            ftp.quit()

            return "data/"+today+"/"+filename




# Upload to database the result
def DBUpload(result,time):
    mydb = mysql.connector.connect(
        host="den1.mysql3.gear.host",
        user="mscproject",
        passwd="Shea0pey!",
        database="mscproject"
    )
    #DATETIME in yyyy-mm-dd hh:mm:ss format
    datetime = str(date.today()).split(" ")[0] + " "
    datetime += time.split("_")[0] + ":" +  time.split("_")[1] + ":00"
    print(datetime)
    mycursor = mydb.cursor()

    sql = "INSERT INTO beehive_data (result, description, date) VALUES (%s, %s, %s)"
    if(result == "OK"):
        val = (1, result, datetime)
    else:
        val = (0, result, datetime)
    mycursor.execute(sql, val)

    mydb.commit()

    print(mycursor.rowcount, "record inserted.")





if __name__ == "__main__":
    thread = Process(target=timeTempThread)
    thread.start()
    while(True):
        filename = FTP(thread)
        print("File downloaded and deleted from FTP...\nStarting data analysis...")
        data = soundAnalysis(filename)
        print("Data analysis finished...")
        thread.terminate()
        temperatures = getTemperatues()
        thread = Process(target=timeTempThread)
        thread.start()
        print("Starting data classification...")
        result = classify(data,temperatures)
        print("Classification result: " + result)
        print("Uploading to database...")
        DBUpload(result,filename)
        print("Result uploaded to database...\nStarting over...")
        sleep(5)
