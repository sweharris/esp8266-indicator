# esp8266-indicator

This is a small piece of code to solve a simple problem.

In the past I've occasionally gone to my garage to find the light has been
left one.  It may have been on for a whole week, if not longer, because
I don't go to the garage very often.  And because the doors are solid, I
can't see the light state from the house.

One of the light sockets is a combined light/power.  So I put a simple
D1 Mini device powered from that socket.  If the light is on, the Mini
powers up and joins the network.

This makes it simple to write a simple "ping" test that updates MQTT

    #!/bin/ksh

    while [ 1 ]
    do
      ping garage-ping -c 5 > /dev/null 2>&1
      if [ $? == 0 ]
      then
        stat=ON
      else
        stat=OFF
      fi
      /home/sweh/bin/mqttcli pub -r -t garage-power -m $stat
    done

I could potentially do this with the Mini directly writing to the queue,
but then I'd need to have some sort of "aging" mechanism; if the last
message on the queue was over 10 seconds ago then assume it was off.  This
is simpler, and can run on the same server as the MQTT broker.

So now this code can just read the queue.  If the message is ON then turn
on the LED.  If the message is OFF then turn it off.  Simple!

In this way the state of the LED on this sketch should mirror the state of
the switch in the garage and now I get visual feedback.

This can easily be used to drive an external LED just by changing the PIN
we set up.

You will need to make a file `network_conn.h` with contents similar to

    const char* ssid       = "your-ssid";
    const char* password   = "your-wpa-passwd";
    const char* mqttServer = "your-MQTT-server";
    const int mqttPort     = 1883;

in order to provide details of your network and MQTT server

The MQTT channel is hard coded to "garage-power"
   
Uses PubSubClient library on top of the ESP8266WiFi one
   
GPL2.0 or later.  See GPL LICENSE file for details.
Original code by Stephen Harris, April 2020
