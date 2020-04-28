// This is a small piece of code to solve a simple problem.
// 
// In the past I've occasionally gone to my garage to find the light has been
// left one.  It may have been on for a whole week, if not longer, because
// I don't go to the garage very often.  And because the doors are solid, I
// can't see the light state from the house.
// 
// One of the light sockets is a combined light/power.  So I put a simple
// D1 Mini device powered from that socket.  If the light is on, the Mini
// powers up and joins the network.
// 
// This makes it simple to write a simple "ping" test that updates MQTT
// 
//     #!/bin/ksh
// 
//     while [ 1 ]
//     do
//       ping garage-ping -c 5 > /dev/null 2>&1
//       if [ $? == 0 ]
//       then
//         stat=ON
//       else
//         stat=OFF
//       fi
//       /home/sweh/bin/mqttcli pub -r -t garage-power -m $stat
//     done
// 
// I could potentially do this with the Mini directly writing to the queue,
// but then I'd need to have some sort of "aging" mechanism; if the last
// message on the queue was over 10 seconds ago then assume it was off.  This
// is simpler, and can run on the same server as the MQTT broker.
// 
// So now this code can just read the queue.  If the message is ON then turn
// on the LED.  If the message is OFF then turn it off.  Simple!
// 
// In this way the state of the LED on this sketch should mirror the state of
// the switch in the garage and now I get visual feedback.
// 
// This can easily be used to drive an external LED just by changing the PIN
// we set up.
// 
// You will need to make a file `network_conn.h` with contents similar to
// 
//     const char* ssid       = "your-ssid";
//     const char* password   = "your-wpa-passwd";
//     const char* mqttServer = "your-MQTT-server";
//     const int mqttPort     = 1883;
// 
// in order to provide details of your network and MQTT server
// 
// The MQTT channel is hard coded to "garage-power"
//    
// Uses PubSubClient library on top of the ESP8266WiFi one
//    
// GPL2.0 or later.  See GPL LICENSE file for details.
// Original code by Stephen Harris, April 2020

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// This include file needs to define the following
//   const char* ssid       = "your-ssid";
//   const char* password   = "your-password";
//   const char* mqttServer = "your-mqtt-server";
//   const int   mqttPort   = 1883;
// I did it this way so I can ignore the file from git
#include "network_conn.h"

// Onboard LED is on Pin 13
#define LEDPIN LED_BUILTIN
#define mqttChannel    "garage-power"

WiFiClient espClient;
PubSubClient client(espClient);

// Only makes sense to call this after the WiFi has connected and time determined.
void log_msg(String msg)
{
  time_t now = time(nullptr);
  String tm = ctime(&now);
  tm.trim();
  tm = tm + ": " + msg;
  Serial.println(tm);
}

// This is called via the main loop when a message appears on the MQTT queue
void callback(char* topic, byte* payload, unsigned int length)
{
  int state=HIGH;

  // Convert the message to a string.  We can't use a simple constructor 'cos the
  // byte array isn't null terminated
  String msg;
  for (int i = 0; i < length; i++)
  {
    msg += char(payload[i]);
  }
  log_msg("Message arrived [" + String(topic) + "] " + msg);

  // Switch the LED on/off
  if (msg == "ON")
  {
    state=LOW;
  }
  digitalWrite(LEDPIN,state);
}

void setup()
{
  pinMode(LEDPIN, OUTPUT);
  Serial.begin(115200);

  // Let's create the channel names based on the MAC address
  unsigned char mac[6];
  char macstr[7];
  WiFi.macAddress(mac);
  sprintf(macstr, "%02X%02X%02X", mac[3], mac[4], mac[5]);

  delay(500);

  Serial.println();
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  WiFi.mode(WIFI_STA);  // Ensure we're in station mode and never start AP
  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.print(": ");

  // Wait for the Wi-Fi to connect
  int i = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(++i);
    Serial.print(' ');
  }

  Serial.println();
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());

  // Get the current time.  Initial log lines may be wrong 'cos
  // it's not instant.  Timezone handling... eh, this is just for
  // debuging, so I don't care.  GMT is always good :-)
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  delay(1000);

  // Now we're on the network, setup the MQTT client
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

#ifdef NETWORK_UPDATE
    __setup_updater();
#endif

}


void loop()
{
  // Try to reconnect to MQTT each time around the loop, in case we disconnect
  while (!client.connected())
  {
    log_msg("Connecting to MQTT Server " + String(mqttServer));

    // Generate a random ID each time
    String clientId = "ESP8266Client-indicator-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      log_msg("MQTT connected.  I am " + WiFi.localIP().toString());
      client.subscribe(mqttChannel);
    } else {
      log_msg("failed with state " + client.state());
      delay(2000);
    }
  }

  // Check the MQTT queue for messages
  client.loop();

#ifdef NETWORK_UPDATE
  __netupdateServer.handleClient();
#endif
}
