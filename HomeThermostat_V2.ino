/*
  1- Wi-Fi
    -User interface for SSID/Password     OK

  2- Local Date/Time Set                  OK

  3- Local Temp/Hum                       OK

  4- External Temp/Hum (WUnderground)     OK  Deleted for now.

  5- Trigger Intervals
    -30 sec local check                 OK
    -10 mins #4 update                  OK  Deleted for now
    -24h reboot                         OK  Deleted for now

  6- Alexa
    - Alexa request
      -English
        -Turn On            OK
        -Turn Off           OK
        -Get Room Temp      OK
        -Set Temp           OK
        -Get Settings       OK
        -Diag               OK
        -upTime             OK
      -Spanish
        -Turn On
        -Turn Off
        -Get Room Temp
        -Set Room Temp

  5- Screen
    -Main Screen
      - + Temp (UP)                 OK
      - - Temp (Down)               OK
      - Room Temp                   OK
      - Set Temp                    OK
      - Light Indicator On          OK
      - Light Indicator Heat on     OK
    -Diagnostic Screen
        -On/Off Status              OK
        -Room Temp                  OK
        -Set Temp                   OK
        -External Temp              Not for now
        -Heat Index                 OK
        -Date/Time                  OK
        -UpTime                     OK
        -Last Alexa Access          OK
    -Network Screen
        -SSID                       OK
        -IP                         OK
        -Gateway                    OK
        -MAC                        OK
        -Signal WiFi.RSSI()         OK
    -System Screen
        -Date Time (Running)        OK
        -Version                    OK
        -Uptime                     OK
        -Internal Voltage           OK


  8- Optimize
    -Delete/Comment Println
    -Delete/Comment unnecesary code

  9- Document
    -Standatd header
    -short function/sub comments
    -any relevant comment needed.
*/
/*
  Thermostat Logic.

  Room Temp: Temperature is retirved every 30 secs from the DHT11 Sensor.
  Set Temp: Temperature that starts or stops the heating. This value is set via Nextion Screen or Alexa.
  On/Off: This setting is the device "On/Off". If Off, the heat is alwas off regardless of the Set/Room values.
        If On, Set/Room values dictacte if relay is on or off.

  Every 30 secs: (CheckTemp fucntion)
  If On/Off = 1 (True or On)
    If Room Temp < Set Temp
      Open Relay
      Update Nextion
    Else
      Close Relay
      Update Nextion
  Else
    Do Nothing

  On On/Off button Change
  CheckTemp function

  On Set Temp Value Change (Alexa/Nextion)
  CheckTemp function






*/
// ----- Include Section ------
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <WiFiUdp.h>
#include <Nextion.h>
#include <NextionHotspot.h>
#include <NextionNumber.h>
#include <NextionText.h>
#include <NextionVariableString.h>
#include <NextionVariableNumeric.h>
#include <NextionPicture.h>
#include <NextionHotspot.h>
#include <SoftwareSerial.h>
#include "DHT.h"
#include <ArduinoJson.h>
#define CAYENNE_PRINT Serial     // Comment this out to disable prints and save space
#include <CayenneMQTTESP8266.h> // Change this to use a different communication device. See Communications examples.


ADC_MODE(ADC_VCC);
/*---- Global Vars ----*/
int ExTemp = 0;
int ExHum  = 0;
int RoomTem = 0;
int RoomHum = 0;
int HeatIndex = 0;
int SetTemp = 23;
String TempSP[46] = {"Cero", "Uno", "Dos", "Tres", "Cuatro", "Cinco", "Seis", "Siete", "Ocho", "Nueve", "Diez", "Once", "Doce", "Trece", "Catorce", "Quince", "Dieciseis", "Diecisiete", "Dieciocho", "Diesi nueve", "Veinte", "Veintiuno", "Veintidos", "Veintitres", "Veinticuatro", "Veinticinco", "Veintiseis", "Veintisiete", "Veintiocho", "Veintinueve", "Treinta", "TreintaiUno", "TreintaiDos", "TreintaiTres", "TreintaiCuatro", "TreintaiCinco", "TreintaiSeis", "TreintaiSiete", "TreintaiOcho", "TreintaiNueve", "Cuarenta", "CuarentaiUno", "CuarentaiDos", "CuarentaiTres", "CuarentaiCuatro", "CuarentaiCinco"};
String MonthNames[13] = {"", "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
bool ThermStatus = false;
#define RelayPin D5
#define DHTPIN D6
#define NextionRX D7
#define NextionTX D8
#define OnLED D4
#define HeatLED D4

int bootTime = 0;
int AlexaTime = 0;
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
char chaTemp[16];

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char username[] = "e65997b0-2d8d-11e8-b1c6-0d0b749c9848";
char password[] = "88c40fa0cff1aa7d5c4f86903f78636cb23dad6b";
char clientID[] = "f97d30f0-2dd7-11e8-af7b-4f8845dd6501";

/*-------- Web Server code ----------------*/
ESP8266WebServer server(81);
String webString = "";   // String to display
void handle_root() {
  server.send(200, "text/plain", "You are now connected to the home thermostat.");
  //Serial.println("HTTP root has been called.");
  Alarm.delay(500);
}

//Nextion
SoftwareSerial nextionSerial(NextionRX, NextionTX); // RX, TX
Nextion nex(nextionSerial);
//Page0
NextionNumber txtRoomTemp(nex, 0, 2, "txtRoomTemp");
NextionNumber txtSetTemp(nex, 0, 1, "txtSetTemp");
NextionHotspot sp_Up(nex, 0, 3, "sp_Up");
NextionHotspot sp_Down(nex, 0, 4, "sp_Down");
NextionHotspot hs_OnOff(nex, 0, 7, "hs_OnOff");
NextionText txt_MainTime(nex, 0, 22, "txt_MainTime");

NextionHotspot hs_Diagnostics0(nex, 0, 19, "hs_Diagnostics");
NextionHotspot hs_Network0(nex, 0, 9, "hs_Network");
NextionHotspot hs_Systems0(nex, 0, 21, "hs_Systems");
NextionHotspot hs_reset(nex, 0, 20, "hs_reset");

NextionVariableString varLocalIP(nex, 0, 10, "varLocalIP");
NextionVariableString varSSID(nex, 0, 11, "varSSID");
NextionVariableString varHost(nex, 0, 12, "varHost");
NextionVariableString varGateway(nex, 0, 13, "varGateway");
NextionVariableString varMAC(nex, 0, 14, "varMAC");
NextionPicture pRunning(nex, 0, 16, "pRunning");
NextionPicture pBooting(nex, 0, 15, "pBooting");
NextionVariableNumeric varOnOff(nex, 0, 8, "varOnOff");
NextionVariableNumeric varRunning(nex, 0, 17, "varRunning");
NextionVariableNumeric varBooting(nex, 0, 18, "varBooting");

//Page1
NextionText txt_SSID(nex, 1, 3, "txt_SSID");
NextionText txt_HOST(nex, 1, 4, "txt_HOST");
NextionText txt_IP(nex, 1, 5, "txt_IP");
NextionText txt_GATEWAY(nex, 1, 6, "txt_GATEWAY");
NextionText txt_MAC(nex, 1, 7, "txt_MAC");

NextionHotspot hs_Thermostat1(nex, 1, 1, "hs_Thermostat");
NextionHotspot hs_Diagnostics1(nex, 1, 8, "hs_Diagnostics");
NextionHotspot hs_Systems1(nex, 1, 9, "hs_Systems");

//Page2
NextionText txt_Diga(nex, 2, 3, "txt_Diga");
NextionHotspot hp_Thermostat2(nex, 2, 1, "hp_Thermostat");
NextionHotspot hs_Network2(nex, 2, 2, "hs_Network");
NextionHotspot hs_Systems2(nex, 2, 4, "hs_Systems");


//Page3
NextionText txt_Date(nex, 3, 4, "txt_Date");
NextionText txt_SDK(nex, 3, 5, "txt_SDK");
NextionText txt_CPU(nex, 3, 6, "txt_CPU");
NextionText txt_Size(nex, 3, 7, "txt_Size");
NextionText txt_ADC(nex, 3, 8, "txt_ADC");
NextionText txt_Core(nex, 3, 9, "txt_Core");
NextionText txt_Uptime(nex, 3, 10, "txt_Uptime");

NextionHotspot hs_Thermostat3(nex, 3, 1, "hs_Thermostat");
NextionHotspot hs_Diagnostics3(nex, 3, 2, "hs_Diagnostics");
NextionHotspot hs_Network3(nex, 3, 3, "hs_Network");

#define ThermoOn  65535
#define ThermoOff 63488


void PowerOnLED(int RPin, int RelayState) {
  pinMode(RPin, OUTPUT);
  digitalWrite(RPin, RelayState);
};
void HeatOnLED(int RPin, int RelayState) {
  pinMode(RPin, OUTPUT);
  digitalWrite(RPin, RelayState);
};

void TurnOff() {
  Serial.println(" Status is On ---> Off");
  varOnOff.setValue(0);
  nex.sendCommand("vis pRedOff,1");
  nex.sendCommand("vis pBlueOn,0");
  txtSetTemp.setForegroundColour(ThermoOff);
  ThermStatus = false;
}
void TurnOn() {
  Serial.println(" Status is Off ---> On");
  varOnOff.setValue(1);
  nex.sendCommand("vis pRedOff,0");
  nex.sendCommand("vis pBlueOn,1");
  txtSetTemp.setForegroundColour(ThermoOn);
  ThermStatus = true;
}
void OnOffCallback(NextionEventType type, INextionTouchable *widget) {
  if (type == NEX_EVENT_POP) {
    int nTemp;
    Serial.print("On Off was pressed :");
    nTemp = varOnOff.getValue();
    if (nTemp == 0) {
      TurnOn();
    }
    else {
      TurnOff();
    }
    CheckThermo();
  }
}
void callback(NextionEventType type, INextionTouchable *widget) {
  if (type == NEX_EVENT_PUSH)
  {

  }
  else if (type == NEX_EVENT_POP)
  {
    ////Serial.println("Push2");
    Serial.print("Changing SetTemp from ");
    Serial.print(SetTemp);
    SetTemp = txtSetTemp.getValue();
    Serial.print(" -----> ");
    Serial.println(SetTemp);

    CheckThermo();
  }
}
void OpenDiagScreen(NextionEventType type, INextionTouchable *widget) {
  Serial.println("--------------------------------------");

  if (type == NEX_EVENT_POP) {
    NextionDiag(txt_Diga);
  }

}
void OpenSystemsScreen(NextionEventType type, INextionTouchable *widget) {
  Serial.println("--------------------------------------");

  if (type == NEX_EVENT_POP) {
    SystemScreen();
  }

}
void HS_Reset(NextionEventType type, INextionTouchable *widget) {
  Serial.println("R E S E T - R E B O O T");
  if (type == NEX_EVENT_POP) {
    RebootESP();
  }
}

//NTP
static const char ntpServerName[] = "us.pool.ntp.org";
const int timeZone = -3;     // Chile
WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets
time_t getNtpTime();
void sendNTPpacket(IPAddress &address);

/*-------- NTP code ----------*/
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
time_t getNtpTime() {
  IPAddress ntpServerIP; // NTP server's ip address
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  //Serial.println("Transmit NTP Request");
  // get a random server from the pool
  if (String(ntpServerIP) == "0.0.0.0") {
    //Serial.println("No connection to NTP server!!!!!!!!");
  }
  //Serial.print("Local IP: ");
  //Serial.println(WiFi.localIP());
  WiFi.hostByName(ntpServerName, ntpServerIP);
  //Serial.print(ntpServerName);
  //Serial.print(": ");
  //Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      //Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  //Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}
// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
String printDigits(int digits) {
  String tempstr = "";
  // utility for digital clock display: prints preceding colon and leading 0
  tempstr = ":";
  if (digits < 10)
    tempstr = tempstr + '0';
  tempstr = tempstr + String(digits);
  return tempstr;
}
String digitalClockDisplay() {
  String tmpstr = "";
  // digital clock display of the time
  tmpstr = tmpstr + String(hour());
  tmpstr = tmpstr + printDigits(minute());
  tmpstr = tmpstr + printDigits(second());
  tmpstr = tmpstr + (" ");
  tmpstr = tmpstr + String(day());
  tmpstr = tmpstr + String(".");
  tmpstr = tmpstr + String(month());
  tmpstr = tmpstr + String(".");
  tmpstr = tmpstr + String(year());
  return tmpstr;
};
void SetNextionDateTime() {
  String tmpstr = "";
  char strText[20];

  tmpstr = tmpstr + String(day());
  tmpstr = tmpstr + String("/");
  tmpstr = tmpstr + String(month());
  tmpstr = tmpstr + String("/");
  tmpstr = tmpstr + String(year());
  tmpstr = tmpstr + (" ");
  tmpstr = tmpstr + String(hour());
  tmpstr = tmpstr + printDigits(minute());
  tmpstr = tmpstr + printDigits(second());
  tmpstr.toCharArray(strText, 20);
  txt_Date.setText(strText);
  txt_MainTime.setText(strText);
}
/*---- Functions ----*/
void CheckThermo() {

  int RelayStatus = 0;
  Serial.println("----------- Checking -------------");
  Serial.println(digitalClockDisplay());

  GetRoomTemp();
  txtRoomTemp.setValue(RoomTem);
  SetTemp = txtSetTemp.getValue();
  ThermStatus = varOnOff.getValue();
  RelayStatus = digitalRead(RelayPin);
  if (ThermStatus){ //Thermostat is On
    if (RoomTem < SetTemp){
      if (RelayStatus == LOW){ //Heat is off, open relay and set heat on
        SetRelay(RelayPin, HIGH);
      }
    }
    else{
      if (RelayStatus == HIGH){ //Heat is on, close relay and set heat off
        SetRelay(RelayPin, LOW);
      }
    }
  }  
  else{ //Termostat is off, turn heat off just in case.
    SetRelay(RelayPin, LOW);
  }
  Cayenne.celsiusWrite(0, RoomTem);
  Cayenne.celsiusWrite(1, SetTemp);
}  

  String SpanishTest() {
    String strSPTest = "";
    strSPTest = strSPTest + "<phoneme alphabet='ipa' ph='ɛl̪ tɛɾ.mos.ˈta.to se ˈa ɛ̃n.sɛ̃n̪.ˈdi.ðo'>El termostato se a encendido</phoneme>.";
    strSPTest = strSPTest + "<phoneme alphabet='ipa' ph='ɛl̪ tɛɾ.mos.ˈta.to se ˈa a.pa.ˈɣa.ðo'>el termostato se ha apagado</phoneme>.";
    strSPTest = strSPTest + "<phoneme alphabet='ipa' ph='la tɛ̃m.pɛ.ɾa.ˈtu.ɾa ãm.bjɛ̃n̪.ˈtal ˈɛs̬ ðe ˈɣɾa.ðos'></phoneme>.";
    strSPTest = strSPTest + "<phoneme alphabet='ipa' ph='ũno ð̞os̮ tɾes̮ kwatɾo θinˠko sei̯s̮ sjete o͡ʧo nweβ̞e ð̞jes'></phoneme>.";

    //strSPTest = strSPTest + "<phoneme alphabet='ipa' ph=''></phoneme>.";
    return strSPTest;
  }
  void SetDefaultsStartup() {
    nex.sendCommand("vis pBooting,0");
    varBooting.setValue(0);
    nex.sendCommand("vis pRunning,0");
    varRunning.setValue(0);
    txtRoomTemp.setValue(-1);
    txtSetTemp.setValue(-1);
    ThermStatus = false;
    varOnOff.setValue(0);
    Alarm.delay(2000);
    nex.sendCommand("vis pBooting,1");
    varBooting.setValue(1);
    txtSetTemp.setForegroundColour(ThermoOff);
    txtSetTemp.setValue(SetTemp);
    nex.sendCommand("vis pRedOff,1");
    nex.sendCommand("vis pBlueOn,0");
    txt_Diga.setText("Booting");

  }
  void SetRelay(int RPin, int RelayState) {
    pinMode(RPin, OUTPUT);
    digitalWrite(RPin, RelayState);
  };
  String ThermoStatus() {
    if (ThermStatus) {
      return "On";
    }
    else
    {
      return "Off";
    }

  }
  String Diagnostics() {
    String strDiag = "Diagnostic is complete. ";

    strDiag = strDiag + "Thermostat status is " + ThermoStatus() + ". ";
    strDiag = strDiag + "Room temperature is " + String(RoomTem) + " degrees"  + ". ";
    //strDiag = strDiag + "External temperature is " + String(ExTemp) + " degrees" + ". ";
    strDiag = strDiag + "Thermostat is set to " + String(SetTemp) + " degrees" + ". ";
    strDiag = strDiag + "Thermostat current date is " + MonthNames[month()] + " " + String(day()) + " and the current time is " + String(hour()) + printDigits(minute()) + ". ";
    strDiag = strDiag + "S S I D is " + WiFi.SSID() + ".";
    strDiag = strDiag + "Local I P is " + IP2String(WiFi.localIP()) + " .";
    strDiag = strDiag + "The thermostat has been operational for " + GetUpTime(true); + ".";
    return strDiag;
  }
  void NextionDiag(NextionText txtObj) {
    char strText[250];
    String tempS = "";
    tempS.toCharArray(strText, 250);
    txtObj.setText(strText);
    Alarm.delay(500);
    Serial.println("Diagnostic screen open");
    tempS = "Thermostat status: " + ThermoStatus() + ".\\r";
    tempS.toCharArray(strText, 250);
    txtObj.setText(strText);
    tempS = tempS + "Room temperature: " + String(RoomTem) + "C" + ".\\r";
    tempS.toCharArray(strText, 250);
    txtObj.setText(strText);
    tempS = tempS + "Heat index: " + String(HeatIndex) + "C" + ".\\r";
    tempS.toCharArray(strText, 250);
    txtObj.setText(strText);
    tempS = tempS + "Thermostat set: " + String(SetTemp) + "C" + ".\\r";
    tempS.toCharArray(strText, 250);
    txtObj.setText(strText);
    tempS = tempS + "Date: " + MonthNames[month()] + " " + String(day()) + ".\\r";
    tempS.toCharArray(strText, 250);
    txtObj.setText(strText) ;
    tempS = tempS + "Time: " + String(hour()) + printDigits(minute()) + ".\\r";
    tempS.toCharArray(strText, 250);
    txtObj.setText(strText);
    tempS = tempS + "Uptime: " + GetUpTime(false) + ".\\r";
    tempS.toCharArray(strText, 250);
    txtObj.setText(strText);
    tempS = tempS + "Alexa: " + GetAlexaTime(false) + ".\\r";
    tempS.toCharArray(strText, 250);
    txtObj.setText(strText);
  }
  void SystemScreen() {
    char strText[25];
    String tempS = "";
    float fTemp;

    Serial.println("Systems screen open");
    tempS = ESP.getSdkVersion();
    tempS.toCharArray(strText, 25);
    txt_SDK.setText(strText);

    tempS = ESP.getCpuFreqMHz();
    tempS = tempS + "Mhz";
    tempS.toCharArray(strText, 25);
    txt_CPU.setText(strText);

    fTemp = ESP.getSketchSize() / 1024.0;
    tempS = String(fTemp) + "MB";
    tempS.toCharArray(strText, 25);
    txt_Size.setText(strText);

    fTemp = ESP.getVcc() / 1000.0;
    tempS = String(fTemp) + "DCV";
    tempS.toCharArray(strText, 25);
    txt_ADC.setText(strText);

    tempS = ESP.getCoreVersion();
    tempS.toCharArray(strText, 25);
    txt_Core.setText(strText);

    tempS = GetUpTime(false);
    tempS.toCharArray(strText, 25);
    txt_Uptime.setText(strText);

  }
  String GetUpTime(bool isText) {
    String strTemp = "";
    int seconds = now() - bootTime;
    int days = seconds / 86400;
    seconds %= 86400;
    byte hours = seconds / 3600;
    seconds %= 3600;
    byte minutes = seconds / 60;
    seconds %= 60;
    if (isText) {
      strTemp = String(days) + " days, " + String(hours) + " hours, " + String(minutes) + " minutes and " + String(seconds) + " seconds since last system reboot.";
    }
    else {
      strTemp = String(days) + "D " + String(hours) + "H " + String(minutes) + "M " + String(seconds) + "S";
    }
    return strTemp;
  }
  String GetAlexaTime(bool isText) {
    String strTemp = "";
    if (AlexaTime == 0) {
      if (isText) {
        strTemp = "has not been accessed since last system reboot.";
      }
      else {
        strTemp = "No access requested.";
      }
    }
    else {
      int seconds = now() - AlexaTime;
      int days = seconds / 86400;
      seconds %= 86400;
      byte hours = seconds / 3600;
      seconds %= 3600;
      byte minutes = seconds / 60;
      seconds %= 60;
      if (isText) {
        strTemp = String(days) + " days, " + String(hours) + " hours, " + String(minutes) + " minutes and " + String(seconds) + " seconds since last Alexa access..";
      }
      else {
        strTemp = String(days) + "D " + String(hours) + "H " + String(minutes) + "M " + String(seconds) + "S";
      }
    }
    return strTemp;
  }
  void GetRoomTemp() {
    /*
      1- Get Room Temp form DHT11 Sensor.
        -Get Temp value in C
        -Get Hum value in %
        -Get Heat Index in C
    */
    float fTemp = 0;
    fTemp = dht.readTemperature();
    RoomTem = fTemp;
    RoomHum = dht.readHumidity();
    HeatIndex = dht.computeHeatIndex(RoomTem, RoomHum, false);
    if (isnan(RoomHum) || isnan(RoomTem)) {
      RoomTem = dht.readTemperature();
      RoomHum = dht.readHumidity();
      HeatIndex = dht.computeHeatIndex(RoomTem, RoomHum, false);
    }
  }
  void RebootESP() {
    ESP.restart();
  }
  char * IP2String(IPAddress IPString) {
    byte oct1 = IPString[0];
    byte oct2 = IPString[1];
    byte oct3 = IPString[2];
    byte oct4 = IPString[3];

    sprintf(chaTemp, "%d.%d.%d.%d", oct1, oct2, oct3, oct4);
    return chaTemp;
  }
  String mac2String(byte ar[]) {
    String s;
    for (byte i = 0; i < 6; ++i)
    {
      char buf[3];
      sprintf(buf, "%2X", ar[i]);
      s += buf;
      if (i < 5) s += ':';
    }
    s.replace(" ", "0");
    return s;
  }
  void GetNetworkData() {
    //Serial.println("----------- setting Nextion Network screen data");
    char bla[25];
    char bla3[25];


    WiFi.SSID().toCharArray(bla, 25);
    varLocalIP.setText(IP2String(WiFi.localIP()));
    Alarm.delay(50);
    varSSID.setText(bla);
    long rssi = WiFi.RSSI();
    String(rssi).toCharArray(bla3, 25);
    varHost.setText(bla3);
    varGateway.setText(IP2String(WiFi.gatewayIP()));
    byte bla2[6];
    WiFi.macAddress(bla2);
    mac2String(bla2).toCharArray(bla3, 25);
    varMAC.setText(bla3);
  }
  void configModeCallback (WiFiManager * myWiFiManager) {
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());
    //if you used auto generated SSID, print it
    Serial.println(myWiFiManager->getConfigPortalSSID());
  }
  void setup() {
    PowerOnLED(OnLED,LOW);
    Serial.begin(115200);         //Start Serial debugger
    Serial.println("1....");
    /*---- Nextion Init ---- 1*/
    Alarm.delay(250);
    Serial.println("1....");
    nextionSerial.begin(9600);    //Start Nextion Serial
    Serial.println("2....");
    Alarm.delay(50);
    Serial.println("3....");
    nex.init();                   //Start Nextion
    Serial.println("4....");
    dht.begin();                  //Start Temp Sensor
    Serial.println("5....");

    /*---- Nextion Init ---- 2*/
    Serial.println(sp_Up.attachCallback(&callback));
    Serial.println(sp_Down.attachCallback(&callback));
    Serial.println(hs_OnOff.attachCallback(&OnOffCallback));
    Serial.println(hs_reset.attachCallback(&HS_Reset));

    Serial.println(hs_Diagnostics0.attachCallback(&OpenDiagScreen));
    Serial.println(hs_Diagnostics1.attachCallback(&OpenDiagScreen));
    Serial.println(hs_Diagnostics3.attachCallback(&OpenDiagScreen));

    Serial.println(hs_Systems0.attachCallback(&OpenSystemsScreen));
    Serial.println(hs_Systems1.attachCallback(&OpenSystemsScreen));
    Serial.println(hs_Systems2.attachCallback(&OpenSystemsScreen));

    SetDefaultsStartup();
    /*---- WiFi Init ----*/
    WiFiManager wifiManager;
    wifiManager.autoConnect("AutoConnectAP");
    Serial.println("WiFi....");

    Cayenne.begin(username, password, clientID);

    /*---- NPT Init ----*/
    static char respBuf[4096];
    Udp.begin(localPort);
    Serial.println("waiting for sync");
    setSyncProvider(getNtpTime);  //Get Local Date/Time
    setSyncInterval(14400);
    setTime(hour(), minute(), second(), day(), month(), year()); //Set Local Date/Time to Alarm lib
    Serial.println("Time set....");
    bootTime = now();
    Serial.println(digitalClockDisplay());
    /*---- WUnderground Init ----*/
    //GetWUnderground();
    //Serial.println("Weather Underground....");
    GetNetworkData();
    Serial.println("Network Data....");
    /*---- WebServer Init ----*/
    server.on("/", handle_root);
    server.on("/SpanishTest", []() { // if you add this subdirectory to your webserver call, you get text below :)
      webString = SpanishTest();
      AlexaTime = now();
      server.send(200, "text/plain", webString);            // send to someones browser when asked
    });
    server.on("/on", []() { // if you add this subdirectory to your webserver call, you get text below :)
      TurnOn();
      AlexaTime = now();
      webString = "The thermostat has been turned on";
      server.send(200, "text/plain", webString);            // send to someones browser when asked
      CheckThermo();
    });
    server.on("/off", []() { // if you add this subdirectory to your webserver call, you get text below :)
      TurnOff();
      AlexaTime = now();
      webString = "The thermostat has been turned off";
      server.send(200, "text/plain", webString);            // send to someones browser when asked
      CheckThermo();
    });
    server.on("/RoomTemp", []() { // if you add this subdirectory to your webserver call, you get text below :)
      AlexaTime = now();
      webString = "The room temperature is " + String(RoomTem) + " degrees and the thermostat is set to " + String(SetTemp) + ".";
      server.send(200, "text/plain", webString);            // send to someones browser when asked
      CheckThermo();
    });
    server.on("/SetTempSet", []() { // if you add this subdirectory to your webserver call, you get text below :)
      SetTemp = server.arg(0).toInt();
      AlexaTime = now();
      txtSetTemp.setValue(SetTemp);
      CheckThermo();
      webString = "The temperature has been changed to  " + String(SetTemp) + " degrees.";
      server.send(200, "text/plain", webString);            // send to someones browser when asked
    });
    server.on("/GetThermostatSeting", []() { // if you add this subdirectory to your webserver call, you get text below :)
      AlexaTime = now();
      webString = "The room temperature is " + String(RoomTem) + " degrees and the thermostat is set to " + String(SetTemp) + " degrees" ;
      server.send(200, "text/plain", webString);            // send to someones browser when asked
      CheckThermo();
    });
    server.on("/Diag", []() { // if you add this subdirectory to your webserver call, you get text below :)
      webString = "" ;
      AlexaTime = now();
      webString = Diagnostics();
      server.send(200, "text/plain", webString);            // send to someones browser when asked
    });
    server.on("/uptime", []() { // if you add this subdirectory to your webserver call, you get text below :)
      AlexaTime = now();
      webString = "The thermostat has been operational for " + GetUpTime(true);
      server.send(200, "text/plain", webString);            // send to someones browser when asked
    });
    server.begin();
    Serial.println("HTTP server started");
    /*---- Alarm/Repeat Init ----*/
    Alarm.timerRepeat(30, CheckThermo);
    Alarm.timerRepeat(1, SetNextionDateTime);
    //Alarm.timerRepeat(300,GetWUnderground);
    //Alarm.timerRepeat(86400,RebootESP);
    nex.sendCommand("vis pBooting,0");
    Alarm.delay(50);
    nex.sendCommand("vis pRunning,1");
    CheckThermo();
    varRunning.setValue(1);
    Alarm.delay(50);
    Serial.println("Setup complete");
    PowerOnLED(OnLED,HIGH);
  }
  void loop() {
    Alarm.delay(50);
    server.handleClient();
    Alarm.delay(50);
    nex.poll();
  }

