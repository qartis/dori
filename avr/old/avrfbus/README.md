AVR with Nokia FBUS
=============

This code connects an AVR atmega88 with a Nokia 3310 mobile phone and any NMEA-compliant GPS receiver, to create a cheap GPS tracker. Send it an SMS, and it will reply with its current GPS coordinates.

Features
----------
By using scavenged parts, the cost of this project can be very low. The most expensive component will be the GPS receiver. Old Nokia 3310 phones are very cheap online. I bought one for $5. These phones (Nokia 3310/6110) are what suicide bombers put into their vests, because they're essentially disposable and very easy to interface with.

Hardware
---
Start off by gutting the phone. We can store the LCD for use in a later project (there are many articles online that explain how to interface with it). Solder a single wire to the power button's active lead. This will let the AVR hard-reset the phone if it ever needs to.

The AVR that I used has only one hardware UART, so I made a hardware multiplexer to allow the chip to communicate with the GPS and the phone using the same two wires. The GPS's TX and the phone's TX both feed through a PNP and NPN transistor, respectively. This lets the AVR selectively receive either signal depending on the state of a single control pin. You don't have to multiplex the AVR's TX, because most GPS devices don't need any configuration so you can connect the GPS's RX to GND.

Description
----------
The AVR's logic is simple. It continually waits for an SMS to be received by the phone. When a message arrives it wakes up and parses the message contents. If the SMS is simply the word 'ping', then the AVR sends the reply 'pong' back to the sender. For any other incoming SMS, the AVR activates the GPS module, captures a full NMEA position sentence, and sends the extracted coordinates back to the sender. It also has a heartbeat function, which sends a simple FBUS command to the phone once an hour to make sure the phone is still actively listening for new text messages. If the phone fails to respond to the heartbeat, the AVR power-cycles it and tries again.

Originally I had tried to keep the overall power draw as low as possible by putting the AVR into its lowest-power sleep mode while waiting for new messages. A timer also needs to run in order to wake the AVR up once per hour to send the phone a heartbeat packet.  However the only onboard timer that runs while the chip is in the POWER_DOWN state is the watchdog timer, which has a maximum interval of about 8s (1/18432000 seconds per cycle / 64 prescaler * 2^16 counter ticks). This means the AVR would be waking up and falling back asleep every 8s until an hour has passed. I had added a simple 60-minute timer using a TS555 and a large capacitor to run while the chip is asleep. This had much less power draw, but higher hardware complexity and more things to go wrong. Eventually I decided that reliability was more important than battery life, so the code now uses the watchdog timer. The file ts555.c (now deleted) was written to trigger the 555 timer.

Note on Phone Hardware
---
This project was built around the Nokia 3310. Similar Nokia phones use slightly different dialects of the FBUS protocol, so the file fbus.c will probably need to be tweaked.

You could also use a cell phone with a built-in software modem using the Hayes AT command set. Most phones that ship with a serial data cable use AT. In this case you can remove fbus.c and add AT handling routines.

License
----------
Some of the FBUS handling code (fbus.c) is from n61sms.c linked below. I don't know its license, but everything except for fbus.c is released to the public domain.

Credits
----------
http://www.embedtronics.com/nokia/fbus.html
The most well-known reference for the FBUS protocol.

http://www.affinix.com/~justin/nokia/n61sms.c
Example FBUS code written by Justin Karneges. License unknown.

http://www.gnokii.org/
The grandfather of most FBUS reverse-engineering efforts. GPL licensed.
