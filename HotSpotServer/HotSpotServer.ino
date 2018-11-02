


/*******************************
  Terry Clarkson & Adam Clarkson
  11/02/18
*******************************/


//------ EEPROM addresses --------------------------------
//  line 1 -      0 to 49  49 bytes
//  line 2 -      50 to 99  49 bytes
//  line 3 -      100 to 149  49 bytes
//  line 4 -      150 to 199 49 bytes
//  checkbox1 -   200
//  checkbox2 -   201
//  checkbox3 -   202
//  checkbox4 -   203
//  checkbox5 -   204


//------------------Include files ------------------------------------------
#include <WiFi.h>                                 // Load Wi-Fi library
#include <Arduino.h>
#include <U8g2lib.h>                              //driver for oled display
#include <string.h>
#include <EEPROM.h>                               //driver for eeprom
//------ files for sd card ------------------------
#include <Update.h>
#include <FS.h>
#include <SD_MMC.h>

#define EEPROM_SIZE 2048                          //rom reserved for eeprom storage
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#define RXD2 16                               //port 2 serial pins for external printer
#define TXD2 17
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);     //identify pins used for oled display


//----------------- Replace with your network credentials ----------------------------------
const char* ssid     = "ProTournament";
const char* password = "123456789";
//--------------------------------------------------------------------------------------------

WiFiServer server(80);                                // Set web server port number to 80

//-----------------Define varibles------------------------------------------------------------
String header;                                        // Variable to store the HTTP request
String save_header;
String line1 = "";
String line2 = "";
String line3= "";
String line4 = "";
char temp_str1[30];
char temp_str2[30];
char temp_str3[30];
char temp_str4[30];
int statt;      //1 = h2 lb   2= h2 lb/oz     3 = 357 lb     4 = 357 lb/oz
int serial_number;
char output_string[31];                                  //converted data to send out
char temp_str[30];
String temp_val = "";
char weight[15];
bool checkbox1_is_checked;
bool checkbox2_is_checked;
bool checkbox3_is_checked;
bool checkbox4_is_checked;
bool checkbox5_is_checked;
volatile int ticket;

String checkbox1_status = "";
String checkbox2_status = "";
String checkbox3_status = "";
String checkbox4_status = "";
String checkbox5_status = "";
volatile int interruptCounter;                              //varible that tracks number of interupts
int totalInterruptCounter;                                  //counter to track total number of interrupts

hw_timer_t * timer = NULL;                                 //in order to configure the timer, we will need
                                                           //a pointer to a variable of type hw_timer_t,
                                                           //which we will later use in the Arduino setup function.


portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;      //we will need to declare a variable of type portMUX_TYPE,
                                                           //which we will use to take care of the synchronization
                                                           //between the main loop and the ISR, when modifying a shared variable.

// setting variables
String settingsCheck1_status = "";
String settingsCheck3_status = "";
String settingsCheck2_status = "";
void settingsPageForm();
void mainPageForm();
bool is_settings = false;

// Assign eeprom save addresses
const int line1_eeprom_addr = 0;
const int line2_eeprom_addr = 50;
const int line3_eeprom_addr = 100;
const int line4_eeprom_addr = 150;

const int checkbox1_eeprom_addr= 200;
const int checkbox2_eeprom_addr= 201;
const int checkbox3_eeprom_addr= 202;
const int checkbox4_eeprom_addr= 203;
const int serial_number_addr = 204;



//----------Define routines -----------------------------------------
String char_replace_http(String str);                   //define routine
void display_graphic(void);
void clear_232_buffer(void);
void convert_string(void);
void clear_output_buffer(void);
void clear_radio_buffer(void);
void print_ticket(void);

//-------------Interuput routines ----------------------------------------
void IRAM_ATTR onTimer()                                  //this is the actual interrupt(place before void setup() code)
  {
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounter++;                                      //put code to perform during interrupt here
  portEXIT_CRITICAL_ISR(&timerMux);
  }

char *database[100][2];                                               //database array to hold anglers name and weight

//--------------------------------------------------------------------------
//-------------Start of Program -----------------------------------------
//------------------------------------------------------------------------
void setup() {

    //---setup 1us counter---------
    timer = timerBegin(0, 80, true);                     //"0" is the timer to use, '80' is the prescaler,true counts up 80mhz divided by 80 = 1 mhz or 1 usec
    timerAttachInterrupt(timer,&onTimer,true);            //"&onTimer" is the int function to call when intrrupt occurs,"true" is edge interupted
    timerAlarmWrite(timer, 1000000, true);                //interupt every 1000000 times
    timerAlarmEnable(timer);                              //this line enables the timer declared 3 lines up and starts it

    pinMode(4,INPUT_PULLUP);                                    //set pin 4 as the pushbutton input to print with pullup
    pinMode(2,OUTPUT);                                        //this is the other side of pushbutton
    digitalWrite(2,LOW);                                        //other side of print pushbutton for test board keep low

    u8g2.begin();                                               //start up oled display
    u8g2.clearBuffer();                                         //clear oled buffer
    u8g2.setFont(u8g2_font_ncenB08_tr);                         // roman style 8 pixel
    u8g2.sendBuffer();                                          //clear display
    ticket = 0;
   // Note the format for setting a serial port is as follows: Serial2.begin(baud-rate, protocol, RX pin, TX pin);
   Serial2.begin(9600, SERIAL_8N1,RXD2, TXD2);             //serial port 2 for thermal printer TX = pin 17 RX = pin 2
   Serial.begin(115200);                                    //start serial port 0 (debug monitor and programming port)
   if (!EEPROM.begin(EEPROM_SIZE))                        //set aside memory for eeprom size
       {
       Serial.println("failed to intialise EEPROM");
       }
   //Serial2.print("THis is a test of serial port 2");



  Serial.print("Setting AP (Access Point)…\n");                         // Connect to Wi-Fi network with SSID and password
  // Remove the password parameter, if you want the AP (Access Point) to be open
  if (!digitalRead(4))                                                  // of print button is held down during power up
      {
      Serial.println("password = 987654321");
      u8g2.clearBuffer();
      u8g2.drawStr(3,10,"Temporary Password");                          //display temp password on oled display
      u8g2.drawStr(3,18,"987654321");
      u8g2.sendBuffer();
      while(!digitalRead(4))                                              //loop until button is released
           {delay(50);}
      WiFi.softAP(ssid,"987654321");
      }
  else
      {WiFi.softAP(ssid,password);                                        //this was declared as constant above
      Serial.println("password = 123456789");}

        //.softAP(const char* ssid, const char* password, int channel, int ssid_hidden, int max_connection)

  IPAddress IP = WiFi.softAPIP();                                       //get the ip address
  Serial.print("AP IP address: ");
  Serial.println(IP);



  server.begin();

  u8g2.clearBuffer();
  u8g2.drawStr(3,10,"SSID = ProTournament");                            // write something to the internal memory
  char ip_string[30];                                                   //declare a character array
  sprintf(ip_string,"IP = %d.%d.%d.%d",WiFi.softAPIP()[0],WiFi.softAPIP()[1],WiFi.softAPIP()[2],WiFi.softAPIP()[3]);   //this creates the ip address format to print (192.169.4.1)
  u8g2.drawStr(3,28,ip_string);                                         //display ip value on oled
  u8g2.sendBuffer();                                                    //transfer buffer value to screen
  delay(5000);
               line1 = (EEPROM.readString(line1_eeprom_addr));              //recall values saved in eeprom
               line2 = (EEPROM.readString(line2_eeprom_addr));
               line3 = (EEPROM.readString(line3_eeprom_addr));
               line4 = (EEPROM.readString(line4_eeprom_addr));
               serial_number = EEPROM.readUInt(serial_number_addr);

               checkbox1_is_checked = (EEPROM.readBool(checkbox1_eeprom_addr));  //recall checkbox status
               checkbox2_is_checked = (EEPROM.readBool(checkbox2_eeprom_addr));
               checkbox3_is_checked = (EEPROM.readBool(checkbox3_eeprom_addr));
               checkbox4_is_checked = (EEPROM.readBool(checkbox4_eeprom_addr));
//         Serial.println("------------eeprom values ---------------------");
//         Serial.println(checkbox1_is_checked ? "True" :"False" );
//         Serial.println(checkbox2_is_checked ? "True" :"False" );
//         Serial.println(checkbox3_is_checked ? "True" :"False" );
//         Serial.println(checkbox4_is_checked ? "True" :"False" );
                checkbox1_is_checked ? checkbox1_status = "checked" : checkbox1_status = "";    //set 'checkbox#_is_checked' to match 'checkbox#_status'
                checkbox2_is_checked ? checkbox2_status = "checked" : checkbox2_status = "";
                checkbox3_is_checked ? checkbox3_status = "checked" : checkbox3_status = "";
                checkbox4_is_checked ? checkbox4_status = "checked" : checkbox4_status = "";

               line1.toCharArray(temp_str1,30);                               //must convert string to a character array for drawStr() to work
               line2.toCharArray(temp_str2,30);
               line3.toCharArray(temp_str3,30);
               line4.toCharArray(temp_str4,30);

                // Writing to OLED display
               u8g2.clearBuffer();
               u8g2.drawStr(3,8,temp_str1);                                   //send line 1 to oled display
               u8g2.drawStr(3,18,temp_str2);
               u8g2.drawStr(3,28,temp_str3);
               u8g2.drawStr(3,48,temp_str4);
               u8g2.sendBuffer();
}
//&&&&&&&&&&&&&&&&&&&&&&&&&   Start of Program Loop  &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
void loop(){

   if (interruptCounter > 0)                           //every one second 1 sec int is generated
      {
      portENTER_CRITICAL(&timerMux);
      interruptCounter--;                                  //reset counter to zero
      portEXIT_CRITICAL(&timerMux);
      totalInterruptCounter++;                              //increment counter for ints generated every second
      //Serial2.print("An interrupt as occurred. Total number: ");
      //Serial2.println(totalInterruptCounter);
     }

   if (totalInterruptCounter >=3)                       //if no signal this timer times out
            {statt = 0;                               //set display mode to 0 so weights do not print
             totalInterruptCounter = 0;              //reset counter
            }
   //--------------- push button routine -------------------------------------------------------
      if (!digitalRead(4))                           //if pushbutton is pressed (low condition), print the ticket
      { print_ticket();                              //print the weight ticket

        delay(300);
        if (checkbox1_status == "checked")           //if checkbox "print 2 tickets" is checked
        {print_ticket();}                           //print second ticket if print 2 copies is selected
        while (!digitalRead(4))                      //loop while button is held down
            {delay(300);}
        serial_number++;                             //increment serial number
        EEPROM.writeUInt(serial_number_addr,serial_number); //save to eeprom
      }



//    while (Serial2.available())                  //if data on recieve buffer, send to serial monitor
//        {Serial.print(char(Serial2.read()));}    //send serial 2 input data to serial monitor





  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port monitor
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
      //      client.println("Refresh: 5");                                        //refresh browser screen every 5 seconds
            client.println();

      //-----read the returned information from form when submit button is pressed and save to memory--------
            String headerT = header.substring(0,header.indexOf("Host:"));                          //create substring from begining to the word 'Host:'

            Serial.println("headerT:");                                                            //print substring to serial monitor
            Serial.println(headerT);
            if ((headerT.indexOf("settings?") >= 0)&& !(header.indexOf("favicon") >= 0))
            {
              is_settings = true;
            }
            else {
              is_settings = false;
            }
            if ((headerT.indexOf("Line1=") >= 0)&& !(header.indexOf("favicon") >= 0))                //if text 'Line1=' is found and text 'favicon' is not found
               {Serial.println("********found it (screen one) *********************************************");
               line1 =  header.substring(header.indexOf("Line1=")+6,header.indexOf("&Line2="));        //parse out the varible strings for the the 4 lines
               line2 =  header.substring(header.indexOf("Line2=")+6,header.indexOf("&Line3="));
               line3 =  header.substring(header.indexOf("Line3=")+6,header.indexOf("&Line4="));
               // Check if line 4 is end of data or if checkbox info follows
               if (headerT.indexOf("&check") >= 0) {
                line4 =  headerT.substring(headerT.indexOf("Line4=")+6,headerT.indexOf("&check"));
               } else {

                line4 =  headerT.substring(headerT.indexOf("Line4=")+6,headerT.indexOf(" HTTP"));
               }
               // Check if checkbox1 is checked

               if (headerT.indexOf("&checkbox1") >= 0) {
                checkbox1_is_checked = true;
                checkbox1_status = "checked";
               } else {
                checkbox1_is_checked = false;
                checkbox1_status = "";
               }


               if (headerT.indexOf("&checkbox2") >= 0) {
                checkbox2_is_checked = true;
                checkbox2_status = "checked";
               } else {
                checkbox2_is_checked = false;
                checkbox2_status = "not_checked";
               }


               if (headerT.indexOf("&checkbox3") >= 0) {
                checkbox3_is_checked = true;
                checkbox3_status = "checked";
               } else {
                checkbox3_is_checked = false;
                checkbox3_status = "not_checked";
               }


               if (headerT.indexOf("&checkbox4") >= 0) {
                checkbox4_is_checked = true;
                checkbox4_status = "checked";
               } else {
                checkbox4_is_checked = false;
                checkbox4_status = "not_checked";
               }

               line1 = char_replace_http(line1);                     //remove and replace http characters
               line2 = char_replace_http(line2);
               line3 = char_replace_http(line3);
               line4 = char_replace_http(line4);
               //-----save varibles to eeprom---------------------------
               EEPROM.writeString(line1_eeprom_addr, line1.substring(0,40));                    //save input box info after to trimming
               EEPROM.writeString(line2_eeprom_addr, line2.substring(0,40));
               EEPROM.writeString(line3_eeprom_addr, line3.substring(0,40));
               EEPROM.writeString(line4_eeprom_addr, line4.substring(0,40));

//               EEPROM.writeString(line1_eeprom_addr, line1);                    //save input box info after to trimming
//               EEPROM.writeString(line2_eeprom_addr, line2);
//               EEPROM.writeString(line3_eeprom_addr, line3);
//               EEPROM.writeString(line4_eeprom_addr, line4);

               EEPROM.writeBool(checkbox1_eeprom_addr,checkbox1_is_checked); //boolean true if checked false if not checked
               EEPROM.writeBool(checkbox2_eeprom_addr,checkbox2_is_checked);
               EEPROM.writeBool(checkbox3_eeprom_addr,checkbox3_is_checked);
               EEPROM.writeBool(checkbox4_eeprom_addr,checkbox4_is_checked);
               EEPROM.commit();      ////save to eeprom
//         Serial.println(EEPROM.readBool(checkbox1_eeprom_addr));  //recall checkbox status
//         Serial.println(EEPROM.readBool(checkbox2_eeprom_addr));
//         Serial.println(EEPROM.readBool(checkbox3_eeprom_addr));
//         Serial.println(EEPROM.readBool(checkbox4_eeprom_addr));
//         Serial.println("--------saved EEPROM values --------------\n");
//         Serial.println(checkbox1_is_checked ? "True" :"False" );
//         Serial.println(checkbox2_is_checked ? "True" :"False" );
//         Serial.println(checkbox3_is_checked ? "True" :"False" );
//         Serial.println(checkbox4_is_checked ? "True" :"False" );



                checkbox1_is_checked ? checkbox1_status = "checked" : checkbox1_status = "";
                checkbox2_is_checked ? checkbox2_status = "checked" : checkbox2_status = "";
                checkbox3_is_checked ? checkbox3_status = "checked" : checkbox3_status = "";
                checkbox4_is_checked ? checkbox4_status = "checked" : checkbox4_status = "";

               Serial.println("********START HEADER*********************************************");
               Serial.println(header);
               Serial.println("********END HEADER*********************************************");
               Serial.println(line1);
               Serial.println(line2);
               Serial.println(line3);
               Serial.println(line4);
               Serial.println("Checkbox1: " + checkbox1_status);
               Serial.println("Checkbox2: " + checkbox2_status);
               Serial.println("Checkbox3: " + checkbox3_status);
               Serial.println("Checkbox4: " + checkbox4_status);

               line1.toCharArray(temp_str1,30);                       //must convert string to a character array for drawStr() to work
               line2.toCharArray(temp_str2,30);
               line3.toCharArray(temp_str3,30);
               line4.toCharArray(temp_str4,30);

               u8g2.clearBuffer();
               u8g2.drawStr(3,8,temp_str1);                                   //send line 1 to display
               u8g2.drawStr(3,18,temp_str2);
               u8g2.drawStr(3,28,temp_str3);
               u8g2.drawStr(3,48,temp_str4);
               u8g2.sendBuffer();
            } 
            else
            {
              // Do the settings page stuff
            }
            //--------------- Display the HTML web page---------------------------
            client.println("<!DOCTYPE html><html>");
            client.println("<head>");
            client.println("    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("    <link rel=\"icon\" href=\"data:,\">");
            client.println("    <style>");
            client.println("        html { font-family: Helvetica; display: inline-block; margin: 2px auto; text-align: center;}");
            client.println("        form { padding: 8px;}");
            client.println("        .button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");// default button style
            client.println("                  text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer; width: 300px;}");
            client.println("        .button2 {background-color: #111111;}</style>");  // greyed out button style
            client.println("</head>");                             

            client.println("<body>");
            client.println("<h1>Pro Tournament Scales</h1>");                                             // Web Page Heading

            if (!is_settings) {
               //-------------Form to enter information-----------------------------------------
                client.println("<form action=\"/\" method=\"GET\">");
                client.println("Line 1:");
                client.println("<input size=\"40\" type=\"text\" name=\"Line1\" value=\"" + line1 + "\">");                                     //first entry field
                client.println("<br>");
                client.println("Line 2:");
                client.println("<input size=\"40\"type=\"text\"name=\"Line2\" value=\"" + line2 + "\">");                                      //second entry field
                client.println("<br>");
                client.println("Line 3:");
                client.println("<input size=\"40\"type=\"text\"name=\"Line3\" value=\"" + line3 + "\">");                                      //second entry field
                client.println("<br>");
                client.println("Line 4:");
                client.println("<input size=\"40\"type=\"text\"name=\"Line4\" value=\"" + line4 + "\">");                                      //second entry field
                client.println("<br>");
                client.println("<br>");

                client.println("<div align = \"left\"><input type=\"checkbox\" id=\"checkbox1\" name=\"checkbox1\" value=\"checkbox1\" " + checkbox1_status + ">");
                client.println("<label for=\"checkbox1\">Print 2 Copies</label></div>");


                client.println("<div align = \"left\"><input type=\"checkbox\" id=\"checkbox2\" name=\"checkbox2\" value=\"checkbox2\" " + checkbox2_status + ">");
                client.println("<label for=\"checkbox2\">Print signature line</label></div>");

                client.println("<div align = \"left\"><input type=\"checkbox\" id=\"checkbox3\" name=\"checkbox3\" value=\"checkbox3\" " + checkbox3_status + ">");
                client.println("<label for=\"checkbox3\">Serialized ticket</label></div>");

                client.println("<div align = \"left\"><input type=\"checkbox\" id=\"checkbox4\" name=\"checkbox4\" value=\"checkbox4\" " + checkbox4_status + ">");
                client.println("<label for=\"checkbox3\">Optional Parameter (1)</label></div>");

                client.println("<br><input type=\"submit\" value=\"Submit\" class=\"button\">");

                client.println("</form>");
                client.println("<form action=\"/settings\" method=\"GET\">");
                client.println("<br><input type=\"submit\" value=\"Settings\" class=\"button\">");
                client.println("</form>");
            } else {
                       //-------------Settings Form to enter information-----------------------------------------
                client.println("<form action=\"/\" method=\"GET\">");
                client.println("<h1>Settings</h1>");

                client.println("<div align = \"left\"><input type=\"checkbox\" id=\"settingsCheck1\" name=\"settingsCheck1\" value=\"settingsCheck1\" " + settingsCheck1_status + ">");
                client.println("<label for=\"settingsCheck1\">settings checkbox 1</label></div>");

                client.println("<div align = \"left\"><input type=\"checkbox\" id=\"settingsCheck2\" name=\"settingsCheck2\" value=\"settingsCheck2\" " + settingsCheck2_status + ">");
                client.println("<label for=\"settingsCheck2\">settings checkbox 2</label></div>");

                client.println("<div align = \"left\"><input type=\"checkbox\" id=\"settingsCheck3\" name=\"settingsCheck3\" value=\"settingsCheck3\" " + settingsCheck3_status + ">");
                client.println("<label for=\"settingsCheck3\">settings checkbox 3</label></div>");

                client.println("<br><input type=\"submit\" value=\"Save Settings\" class=\"button\">");

                client.println("</form>");
                is_settings = false;                                                               //clear flag
            }

            client.println("</body>");
            client.println("</html>");
            client.println();                                                     // The HTTP response ends with another blank line
            // Break out of the while loop
            break;
          } else {                                                                // if you got a newline, then clear currentLine
            currentLine = "";
          }
        }
        else if (c != '\r') {                                                     // if you got anything else but a carriage return character,
          currentLine += c;                                                       // add it to the end of the currentLine
        }

      }//if (client.available())
    }//while (client.connected())

    header = "";                                                                   // Clear the header variable
    save_header = "";

    client.stop();                                                                 // Close the connection
    Serial.println("Client disconnected.");
    Serial.println("");
    u8g2.clearBuffer();

//    print_ticket();                                                                //print the weigh ticket when submit button is pressed
//
//    if (checkbox1_status == "checked")
//        { print_ticket();}                                                         //print second ticket if print 2 copies is selected
//
//    delay(3000);                                                                   //one second delay

  }//If Client

}//end of wifi loop



//%%%%%%%%%%%%%%%%%%%%%% functions %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%




String char_replace_http(String str) {
  str.replace("+", " ");
  str.replace("%21", "!");
  str.replace("%22", "\"");
  str.replace("%23", "#");
  str.replace("%24", "$");
  str.replace("%26", "&");
  str.replace("%27", "'");
  str.replace("%28", "(");
  str.replace("%29", ")");
  str.replace("%2A", "*");
  str.replace("%2B", "+");
  str.replace("%2C", ",");
  str.replace("%2D", "-");
  str.replace("%2E", ".");
  str.replace("%2F", "/");
  str.replace("%30", "0");
  str.replace("%31", "1");
  str.replace("%32", "2");
  str.replace("%33", "3");
  str.replace("%34", "4");
  str.replace("%35", "5");
  str.replace("%36", "6");
  str.replace("%37", "7");
  str.replace("%38", "8");
  str.replace("%39", "9");
  str.replace("%3A", ":");
  str.replace("%3B", ";");
  str.replace("%3C", "<");
  str.replace("%3D", "=");
  str.replace("%3E", ">");
  str.replace("%3F", "\?");
  str.replace("%40", "@");
  str.replace("%41", "A");
  str.replace("%42", "B");
  str.replace("%43", "C");
  str.replace("%44", "D");
  str.replace("%45", "E");
  str.replace("%46", "F");
  str.replace("%47", "G");
  str.replace("%48", "H");
  str.replace("%49", "I");
  str.replace("%4A", "J");
  str.replace("%4B", "K");
  str.replace("%4C", "L");
  str.replace("%4D", "M");
  str.replace("%4E", "N");
  str.replace("%4F", "O");
  str.replace("%50", "P");
  str.replace("%51", "Q");
  str.replace("%52", "R");
  str.replace("%53", "S");
  str.replace("%54", "T");
  str.replace("%55", "U");
  str.replace("%56", "V");
  str.replace("%57", "W");
  str.replace("%58", "X");
  str.replace("%59", "Y");
  str.replace("%5A", "Z");
  str.replace("%5B", "[");
  str.replace("%5C", "\\");
  str.replace("%5D", "]");
  str.replace("%5E", "^");
  str.replace("%5F", "_");
  str.replace("%60", "`");
  str.replace("%61", "a");
  str.replace("%62", "b");
  str.replace("%63", "c");
  str.replace("%64", "d");
  str.replace("%65", "e");
  str.replace("%66", "f");
  str.replace("%67", "g");
  str.replace("%68", "h");
  str.replace("%69", "i");
  str.replace("%6A", "j");
  str.replace("%6B", "k");
  str.replace("%6C", "l");
  str.replace("%6D", "m");
  str.replace("%6E", "n");
  str.replace("%6F", "o");
  str.replace("%70", "p");
  str.replace("%71", "q");
  str.replace("%72", "r");
  str.replace("%73", "s");
  str.replace("%74", "t");
  str.replace("%75", "u");
  str.replace("%76", "v");
  str.replace("%77", "w");
  str.replace("%78", "x");
  str.replace("%79", "y");
  str.replace("%7A", "z");
  str.replace("%7B", "{");
  str.replace("%7C", "|");
  str.replace("%7D", "}");
  str.replace("%7E", "~");
  str.replace("%E2%82%AC", "`");
  str.replace("%E2%80%9A", "‚");
  str.replace("%C6%92", "ƒ");
  str.replace("%E2%80%9E", "„");
  str.replace("%E2%80%A6", "…");
  str.replace("%E2%80%A0", "†");
  str.replace("%E2%80%A1", "‡");
  str.replace("%CB%86", "ˆ");
  str.replace("%E2%80%B0", "‰");
  str.replace("%C5%A0", "Š");
  str.replace("%E2%80%B9", "‹");
  str.replace("%C5%92", "Œ");
  str.replace("%C5%BD", "Ž");
  str.replace("%E2%80%98", "‘");
  str.replace("%E2%80%99", "’");
  str.replace("%E2%80%9C", "“");
  str.replace("%E2%80%9D", "”");
  str.replace("%E2%80%A2", "•");
  str.replace("%E2%80%93", "–");
  str.replace("%E2%80%94", "—");
  str.replace("%CB%9C", "˜");
  str.replace("%E2%84", "™");
  str.replace("%C5%A1", "š");
  str.replace("%E2%80", "›");
  str.replace("%C5%93", "œ");
  str.replace("%C5%BE", "ž");
  str.replace("%C5%B8", "Ÿ");
  str.replace("%C2%A1", "¡");
  str.replace("%C2%A2", "¢");
  str.replace("%C2%A3", "£");
  str.replace("%C2%A4", "¤");
  str.replace("%C2%A5", "¥");
  str.replace("%C2%A6", "¦");
  str.replace("%C2%A7", "§");
  str.replace("%C2%A8", "¨");
  str.replace("%C2%A9", "©");
  str.replace("%C2%AA", "ª");
  str.replace("%C2%AB", "«");
  str.replace("%C2%AC", "¬");
  str.replace("%C2%AE", "®");
  str.replace("%C2%AF", "¯");
  str.replace("%C2%B0", "°");
  str.replace("%C2%B1", "±");
  str.replace("%C2%B2", "²");
  str.replace("%C2%B3", "³");
  str.replace("%C2%B4", "´");
  str.replace("%C2%B5", "µ");
  str.replace("%C2%B6", "¶");
  str.replace("%C2%B7", "·");
  str.replace("%C2%B8", "¸");
  str.replace("%C2%B9", "¹");
  str.replace("%C2%BA", "º");
  str.replace("%C2%BB", "»");
  str.replace("%C2%BC", "¼");
  str.replace("%C2%BD", "½");
  str.replace("%C2%BE", "¾");
  str.replace("%C2%BF", "¿");
  str.replace("%C3%80", "À");
  str.replace("%C3%81", "Á");
  str.replace("%C3%82", "Â");
  str.replace("%C3%83", "Ã");
  str.replace("%C3%84", "Ä");
  str.replace("%C3%85", "Å");
  str.replace("%C3%86", "Æ");
  str.replace("%C3%87", "Ç");
  str.replace("%C3%88", "È");
  str.replace("%C3%89", "É");
  str.replace("%C3%8A", "Ê");
  str.replace("%C3%8B", "Ë");
  str.replace("%C3%8C", "Ì");
  str.replace("%C3%8D", "Í");
  str.replace("%C3%8E", "Î");
  str.replace("%C3%8F", "Ï");
  str.replace("%C3%90", "Ð");
  str.replace("%C3%91", "Ñ");
  str.replace("%C3%92", "Ò");
  str.replace("%C3%93", "Ó");
  str.replace("%C3%94", "Ô");
  str.replace("%C3%95", "Õ");
  str.replace("%C3%96", "Ö");
  str.replace("%C3%97", "×");
  str.replace("%C3%98", "Ø");
  str.replace("%C3%99", "Ù");
  str.replace("%C3%9A", "Ú");
  str.replace("%C3%9B", "Û");
  str.replace("%C3%9C", "Ü");
  str.replace("%C3%9D", "Ý");
  str.replace("%C3%9E", "Þ");
  str.replace("%C3%9F", "ß");
  str.replace("%C3%A0", "à");
  str.replace("%C3%A1", "á");
  str.replace("%C3%A2", "â");
  str.replace("%C3%A3", "ã");
  str.replace("%C3%A4", "ä");
  str.replace("%C3%A5", "å");
  str.replace("%C3%A6", "æ");
  str.replace("%C3%A7", "ç");
  str.replace("%C3%A8", "è");
  str.replace("%C3%A9", "é");
  str.replace("%C3%AA", "ê");
  str.replace("%C3%AB", "ë");
  str.replace("%C3%AC", "ì");
  str.replace("%C3%AD", "í");
  str.replace("%C3%AE", "î");
  str.replace("%C3%AF", "ï");
  str.replace("%C3%B0", "ð");
  str.replace("%C3%B1", "ñ");
  str.replace("%C3%B2", "ò");
  str.replace("%C3%B3", "ó");
  str.replace("%C3%B4", "ô");
  str.replace("%C3%B5", "õ");
  str.replace("%C3%B6", "ö");
  str.replace("%C3%B7", "÷");
  str.replace("%C3%B8", "ø");
  str.replace("%C3%B9", "ù");
  str.replace("%C3%BA", "ú");
  str.replace("%C3%BB", "û");
  str.replace("%C3%BC", "ü");
  str.replace("%C3%BD", "ý");
  str.replace("%C3%BE", "þ");
  str.replace("%C3%BF", "ÿ");


  // replace percent last so not to double swap
  str.replace("%25", "%");
  return str;
}

//-------------------------- Print Ticket ----------------------------------------------
void print_ticket(void)
              {
                int i=0;
                 String temp_string = "";
                 Serial2.write(0x1B);                //initialize printer
                 Serial2.write('@');

                 Serial2.write(0x1B);                //upside down printing
                 Serial2.write('{');
                 Serial2.write('1');

                 Serial2.write(0x1B);                //B Font 12x24
                 Serial2.write('M');
                 Serial2.write('1');

                 Serial2.write(0x1B);                //justification: center text
                 Serial2.write('a');
                 Serial2.write('1');

                 Serial2.write(0x1D);                //character size : hoizontal x2, vertical x1
                 Serial2.write(0x21);
                 Serial2.write(0x11);

                 Serial2.write(0x1B);                 //bold mode on
                 Serial2.write(0x21);
                 Serial2.write(0x38);

                 Serial2.write(0x1D);                 //multiply text size by x4 by x4
                 Serial2.write(0x21);
                 Serial2.write(0x44);

                 Serial2.write(0x1D);                 //turn smoothing on
                 Serial2.write(0x62);
                 Serial2.write('1');

                 Serial2.write(0x1D);                 //set to small text
                 Serial2.write(0x21);
                 Serial2.write(0x00);

                 if (line4 != "")
                      {Serial2.println(line4);}      //print sponsor line if anything is in it


                 if (checkbox2_status == "checked")
                      { Serial2.println("Sign________________________________________");}  //print signature line

                 Serial2.write(0x1D);                 //set text size to x4 by x4
                 Serial2.write(0x21);
                 Serial2.write(0x44);



               i=0;
               while (i++ <= 8)
                  {Serial2.write(0xC4);}              //horizontal line
                 Serial2.write(0x0A);


              //----------save data to database --------------------------------------------------
              // weight = "112.56";
               //line4.toCharArray(temp_str1,30);
              // *database [ticket][0] = temp_str1;  //save name to data base
             //  *database [ticket][1] = weight;  //save the weight

line1.toCharArray(temp_str1,30);


               if(statt == 1 )                     //h2 lb mode
                  {
                    Serial2.print(output_string);  //send weight value
                    Serial2.write(0x1D);                 //2x text size
                    Serial2.write(0x21);
                    Serial2.write(0x11);
                    Serial.printf("Lbs\n");             //print "Lbs"
                    clear_output_buffer();;                //clear the output string
                  }


               else if (statt == 2)                     //h2 lb/oz mode
                  {

                     Serial2.write(output_string[1]);          //send out string one byte at a time.
                     Serial2.write(output_string[2]);          //print lb value
                     Serial2.write(output_string[3]);
                     Serial2.write(0x1D);                 //normal text size
                     Serial2.write(0x21);
                     Serial2.write(0x00);
                     Serial2.printf("Lb");
                     Serial2.write(0x1D);                 //large text size
                     Serial2.write(0x21);
                     Serial2.write(0x44);
                     Serial2.write(output_string[5]);     //print oz value
                     Serial2.write(output_string[6]);
                     Serial2.write(output_string[7]);
                     Serial2.write(output_string[8]);

                     Serial2.write(0x1D);                 //normal text size
                     Serial2.write(0x21);
                     Serial2.write(0x00);
                     Serial2.print("oz\n");              //print the oz label with return
                     clear_output_buffer();;                //clear the output string
                  }

               else if ( statt == 3)               //357 lb mode
                  {
                     Serial2.write(output_string[0]);  //send weight
                     Serial2.write(output_string[1]);
                     Serial2.write(output_string[2]);
                     Serial2.write(output_string[3]);  //decimal point
                     Serial2.write(output_string[4]);
                     Serial2.write(output_string[5]);


                     Serial2.write(0x1D);                 //normal text size
                     Serial2.write(0x21);
                     Serial2.write(0x11);
                     Serial2.print("Lbs\n");
                     clear_output_buffer();;                //clear the output string
         // weight = "";
                  }

               else if (statt == 4)                     //357 lb/oz mode
                  {
                     Serial2.write(output_string[0]);
                     Serial2.write(output_string[1]);      //send lbs
                     Serial2.write(output_string[2]);

                     Serial2.write(0x1D);                 //normal text size
                     Serial2.write(0x21);
                     Serial2.write(0x11);
                     Serial2.printf("Lb");                //print "lb" label
                     Serial2.write(0x1D);                 //large text size
                     Serial2.write(0x21);
                     Serial2.write(0x44);
                     Serial2.write(output_string[6]);
                     Serial2.write(output_string[7]);
                     Serial2.write(output_string[8]);
                     Serial2.write(output_string[9]);
                     Serial2.write(output_string[10]);
                     Serial2.write(0x1D);                 //normal text size
                     Serial2.write(0x21);
                     Serial2.write(0x11);
                     Serial2.print("oz\n");

                     clear_output_buffer();              //clear the output string
     //  weight = "";
                  }
                else if (statt == 0)                //no signal
                  {
                  Serial2.printf("No Signal");
                  }

                 Serial2.write(0x1D);                 //multiply text size by 4 by 4
                 Serial2.write(0x21);
                 Serial2.write(0x44);


               i=0;
               while (i++ <= 8)
                  {
                  Serial2.write(0xC4);               //horizontal line
                  }
                 Serial2.write(0x0A);

                 Serial2.write(0x1D);                 //small text
                 Serial2.write(0x21);
                 Serial2.write(0x00);


                 if (checkbox3_status == "checked")                            //is serialized ticket check box checked
                     {Serial2.printf("S/N # %08d",serial_number);
                     Serial2.write(0x0A);
                     }



                 Serial2.write(0x1D);                //character size(horiz x2   vertical x2)
                 Serial2.write('!');
                 Serial2.write(0x11);



              //------bottom of box--------------------
                 Serial2.write(0xC8);               //bottom of double line square box
                i=0;
               while (i++ <= 14)
                  {
                   Serial2.write(0xCD);
                  }
                 Serial2.write(0xBC);             //right bottom corner
                 Serial2.write(0x0A);
               //-------------------------------------------------



                 Serial2.write(0xBA);            //left side line
                 Serial2.printf("     ");
                 Serial2.printf("WEIGHT");
                 Serial2.printf("    ");
                 Serial2.write(0xBA);            //right side line
                 Serial2.write(0x0A);
                 Serial2.write(0xBA);            //left side line
                 Serial2.printf("    ");
                 Serial2.printf("OFFICIAL");
                 Serial2.printf("   ");
                 Serial2.write(0xBA);              //right side double line
                 Serial2.write(0x0A);

                  Serial2.write(0xC9);               //top of double line square box
                  i=0;
                  while (i++ <= 14)
                      {  Serial2.write(0xCD);}
                 Serial2.write(0xBB);


                 Serial2.write(0x1D);
                 Serial2.write(0x00);

               //--------------area to insert tournament name and address and date--------------
               if (line1!= "")                                //if line 1 is not blank

                   {
                       Serial2.write(0x1D);                 // set text size to small size
                       Serial2.write(0x21);
                       Serial2.write(0x00);

                       Serial2.write(0x0A);                 //line feeds
                       Serial2.write(0x0A);
                       Serial2.write(0x0A);

                       Serial2.print(line3);               //print 3rd line of text
                       Serial2.write(0x0A);

                       Serial2.print(line2);                //print second line of text
                       Serial2.write(0x0A);

                       Serial2.print(line1);                //print first line of text
                       Serial2.write(0x0A);
                   }
               else
                  {
                  //--------print pts name in reverse text---------------

                    Serial2.write(0x1D);                 // set text size to small size
                    Serial2.write(0x21);
                    Serial2.write(0x00);

                    Serial2.write(0x1D);                //reverse text toggle on
                    Serial2.write('B');
                    Serial2.write('1');                 //1 = turn on reverse 0= turn off reverse

                    Serial2.printf("\n\n\n        Pro Tournament Scales         \n");

                    Serial2.write(0x1D);                //reverse text toggle off
                    Serial2.write('B');
                    Serial2.write('0');
                    Serial2.printf("stat = %d\n",stat); //diagnostic
                  }
               //-------------- cut paper-----------------------------
                 Serial2.write(0x1D);                // "GS" cut paper
                 Serial2.write('V');                 //"V"
                 Serial2.write(0x42);                //decimal 66
                 Serial2.write(0xB0);                //length to feed before cut (mm)
              // delay_ms(200);
              // while (input(Pin_B1 == 0))          //wait for switch to be released
              //     {delay_ms(5);}
              // ticket = ticket + 1;                        pointer for weigh tickets
           } //end of routine
//--------------------------------------------------------------------------------
void clear_output_buffer(void)
    {int i=0;
     while(i <= 30)
         {output_string[i] = 0;
          i=i+1;
         }
 //    next_in = 0;
     }

//-----------------  Printer routine from ccs compiler  -------------------------------------


/*

char buffer[31];                                         //incoming data buffer                                                                                        //reserve ram for buffer
char radio_buffer[31];
char temp_buffer[31];                                    //temporary register to hold values while calculating
char output_string[31];                                  //converted data to send out
char Line4[45];
char Line3[45];
char Line2[45];
char Line1[45];
char next_in;
char data_in,delay_constant;
int1 stx_flag,eox_flag,process_buffer_flag;
int1 B_status;
int16 graphic_word,timer_one,light_delay;
int32 timer_100us;
char x,out,rb;
char large_text[4];
char weight[15];
int i;
int stat;      //1 = h2 lb   2= h2 lb/oz     3 = 357 lb     4 = 357 lb/oz
//*****************************************  Declare sub-routines  *****************************************************************

void display_graphic(void);
void clear_232_buffer(void);
void convert_string(void);
void clear_output_buffer(void);
void clear_radio_buffer(void);
void write_string(int8 address,int8 string[]);
void read_string(int8 address, char data);
//================================================  Interrupts =====================================================================

#int_rda
void serial_isr()
  {
  if(process_buffer_flag ==0 )
     {buffer[next_in]=fgetc(ComA);                                  //get character from uart and save in buffer
      if(++next_in >=30)
          {next_in = 30;}
      data_in=1;
     }                                                //set flag so program will process string
      else
        {getc();}                                               //go ahead and get character to keep uart from overloading
  }

#int_TIMER1
void TIMER1_isr(void)                                        //timer 1 set for 13.1 ms interrupts
   {
   ++timer_one;
   }


#int_TIMER2
void TIMER2_isr(void)                                        //timer 2 set for 51.2us interupts
   {
   ++timer_100us;
   }



//++++++++++++++++++++++++++++++++++++++++++++++++++  Start of main program loop  ++++++++++++++++++++++++++++++++++++++++++++++++++
   while(1)                                                //Once program enters this loop it will stay here until powered off
      {

          if (timer_one >=100)                       //if no signal this timer times out
            {stat = 0;
            timer_one = 1510;
            }


           restart_wdt();
           if (input(Pin_B0)== 0)               //if switch is pressed
              {
                 Serial2.write(0x1B);                //initialize printer
                 Serial2.write('@');

                 Serial2.write(0x1B);                //upside down printing
                 Serial2.write('{');
                 Serial2.write('1');

                 Serial2.write(0x1B);                //B Font 12x24
                 Serial2.write('M');
                 Serial2.write('1');

                 Serial2.write(0x1B);                //justification: center text
                 Serial2.write('a');
                 Serial2.write('1');

                 Serial2.write(0x1D);                //character size : hoizontal x2, vertical x1
                 Serial2.write(0x21);
                 Serial2.write(0x11);

                 Serial2.write(0x1B);                 //bold mode on
                 Serial2.write(0x21);
                 Serial2.write(0x38);

                 Serial2.write(0x1D);                 //multiply text size by x4 by x4
                 Serial2.write(0x21);
                 Serial2.write(0x44);

                 Serial2.write(0x1D);                 //turn smoothing on
                 Serial2.write(0x62);
                 Serial2.write('1');

               i=0;
               while (I++ <= 8)
                  {
                    Serial2.write(0xC4);              //horizontal line
                  }
                 Serial2.write(0x0A);
              // weight = "112.56";

               if(stat == 1 )                     //h2 lb mode
                  {//sprintf(output_string, "%S",weight);
                   fprintf("%s",output_string);  //send weight value
                     Serial2.write(0x1D);                 //2x text size
                     Serial2.write(0x21);
                     Serial2.write(0x11);
                   fprintf("Lbs\n");             //print "Lbs"
                   output_string = "";                //clear the output string
                  }


               else if (stat == 2)                     //h2 lb/oz mode
                  {

                     Serial2.write(output_string[1]);          //send out string one byte at a time.
                     Serial2.write(output_string[2]);          //print lb value
                     Serial2.write(output_string[3]);
                     Serial2.write(0x1D);                 //normal text size
                     Serial2.write(0x21);
                     Serial2.write(0x00);
                    fprintf("Lb");
                     Serial2.write(0x1D);                 //large text size
                     Serial2.write(0x21);
                     Serial2.write(0x44);
                     Serial2.write(output_string[5]);     //print oz value
                     Serial2.write(output_string[6]);
                     Serial2.write(output_string[7]);
                     Serial2.write(output_string[8]);

                     Serial2.write(0x1D);                 //normal text size
                     Serial2.write(0x21);
                     Serial2.write(0x00);
                    fprintf("oz\n");              //print the oz label with return

                   output_string = "";               //clear string
                  }

               else if ( stat == 3)               //357 lb mode
                  {
                     Serial2.write(output_string[0]);  //send weight
                     Serial2.write(output_string[1]);
                     Serial2.write(output_string[2]);
                     Serial2.write(output_string[3]);  //decimal point
                     Serial2.write(output_string[4]);
                     Serial2.write(output_string[5]);


                     Serial2.write(0x1D);                 //normal text size
                     Serial2.write(0x21);
                     Serial2.write(0x11);
                   fprintf("Lbs\n");
                   output_string = "";                //clear the output string
                   weight = "";
                  }

               else if (stat == 4)                     //357 lb/oz mode
                  {
                     Serial2.write(output_string[0]);
                     Serial2.write(output_string[1]);      //send lbs
                     Serial2.write(output_string[2]);

                     Serial2.write(0x1D);                 //normal text size
                     Serial2.write(0x21);
                     Serial2.write(0x11);
                     fprintf("Lb");                //print "lb" label
                     Serial2.write(0x1D);                 //large text size
                     Serial2.write(0x21);
                     Serial2.write(0x44);
                     Serial2.write(output_string[6]);
                     Serial2.write(output_string[7]);
                     Serial2.write(output_string[8]);
                     Serial2.write(output_string[9]);
                     Serial2.write(output_string[10]);
                     Serial2.write(0x1D);                 //normal text size
                     Serial2.write(0x21);
                     Serial2.write(0x11);
                   fprintf("oz\n");

                   output_string = "";               //clear the output string
                   weight = "";
                  }
                else if (stat == 0)                //no signal
                  {
                  fprintf( ,"No Signal");
                  }

                 Serial2.write(0x1D);                 //multiply text size by 4 by 4
                 Serial2.write(0x21);
                 Serial2.write(0x44);


               i=0;
               while (I++ <= 8)
                  {
                    Serial2.write(0xC4);               //horizontal line
                  }
                 Serial2.write(0x0A);

                 Serial2.write(0x1D);                //character size(horiz x2   vertical x2)
                 Serial2.write('!');
                 Serial2.write(0x11);

              //------bottom of box--------------------
                 Serial2.write(0xC8);               //bottom of double line square box
                i=0;
               while (I++ <= 14)
                  {
                    Serial2.write(0xCD, );
                  }
                 Serial2.write(0xBC);             //right bottom corner
                 Serial2.write(0x0A);
               //-------------------------------------------------

               //  Serial2.write(0x0A,ComA);

                 Serial2.write(0xBA);            //left side line
               fprintf("     ");
               fprintf("WEIGHT");
               fprintf("    ");
                 Serial2.write(0xBA);            //right side line
                 Serial2.write(0x0A);
                 Serial2.write(0xBA);            //left side line
               fprintf("    ");
               fprintf("OFFICIAL");
               fprintf("   ");
                 Serial2.write(0xBA);              //right side double line
                 Serial2.write(0x0A);

                  Serial2.write(0xC9);               //top of double line square box
               i=0;
               while (I++ <= 14)
                  {  Serial2.write(0xCD);}
                 Serial2.write(0xBB);


                 Serial2.write(0x1D);
                 Serial2.write(0x00);

               //--------------area to insert tournament name and address and date--------------
               if (Line1!= "")

                   {   Serial2.write(0x0A);
                       Serial2.write(0x0A);
                       Serial2.write(0x0A);
                     fprintf(Line3);
                       Serial2.write(0x0A);
                     fprintf(Line2);
                       Serial2.write(0x0A);
                     fprintf(Line1);
                       Serial2.write(0x0A);


                   }
               else
                  {
                  //--------print pts name in reverse text---------------
                    Serial2.write(0x1D, );                //reverse text toggle on
                    Serial2.write('B');
                    Serial2.write('1');                 //1 = turn on reverse 0= turn off reverse

                  fprintf("\n\n\n        Pro Tournament Scales         \n");

                    Serial2.write(0x1D);                //reverse text toggle off
                    Serial2.write('B');
                    Serial2.write('0');
                  fprintf("stat = %d\n",stat); //diagnostic
                  }
               //---------------------------------------------------------------
               weight = "";                     //clear the weight value
                 Serial2.write(0x1D);                // "GS" cut paper
                 Serial2.write('V');                 //"V"
                 Serial2.write(0x42);                //decimal 66
                 Serial2.write(0xB0);                //length to feed before cut (mm)
               delay_ms(200);
               while (input(Pin_B1 == 0))          //wait for switch to be released
                   {delay_ms(5);}
           }//if (input(Pin_B0)== 0)

  //-----------------

       if (next_in >= 30)                                   //limit serial recieve buffer to this value
          {next_in = 0;                                    //reset everything if buffer overruns
           stx_flag = 0;
           eox_flag = 0;
          }



  ///-----code below is the software uart that reads the radio and looks for commands coming through the air
       if (kbhit( ) ==1)                                        //check for input from radio on software rs232
           { timer_one = 0;                                         //    reset no signal timer
           radio_buffer[rb] = fgetc( );                         //get character from software uart

            if(radio_buffer[rb]== 0x02)                           //check for start of string for scale command
              {rb=0;
               radio_buffer[rb]=0x02;
              }
             if(radio_buffer[rb]== 0x0A)                           //check for start of string for 357 scale command
              {rb=0;                                               //reset pointer
               radio_buffer[rb]=0x0A;
              }



           if(++rb >30)                                          //increment pointer, reset and clear buffer if overflow
              {rb=0;
              clear_radio_buffer();                               //clear buffer on overflow
              }                                                   //buffer overflow, reset pointer to start

           if(radio_buffer[rb-1]==0x0D && ((radio_buffer[0] == 0x02) || radio_buffer[0] == 0x0A))//end of string and start of string accepted
              {
              if (radio_buffer[7] == 0x2E)                        //lb mode if decimal is in 7th position
                 {
                 if (radio_buffer[0] == 0x02)
                         {stat=1;                                 //lb  mode in H2
                         output_string = "";                      //clear the string
                         memmove(output_string,radio_buffer+3,7);
                         clear_232_buffer();
                         clear_radio_buffer();
                         }
                   else
                         {stat=3;                                //lb mode in 357
                         output_string = "";
                         memmove(output_string,radio_buffer+4,6);
                         clear_232_buffer();
                         clear_radio_buffer();
                         }
                 }

              else
                 {

                  if (radio_buffer[14] == 0x7A)                //if z is in 14 position
                         {stat=4;                              //lb/oz mode in 357
                         output_string = "";
                         memmove(output_string,radio_buffer+3,10);
                         clear_232_buffer();
                         clear_radio_buffer();
                         }
                  else
                         { stat=2;                              //lb oz mode in H2
                         output_string = "";
                         memmove(output_string,radio_buffer+2,12);
                         clear_232_buffer();
                         clear_radio_buffer();
                         }
                 }
              sprintf(weight,"%s",output_string);                 //save value to string named weight
              rb=0;                                               //reset pointer
              output_toggle(PIN_A4);                               //flash led
              clear_radio_buffer();                               //clear buffer
              }
           }

   //--- this is the hardware uart that reads the info coming from the scale
      if(data_in==1)                                        //get character in uart if "data_in" flag is set
        {data_in = 0;                                      //reset flag used in uart interuupt routine


       if (buffer[0] !=0xFF)                    //if not a weight string pass character through to radio
                   {
                   x = next_in-1;                         //save pointer value
                   next_in = 0;                           //reset pointer to prevent overflow
                   disable_interrupts(INT_RDA);
                   Serial2.write(buffer[x]);                //send character out the software uart
                   enable_interrupts(INT_RDA);
                   }
        else
          {
            if(buffer[next_in-1] == 0xFF)                   //weight string begins with 0xFF
              {clear_232_buffer();                       //new string so clear out buffer
               buffer[0] = 0xFF;
               next_in =1;                                 //reset the buffer pointer to start
               stx_flag = 1;                               //set flag so program knows start of transmission was recieved
              }
           else
              {

              if ((buffer[next_in-1] == 0x0D) && (buffer[0] ==0xFF) )   //check for end of transmission (carriage return)
                 {
                  if (buffer[0] == 0xFF )                               //if stx is in first postion of string
                    {
                     memcpy(temp_buffer,buffer,next_in);                //save input string to temp buffer
                     process_buffer_flag = 1;                           //of stx and eox was recieved then set process buffer flag
                    }
                  else
                    {clear_232_buffer();}                              //reset buffer since
                  }
              if (next_in >= 30)
                 {clear_232_buffer();}                        //buffer overflow, clear buffer and reset pointer
              }
       if(process_buffer_flag ==1)                         //if buffer is complete then process
         {
          convert_string();                             //send converted output string to remote display and taysys
          process_buffer_flag = 0;                        //reset flags
          eox_flag = 0;
          stx_flag = 0;
         }
      }
   }//test3434

   }//end for while(1)
}//end of main()
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//=======================================================Sub Routines===============================================================

void write_string(int8 address,int8 string[])
     {
     int8 ctr = 0;                                    //set counter to zero
     while(string[ctr] !=0)
        {write_eeprom (addresss++, string[ctr])       //write string to eeprom
        ctr++;
        }
     }

void read_string(int8 address, char data)
     {
      int i = 0;
      char string[120];
      for(i=0;i<= strlen(data)-1; i++)
         {string[i] = read_eeprom(address++);}
    }

void clear_232_buffer(void)
    {int i=0;
     while (i <= 30)
         {
          buffer[i] = 0;                                    //set to zero
          i=i+1;                                            //increment pointer
         }
     next_in = 0;
    }

void clear_output_buffer(void)
    {int i=0;
     while(i <= 30)
         {output_string[i] = 0;
          i=i+1;
         }
     next_in = 0;
     }

void clear_radio_buffer(void)
     {int i=0;
     while(i <= 30)
      {radio_buffer[i] = 0x00;
      i=i+1;
      }
     next_in = 0;
     }


//-------------------------------------------




void convert_string(void)
   {                                      //take 11 byte input string and convert to 14 byte ts string

   if(buffer[0] == 0xFF)                 //if string starts with 0xFF this is a weight value to send to remotes and taysys
       {

       output_string[14] = 0x00;
       output_string[13] = 0x0D;               //"cr" carriage return
       if (buffer[3] ==0x4c)
          {
           output_string[12] = 0x48;             //(changes to H when locked)
          }
       else
          {output_string[12] = 0x4D;}             //"M" indicates movement
       output_string[11] = 0x47;                //"G" or "N" for gross mode or net mode
       output_string[10] = 0x4C;                //"L"  indicates lbs
       output_string[9] =  buffer[9];           //hundreths column
       output_string[8] =  buffer[8];           //tenths column
       output_string[7] =  buffer[7];                //"." decimal point
       output_string[6] =  buffer[6];           //value of weight
       output_string[5] =  buffer[5];
       output_string[4] =  buffer[4];
       output_string[3] =  buffer[3];
       output_string[2] =  0x20;
       output_string[1] =  0x20;
       output_string[0] =  0x02;
       fprintf(ComB,"%s",output_string);                     //send data out    x = 0;
       clear_output_buffer();
       }
    else
       {

      fprintf(ComB,"%s",temp_buffer);
     }

   }*/



////--------------timer interupt code example ----------------------
//volatile int interruptCounter;                              //varible that tracks number of interupts
//int totalInterruptCounter;                                  //counter to track total number of interrupts
//
//hw_timer_t * timer = NULL;                                 //in order to configure the timer, we will need
//                                                           //a pointer to a variable of type hw_timer_t,
//                                                           //which we will later use in the Arduino setup function.
//
//
//portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;      //we will need to declare a variable of type portMUX_TYPE,
//                                                           //which we will use to take care of the synchronization
//                                                           //between the main loop and the ISR, when modifying a shared variable.

//------------------- Timer INterupt  ---------------------------------------------------------------------------------------
//void IRAM_ATTR onTimer()                                  //this is the actual interuupt(place before void setup() code)
//  {
//  portENTER_CRITICAL_ISR(&timerMux);
//  interruptCounter++;                                      //put code to perform during interrupt here
//  portEXIT_CRITICAL_ISR(&timerMux);
//  }
//
//void setup() {
//
//  Serial.begin(115200);
//
//  timer = timerBegin(0, 80, true);                     //"0" is the timer to use, '80' is the prescaler,true counts up 80mhz divided by 80 = 1 mhz or 1 usec
//  timerAttachInterrupt(timer,&onTimer,               //&onTimer is the function to call when intrrupt occurs,"true" is edge interupted
//  timerAlarmWrite(timer, 1000000, true);                //interupt every 1000000 times
//  timerAlarmEnable(timer);                              //this line enables the timer and starts it
//
//}


//------------------EEPROM examples-------------------------------------------------------------------------------
//  EEPROM.writeByte(address, -128);                  // -2^7
//  Serial.println(EEPROM.readByte(address));
//
//  EEPROM.writeChar(address, 'A');                   // Same as writyByte and readByte
//  Serial.println(char(EEPROM.readChar(address)));
//
//  EEPROM.writeUChar(address, 255);                  // 2^8 - 1
//  Serial.println(EEPROM.readUChar(address));
//
//  EEPROM.writeShort(address, -32768);               // -2^15
//  Serial.println(EEPROM.readShort(address));
//
//  EEPROM.writeUShort(address, 65535);               // 2^16 - 1
//  Serial.println(EEPROM.readUShort(address));
//
//  EEPROM.writeInt(address, -2147483648);            // -2^31
//  Serial.println(EEPROM.readInt(address));
//
//  EEPROM.writeUInt(address, 4294967295);            // 2^32 - 1
//  Serial.println(EEPROM.readUInt(address));
//
//  EEPROM.writeLong(address, -2147483648);           // Same as writeInt and readInt
//  Serial.println(EEPROM.readLong(address));
//
//  EEPROM.writeULong(address, 4294967295);           // Same as writeUInt and readUInt
//  Serial.println(EEPROM.readULong(address));
//
//  int64_t value = -9223372036854775808;             // -2^63
//  EEPROM.writeLong64(address, value);
//  value = 0;                                        // Clear value
//  value = EEPROM.readLong64(value);
//  Serial.printf("0x%08X", (uint32_t)(value >> 32)); // Print High 4 bytes in HEX
//  Serial.printf("%08X\n", (uint32_t)value);         // Print Low 4 bytes in HEX
//
//  uint64_t  Value = 18446744073709551615;           // 2^64 - 1
//  EEPROM.writeULong64(address, Value);
//  Value = 0;                                        // Clear Value
//  Value = EEPROM.readULong64(Value);
//  Serial.printf("0x%08X", (uint32_t)(Value >> 32)); // Print High 4 bytes in HEX
//  Serial.printf("%08X\n", (uint32_t)Value);         // Print Low 4 bytes in HEX
//
//  EEPROM.writeFloat(address, 1234.1234);
//  Serial.println(EEPROM.readFloat(address), 4);
//
//  EEPROM.writeDouble(address, 123456789.123456789);
//  Serial.println(EEPROM.readDouble(address), 8);

//  EEPROM.writeBool(address, true);
//  Serial.println(EEPROM.readBool(address));
//
//  String sentence = "I love ESP32.";
//  EEPROM.writeString(address, sentence);
//  Serial.println(EEPROM.readString(address));
//
//  char gratitude[] = "Thank You Espressif!";
//  EEPROM.writeString(address, gratitude);
//  Serial.println(EEPROM.readString(address));

//------------------------- reflash program from sd card -------------------------------------------------
/*
 Name:      SD_Update.ino
 Created:   12.09.2017 15:07:17
 Author:    Frederik Merz <frederik.merz@novalight.de>
 Purpose:   Update firmware from SD card
 Steps:
   1. Flash this image to the ESP32 an run it
   2. Copy update.bin to a SD-Card, you can basically
      compile this or any other example
      then copy and rename the app binary to the sd card root
   3. Connect SD-Card as shown in SD_MMC example,
      this can also be adapted for SPI
   3. After successfull update and reboot, ESP32 shall start the new app
*/

//#include <Update.h>
//#include <FS.h>
//#include <SD_MMC.h>

// perform the actual update from a given stream
void performUpdate(Stream &updateSource, size_t updateSize) {
   if (Update.begin(updateSize)) {
      size_t written = Update.writeStream(updateSource);
      if (written == updateSize) {
         Serial.println("Written : " + String(written) + " successfully");
      }
      else {
         Serial.println("Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
      }
      if (Update.end()) {
         Serial.println("OTA done!");
         if (Update.isFinished()) {
            Serial.println("Update successfully completed. Rebooting.");
         }
         else {
            Serial.println("Update not finished? Something went wrong!");
         }
      }
      else {
         Serial.println("Error Occurred. Error #: " + String(Update.getError()));
      }

   }
   else
   {
      Serial.println("Not enough space to begin OTA");
   }
}

// check given FS for valid update.bin and perform update if available
void updateFromFS(fs::FS &fs) {
   File updateBin = fs.open("/update.bin");
   if (updateBin) {
      if(updateBin.isDirectory()){
         Serial.println("Error, update.bin is not a file");
         updateBin.close();
         return;
      }

      size_t updateSize = updateBin.size();

      if (updateSize > 0) {
         Serial.println("Try to start update");
         performUpdate(updateBin, updateSize);
      }
      else {
         Serial.println("Error, file is empty");
      }

      updateBin.close();

      // whe finished remove the binary from sd card to indicate end of the process
      fs.remove("/update.bin");
   }
   else {
      Serial.println("Could not load update.bin from sd root");
   }
}

//void setup() {
//   uint8_t cardType;
//   Serial.begin(115200);
//   Serial.println("Welcome to the SD-Update example!");
//
//   // You can uncomment this and build again
//   // Serial.println("Update successfull");
//
//   //first init and check SD card
//   if (!SD_MMC.begin()) {
//      rebootEspWithReason("Card Mount Failed");
//   }
//
//   cardType = SD_MMC.cardType();
//
//   if (cardType == CARD_NONE) {
//      rebootEspWithReason("No SD_MMC card attached");
//   }else{
//      updateFromFS(SD_MMC);
//  }
//}

void rebootEspWithReason(String reason){
    Serial.println(reason);
    delay(1000);
    ESP.restart();
}

//will not be reached
//void loop() {

//}
