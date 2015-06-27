/*
 * Web_Temp_Gauge.ino
 *
 * Created: 6/4/2015 7:40:21 PM
 * Author: Micke
 */ 

#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <MickeDallasTemperature.h>
#include <MickeDebug.h>

//The pin that Dallas Temp Probe is attached to
#define ONE_WIRE_CONTROL_PIN 2

// size of buffer used to capture HTTP requests
#define REQ_BUF_SZ   50

MickeDebug mDebug(100);

//Temp reading objects
OneWire oWire(ONE_WIRE_CONTROL_PIN);
MickeDallasTemperature mDallasTemp(&oWire);

float temperature;			//Stores current Temperature


// MAC address from Ethernet shield sticker under board
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 99); // IP address, may need to change depending on network
EthernetServer server(80);  // create a server at port 80
File webFile;               // the web page file on the SD card
char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
char req_index = 0;              // index into HTTP_req buffer

void setup()
{
	Serial.begin(9600);       // for debugging
	
	// disable Ethernet chip and SD Card
	pinMode(10, OUTPUT);
	digitalWrite(10, HIGH);
	pinMode(4, OUTPUT);
	digitalWrite(4, HIGH);


	
	// initialize SD card
	Serial.println("Initializing SD card...");
	if (!SD.begin(4)) {
		Serial.println("ERROR - SD card initialization failed!");
		return;    // init failed
	}
	Serial.println("SUCCESS - SD card initialized.");
	// check for index.htm file
	if (!SD.exists("index.htm")) {
		Serial.println("ERROR - Can't find index.htm file!");
		return;  // can't find index file
	}
	Serial.println("SUCCESS - Found index.htm file.");
	
	//Running with DHCP
	//Ethernet.begin(mac);

	//Run with fixed IP
	Ethernet.begin(mac, ip);  // initialize Ethernet device with Fixed IP
	
	// print your local IP address:
	Serial.print("My IP address: ");
	for (byte thisByte = 0; thisByte < 4; thisByte++) {
		// print the value of each byte of the IP address:
		Serial.print(Ethernet.localIP()[thisByte], DEC);
		Serial.print(".");
	}
	Serial.println(" ");	mDallasTemp.InitSensors();	//Try to get and print one temp
	temperature = mDallasTemp.getSensorTempC(0);
	Serial.print("Temp = ");
	Serial.println(temperature);		Serial.println("Setup done!");
	
	server.begin();           // start to listen for clients
}

void loop()
{
	EthernetClient client = server.available();  // try to get client

	if (client) {  // got client?
		boolean currentLineIsBlank = true;
		while (client.connected()) {
			if (client.available()) {   // client data available to read
				char c = client.read(); // read 1 byte (character) from client
				// buffer first part of HTTP request in HTTP_req array (string)
				// leave last element in array as 0 to null terminate string (REQ_BUF_SZ - 1)
				if (req_index < (REQ_BUF_SZ - 1)) {
					HTTP_req[req_index] = c;          // save HTTP request character
					req_index++;
				}
				// last line of client request is blank and ends with \n
				// respond to client only after last line received
				if (c == '\n' && currentLineIsBlank) {
					// send a standard http response header
					client.println("HTTP/1.1 200 OK");
					// remainder of header follows below, depending on if
					// web page or XML page is requested
					// Ajax request - send XML file
					if (StrContains(HTTP_req, "ajax_inputs")) {
						// send rest of HTTP header
						client.println("Content-Type: text/xml");
						client.println("Connection: keep-alive");
						client.println();
						// send XML file containing input states
						XML_response(client);
					}
					else {  // web page request
						// send rest of HTTP header
						client.println("Content-Type: text/html");
						client.println("Connection: keep-alive");
						client.println();
						// send web page
						webFile = SD.open("index.htm");        // open web page file
						if (webFile) {
							while(webFile.available()) {
								client.write(webFile.read()); // send web page to client
							}
							webFile.close();
						}
					}
					// display received HTTP request on serial port
					Serial.print(HTTP_req);
					// reset buffer index and all buffer elements to 0
					req_index = 0;
					StrClear(HTTP_req, REQ_BUF_SZ);
					break;
				}
				// every line of text received from the client ends with \r\n
				if (c == '\n') {
					// last character on line of received text
					// starting new line with next character read
					currentLineIsBlank = true;
				}
				else if (c != '\r') {
					// a text character was received from client
					currentLineIsBlank = false;
				}
			} // end if (client.available())
		} // end while (client.connected())
		delay(1);      // give the web browser time to receive the data
		client.stop(); // close the connection
	} // end if (client)
}

// send the XML file containing analog value
void XML_response(EthernetClient cl)
{
	temperature = mDallasTemp.getSensorTempC(0);
	//int analog_val
	 float analog_val;
	
	cl.print("<?xml version = \"1.0\" ?>");
	cl.print("<inputs>");
	// read analog pin A2
	//analog_val = analogRead(2);
	analog_val = temperature;
	cl.print("<analog>");
	cl.print(analog_val);
	cl.print("</analog>");
	cl.print("</inputs>");
}

// sets every element of str to 0 (clears array)
void StrClear(char *str, char length)
{
	for (int i = 0; i < length; i++) {
		str[i] = 0;
	}
}

// searches for the string sfind in the string str
// returns 1 if string found
// returns 0 if string not found
char StrContains(char *str, char *sfind)
{
	char found = 0;
	char index = 0;
	char len;

	len = strlen(str);
	
	if (strlen(sfind) > len) {
		return 0;
	}
	while (index < len) {
		if (str[index] == sfind[found]) {
			found++;
			if (strlen(sfind) == found) {
				return 1;
			}
		}
		else {
			found = 0;
		}
		index++;
	}

	return 0;
}