#include "globals.h"
#include "RTClib.h"
#include <Wire.h>
#include <RTCZero.h>
#include <SD.h>
#include <MKRGSM.h>

//Real time clock for file name generation and decision making
RTCZero rtcZero;

//Stores the name of the file to be created and sent
String fileName;

//Time in string format
String Time = "00/00 00:00:00";

//Files for storing data, sending data and exploring SD card hierarchy
File root;
File fh;
File file ;

//Buffers for File Transfer Protocol communication
char outBuf[128];
char outCount;
byte clientBuf[64];
int clientCount;
byte respCode;
byte thisByte;

//Data transfer FTP ports
unsigned int hiPort, loPort;

//GSM-GPRS access classes
GPRS gprs;
GSM gsmAccess;
GSMClient client;
GSMClient dclient;

short startTime = 7;
short stopTime = 22;

int i, j;

void setup() {
  Serial.begin(115200);
  delay(1000);
  initGSM();
  initRTC();
}



void loop() {
  setTime();
  boolean postStartTime = ( (Time.substring(6, 8)).toInt() > startTime );
  boolean preStopTime = ( (Time.substring(6, 8)).toInt() < stopTime );
  // Time to sample and send data
  if ( postStartTime &&  preStopTime) {
    // Set the file name, based on time and date, where the sampled data will be stored
    setFileName();

    // Create the directory, based on time and date, where the sampled data will be stored.
    makeDir();

    // Start the sampling process. Store data in the specified file
    sample();

    // Connect to the FTP server for data transfer
    connectFTP();

    // Send the sampled data and append the log file
    sendFiles();

    // Close the connection with the FTP server
    closeFTP();

    //Reset the board and start over
    logData("Resetting...");
    reset();
  }
  // Time to sleep and wait until the next day
  else {
    // Sleep for an hour
    logData("Sleeping for an hour...");
    delay(3600000);

    // Reset the board
    logData("Resetting...");
    reset();
  }

}






//========================================= RTC/GSM Initialization  =========================================//

void initGSM() {
  boolean notConnected = true;
  gprs.setTimeout(180000);
  gsmAccess.setTimeout(180000);
  while (notConnected) {
    Serial.println("Initializing GSM, GPRS");
    logDataNoTime("Initializing GSM, GPRS");
    if ((gsmAccess.begin(SECRET_PINNUMBER) == GSM_READY) &&
        (gprs.attachGPRS(SECRET_GPRS_APN , SECRET_GPRS_LOGIN, SECRET_GPRS_PASSWORD) == GPRS_READY)) {
      Serial.println("GSM Initialized");
      logDataNoTime("GSM Initialized");
      notConnected = false;
    }
    else {
      Serial.println("Error initializing GPRS");
      logDataNoTime("Error initializing GPRS");
    }
  }
}


void initRTC() {
  rtcZero.begin();
  rtcZero.setEpoch(gsmAccess.getTime());
  logData("RTC initialized");
  Serial.println("RTC initialized");
  //rtc.adjust(DateTime(rtcZero.getYear(), rtcZero.getMonth(), rtcZero.getDay(), rtcZero.getHours(), rtcZero.getMinutes(), rtcZero.getSeconds()));
}






//================================================= FTP ==========================================//

void sendFiles() {
  setFileName();
  String rootDir = fileName.substring(0, 5) + "/";
  root = SD.open(rootDir);
  while (true) {
    fh =  root.openNextFile();
    if (! fh)
      break;

    //Serial.println(fh.name());
    if (uploadFTP()) {
      fh.close();
      SD.remove(String(fileName.substring(0, 5) + "/" + fh.name()));
    }
    else
      fh.close();
    closeFTP();
    if (fh =  SD.open("log.txt"))
      Serial.println("Log opened");
    connectFTP();


    if (uploadFTP()) {
      fh.close();
      SD.remove("LOG.txt");
    }
    else
      fh.close();
  }
  Serial.println("done!");
}

byte connectFTP() {
  if (client.connect(SECRET_FTP_HOST, 21)) {
    Serial.println(F("Command connected"));
    if (fh.name() != "log.txt")
      logData("Command connected");
  }
  else {
    fh.close();
    Serial.println(F("Command connection failed"));
    if (fh.name() != "log.txt")
      logData("Command connection failed");
    return 0;
  }

  if (!eRcv()) return 0;

  client.print(F("USER "));
  client.println(F(SECRET_FTP_USER));

  if (!eRcv()) return 0;

  client.print(F("PASS "));
  client.println(F(SECRET_FTP_PASSWORD));

  if (!eRcv()) return 0;

  client.println(F("SYST"));

  if (!eRcv()) return 0;

  client.println(F("PASV"));

  if (!eRcv()) return 0;

  char *tStr = strtok(outBuf, "(,");
  int array_pasv[6];
  for ( i = 0; i < 6; i++) {
    tStr = strtok(NULL, "(,");
    array_pasv[i] = atoi(tStr);
    if (tStr == NULL)
    {
      Serial.println(F("Bad PASV Answer"));
      if (fh.name() != "log.txt")
        logData("Bad PASV Answer");
    }
  }

  hiPort = array_pasv[4] << 8;
  loPort = array_pasv[5] & 255;

  Serial.print(F("Data port: "));
  if (fh.name() != "log.txt")
    logData("Data port: ");
  hiPort = hiPort | loPort;
  Serial.println(hiPort);
  if (fh.name() != "log.txt")
    logData((String)hiPort);


}

byte uploadFTP() {
  if (dclient.connect(SECRET_FTP_HOST, hiPort)) {
    Serial.println(F("Data connected"));
    if (fh.name() != "log.txt")
      logData("Data connected");
  }
  else {
    Serial.println(F("Data connection failed"));
    if (fh.name() != "log.txt")
      logData("Data connection failed");
    client.stop();
    fh.close();
    return 0;
  }

  client.print(F("APPE /data/"));
  client.print(fileName.substring(0, 5));
  client.print(F("/"));
  if (fh.name() != "log.txt")
    client.println(fh.name());
  else
    client.println(F("000.txt"));
  Serial.print("Uploading: ");
  Serial.println(F(fh.name()));
  if (fh.name() != "log.txt") {
    logData("Uploading: ");
    logData(fh.name());
  }

  Serial.println(F("Writing"));
  if (fh.name() != "log.txt")
    logData("Writing");


  clientCount = 0;

  while (fh.available())
  {
    clientBuf[clientCount] = fh.read();
    clientCount++;

    if (clientCount > 63)
    {
      dclient.write(clientBuf, 64);
      clientCount = 0;
    }
  }

  if (clientCount > 0) dclient.write(clientBuf, clientCount);



  dclient.stop();
  Serial.println(F("Data disconnected"));
  if (fh.name() != "log.txt")
    logData("Data disconnected");
  return 1;
}

byte closeFTP() {
  client.println(F("QUIT"));


  client.stop();
  Serial.println(F("Command disconnected"));
  if (fh.name() != "log.txt")
    logData("Command disconnected");

  Serial.println(F("SD closed"));
  fh.close();
  if (fh.name() != "log.txt")
    logData("SD closed");
  fh.close();
  return 1;
}

byte eRcv() {
  while (!client.available()) delay(1);

  respCode = client.peek();

  if (respCode != 50 && respCode != 51 && respCode != 52) {
    logData("peek: error");
    logData("Reseting..");
    NVIC_SystemReset();
  }

  outCount = 0;

  if (fh.name() != "log.txt")
    logTimeStamp();
  while (client.available())
  {
    thisByte = client.read();
    Serial.write(thisByte);
    if (fh.name() != "log.txt")
      logDataByte(thisByte);

    if (outCount < 127)
    {
      outBuf[outCount] = thisByte;
      outCount++;
      outBuf[outCount] = 0;
    }
  }

  if (respCode >= '4')
  {
    efail();
    return 0;
  }
  if (respCode >= 'T' || respCode >= 't')
  {
    efail();
    return 0;
  }


  return 1;
}

void efail() {
  thisByte = 0;

  client.println(F("QUIT"));

  while (!client.available()) delay(1);
  if (fh.name() != "log.txt")
    logTimeStamp();
  while (client.available())
  {
    thisByte = client.read();
    Serial.write(thisByte);
    if (fh.name() != "log.txt")
      logDataByte(thisByte);
  }

  client.stop();
  Serial.println(F("Command disconnected"));
  if (fh.name() != "log.txt")
    logData("Command disconnected");

  Serial.println(F("SD closed"));
  if (fh.name() != "log.txt")
    logData("SD closed");
  fh.close();
}






//============================================= LOGGING =============================================//

void logData(String command) {
  while (!SD.begin(4)) {
    Serial.println("SD error in logging");
  }
  File logFile ;
  logFile = SD.open("log.txt", FILE_WRITE);
  setTime();
  logFile.print(Time);
  logFile.print("-->");
  logFile.println(command);
  logFile.close();
}

void logDataNoTime(String command) {
  while (!SD.begin(4)) {
    Serial.println("SD error in logging");
  }
  File logFile ;
  logFile = SD.open("log.txt", FILE_WRITE);
  logFile.print(Time);
  logFile.print("-->");
  logFile.println(command);
  logFile.close();
}

void logDataByte(byte command) {
  File logFile ;
  logFile = SD.open("log.txt", FILE_WRITE);
  logFile.write(command);
  logFile.close();
}

void logTimeStamp() {
  File logFile ;
  logFile = SD.open("log.txt", FILE_WRITE);
  setTime();
  logFile.print(Time);
  logFile.print("-->");
  logFile.close();
}







//============================================= SAMPLING =============================================//

void setFileName() {
  // Print date...
  fileName = Time.substring(0, 2);
  fileName += "_";
  fileName += Time.substring(3, 5);
  fileName += "/";

  // ...and time
  fileName += String((Time.substring(6, 8)).toInt() + 1);
  fileName += "_";
  fileName += Time.substring(9, 11);
  fileName += ".txt";
}

void setTime() {
  Time = print2digits(rtcZero.getDay());
  Time += "/";
  Time += print2digits(rtcZero.getMonth());
  Time += " ";

  // ...and time
  Time += print2digits(rtcZero.getHours());
  Time += ":";
  Time += print2digits(rtcZero.getMinutes());
  Time += ":";
  Time += print2digits(rtcZero.getSeconds());
}

String print2digits(int number) {
  if (number < 10)
    return ("0" + (String)number) ;

  return ((String)number) ;
}

void sample() {
  while (!SD.begin(4)) {
    logData("SD error in sampling");
  }
  logData("SD opened");

  if (!(file = SD.open(fileName, FILE_WRITE)))
  {
    logData("File error in sampling process");
    reset();
  }

  logData("Started sampling process. File name: " + fileName);
  analogReadResolution(12);
  Serial.println(millis());
  for (j = 0; j < 20000; j++) {
    file.println(analogRead(A0));
    //delayMicroseconds(10);
  }
  Serial.println(millis());
  file.close();
  logData("Finished sampling process");

}

void makeDir() {
  while (!SD.begin(4)) {
    Serial.println("SD error");
  }

  if (SD.mkdir(fileName.substring(0, 5)))
    logData("Drirectory created");
  else
    logData("ERROR: Drirectory NOT created");
}

void reset() {
  NVIC_SystemReset();
}
