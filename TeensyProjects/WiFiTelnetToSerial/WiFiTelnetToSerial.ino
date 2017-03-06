/*
  WiFiTelnetToSerial - Example Transparent UART to Telnet Server for esp8266

  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the ESP8266WiFi library for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

char ssid[32] = "Hilton";
char password[32] = "gb7bpqgb7bpq";

const char *apssid = "ESPcfg";		// This doesn't seem to work!!!
const char *appassword = "BPQ";

WiFiServer server(8105);
WiFiClient serverClient;

ESP8266WebServer webserver(80);		// For configuration/Status

void setup()
{
  Serial.begin(115200);

  // Set SSID and Password from EEPROM

  EEPROM.begin(128);

  //SaveConfig();

  if (EEPROM.read(0) == 0x55 && EEPROM.read(1) == 0xAA)
  {
    // Value has benn set

    int i;
    for (i = 0; i < 31; i++)
    {
      ssid[i] = EEPROM.read(i + 2);
      password[i] = EEPROM.read(i + 34);
    }
  }
  else
  {
    Serial.println("\nSSID and Password not in EEPROM");
  }

  // This should cconfigure the access point with our ssid and password, but doesn'r

  WiFi.softAPdisconnect(true);
  WiFi.disconnect();
  delay(100);
  WiFi.mode(WIFI_AP_STA);
  delay(100);
  WiFi.softAP(apssid, appassword);
  WiFi.begin(ssid, password);

  webserver.on("/", handleRoot);
  webserver.on("/config", handleConfig);
  webserver.onNotFound(handleNotFound);
  webserver.begin();

  Serial.printf("\nConnecting to *%s*%s*\n", ssid, password);

  uint8_t i = 0;

  while (WiFi.status() != WL_CONNECTED && i++ < 20) delay(500);

  if (i == 21)
  {
    Serial.print("Could not connect to "); Serial.println(ssid);
    while (1)
    {
      webserver.handleClient();
      delay(5);
      // Should we try again if this fails??
    }
  }

  //start UART and the TCP server

  server.begin();
  server.setNoDelay(true);

  Serial.print("Ready! Use 'telnet ");
  Serial.print(WiFi.localIP());
  Serial.println(" 8105' to connect");
}

void loop()
{
  uint8_t i;

  webserver.handleClient();

  //check if there are any new clients

  // is it better to close an old client??

  if (server.hasClient())
  {
    if (!serverClient || !serverClient.connected())
    {
      if (serverClient)
        serverClient.stop();

      serverClient = server.available();
      Serial.print("New client");
    }
    else
    {
      // already connected so reject

      WiFiClient serverClient = server.available();
      serverClient.stop();
    }

    //check client for data

    if (serverClient && serverClient.connected())
    {
      if (serverClient.available())
      {
        //get data from the telnet client and push it to the UART
        while (serverClient.available())
          Serial.write(serverClient.read());
      }
    }
  }
  //check UART for data

  if (Serial.available())
  {
    size_t tot = 1024;
    uint8_t sbuf[tot];
    tot = 0;

    // try to block data or tcp delays can give serial port overrun

    while (Serial.available())
    {
      size_t len = Serial.available();

      if (len > 1024 - tot)
        break;

      Serial.readBytes(&sbuf[tot], len);
      tot += len;
      delay (1);
    }

    // push UART data to telnet client if connected

    if (serverClient && serverClient.connected())
      serverClient.write(sbuf, tot);
  }
}

void handleRoot()
{
  String IP;
  IP =  String(WiFi.localIP()[0]) + "." + String(WiFi.localIP()[1]) + "." + String(WiFi.localIP()[2]) + "." + String(WiFi.localIP()[3]);

  String content = "<html><body><form action='/config' method='POST'>Configure ARDOP/ESP Interface<br>";
  content += "<vr><br>SSID: &nbsp; &nbsp; &nbsp; <input type='text' name='SSID' placeholder=''>";
  content += " &nbsp; &nbsp; Password: <input type='password' name='PASSWORD' placeholder=''><br>";
  content += "<br><br><input type='submit' name='Save' value='Save'>";
  content += "  &nbsp; <input type='submit' name='Reconnect' value='Reconnect'>";
  content += "<br><br>IP Address = " + IP + "</form><br>";
  content += "</body></html>";

  webserver.send(200, "text/html", content);

}

void handleConfig()
{
  String message = "SSID and Password Updated\n\n";
  char Action[32];

  message += "URI: ";
  message += webserver.uri();
  message += "\nMethod: ";
  message += (webserver.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += webserver.args();
  message += "\n";
  for (uint8_t i = 0; i < webserver.args(); i++) {
    message += " " + webserver.argName(i) + ": " + webserver.arg(i) + "\n";
  }

  webserver.arg(2).getBytes((unsigned char *)Action, 32);

  if (strcmp(Action, "Reconnect") == 0)
  {
    WiFi.begin(ssid, password);
    message = "Reconnecting..\n\n";
  }
  else if (strcmp(Action, "Save") == 0)
  {
    webserver.arg(0).getBytes((unsigned char *)ssid, 32);
    webserver.arg(1).getBytes((unsigned char *)password, 32);
    SaveConfig();
  }
  else
    message = Action;

  webserver.send(200, "text/plain", message);
}

void SaveConfig()
{
  int i;
  for (i = 0; i < 31; i++)
  {
    EEPROM.write(i + 2, ssid[i]);
    EEPROM.write(i + 34, password[i]);
  }
  EEPROM.write(0, 0x55);
  EEPROM.write(1, 0xAA);
  EEPROM.commit();
}


void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += webserver.uri();
  message += "\nMethod: ";
  message += (webserver.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += webserver.args();
  message += "\n";
  for (uint8_t i = 0; i < webserver.args(); i++) {
    message += " " + webserver.argName(i) + ": " + webserver.arg(i) + "\n";
  }
  webserver.send(404, "text/plain", message);
}

