// Visual Micro is in vMicro>General>Tutorial Mode
// 
/*
	Name:       sensor_read.ino
	Created:	05.06.2020 01:16:19
	Author:     DESKTOP-LF3T8UB\Simon
*/

// Define User Types below here or use a .h file
//


// Define Function Prototypes that use User Types below here or use a .h file
//


// Define Functions below here or use other .ino or cpp files
//

void setup() {
	Serial.begin(115200); // open serial port, set the baud rate as 9600 bps
}
void loop() {
	int val;
	val = analogRead(0); //connect sensor to Analog 0
	Serial.println(val); //print the value to serial port
	delay(500);
}
