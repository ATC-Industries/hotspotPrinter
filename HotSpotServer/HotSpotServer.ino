

/**************************************************************
  Terry Clarkson & Adam Clarkson
  11/02/18
***************************************************************
           [NOTE- TO-Do list located at bottom of this file]

This program uses a serial port to read an XBee radio board connected to UART 1
and send the weight value recieved from a scale to a printer connected to UART 2
UART 1 is used for a debug monitor.

The board is setup as a WIFI hotspot so user can log onto the system and set and
change parameters related to the printing of the weigh ticket.

Pass word override - power up the system while holding down the print button,default
pass word is [987654321]

                                                                                                                  __________________________
pin assignment                                      5 volt--------------------------------------------------------|                         |
                                  |-------------|     GND  -------------------------------------------------------|                         |
                            ______|             |_______                                                          |                         |
            3.3 volts out---|3.3V                   gnd |                                                         |                         |
                            |EN                    IO23 | ---- SPI MOSI to SD card--------------------------------|                         |
                            |SVP                   IO22 | ---- SCL pin to 4x20 LCD display --------------|----|   |       SD CARD           |
                            |SVN                   TXD0 | ---- Serial TX Monitor and programming uart0   |  L |   |                         |
      ___________           |IO34                  RXD0 | ---- Serial RX Monitor and programming uart0   |  C |   |                         |
     /           \          |IO35                  IO21 | ---- SDA pin to 4x20 LCD display --------------|  D |   |                         |
    |             |---------|IO32                   GND |                                                ------   |                         |
    |             |---------|IO33                  IO19 | ---- SPI MISO to SD card--------------------------------|                         |
    |   XBEE      |         |IO25                  IO18 | ---- clock on SD card-----------------------------------|                         |
    |             |    F2---|IO26                  IO5  | ---- CS on SD card--------------------------------------|                         |
    |             |    F4---|IO27                  IO17 | ---- TX Uart2 Printer                                   |                         |
    |             |         |IO14                  IO16 | ---- RX Uart2 Printer                                   |_________________________|
    |             |         |IO12                  IO4  | ---- F3
     -------------          |GND                   IO0  |
                        F1--|IO13                  IO2  | ---- PRT button
                            |SD2                   IO15 |
                            |SD3                   SD1  |
                            |CMD                   SD0  |
           5 volts in   ----|5V                    CLK  |
                            _____________________________



*/
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


//------------------INclude files ------------------------------------------
#include <WiFi.h>                                 // Load Wi-Fi library
#include <Arduino.h>
//#include <U8g2lib.h>                              //driver for oled display
#include <string.h>                               //enables the string fuctions
#include <EEPROM.h>                               //driver for eeprom

//------ files for sd card ------------------------
#include <Update.h>
#include <FS.h>
#include <SD.h>                                  //routines for SD card reader/writer
#include <SPI.h>                                 //SPI functions
#include <Wire.h>
#include <LiquidCrystal_I2C.h>                 //4x20 lcd display

#include "css.h"      // refrence to css file to bring in CSS styles
#include "SDfunc.h"   // refrence the SD card functions
#include "html.h"     // refrence to HTML generation functions

#define EEPROM_SIZE 1024                            //rom reserved for eeprom storage

#define RXD2 16                                     //port 2 serial pins for external printer
#define TXD2 17
//----------- assign port pins to buttons -----------------------------
#define button_F1 13  ///works
#define button_F2 26  //works

#define button_F3 4  //works
#define button_F4 27  //works
#define button_PRINT 2 //works
//U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);     //identify pins used for oled display

//----------------- an integer array to hold the version number ----------------------------------
const int VERSION_NUMBER[3] = {0,0,4};   // [MAJOR, MINOR, PATCH]

//----------------- Replace with network credentials ----------------------------------
const char* ssid     = "ProTournament";
const char* password = "123456789";
WiFiServer server(80);                                 // Set web server port number to 80
//--------------------------------------------------------------------------------------------



//-----------------Define varibles------------------------------------------------------------
String header;                                        // Variable to store the HTTP request
String save_header;
String line1 = "";
String line2 = "";
String line3= "";
String line4 = "";
char temp_str1[31];
char temp_str2[31];
char temp_str3[31];
char temp_str4[31];
char radio_rx_array[31];                          //array being recieved on xbee radio
bool radio_rx_ready = false;                         // whether the rec radio string is complete
int radio_rx_pointer;                               //pointer for radio rx buffer
int statt;      //1 = h2 lb   2= h2 lb/oz     3 = 357 lb     4 = 357 lb/oz
int serial_number;
char output_string[31];                                  //converted data to send out
char temp_str[31];
String temp_val = "";
char weight[15];
int touch_value_0;
int touch_value_1 = 100;
int touch_value_2 = 100;
int touch_value_3 = 100;
int touch_value_4 = 100;
int touch_value_5 = 100;
int read_keyboard_timer;
bool cb_print_2_copies;
bool cb_print_signature_line;
bool cb_serial_ticket;
bool cb_print_when_locked;
bool checkbox5_is_checked;
bool lock_flag = false;                                     //flag that indicates weight is a locked value
bool cb_print_on_lock;                                      //check box flag for print on lock
volatile int ticket;                                        //ticket serial number
byte Imac[6];                                               //array to hold the mac address
String checkbox1_status = "";
String checkbox2_status = "";
String checkbox3_status = "";
String checkbox4_status = "";
String checkbox5_status = "";
bool isSDCardPresent = false;
volatile int interruptCounter;                             //varible that tracks number of interupts
int totalInterruptCounter;                                 //counter to track total number of interrupts
int no_signal_timer;                                       //timeout counter used to display No Signal on display
char *database[100][2];                                    //database array to hold anglers name and weight
hw_timer_t * timer = NULL;                                 //in order to configure the timer, we will need
                                                           //a pointer to a variable of type hw_timer_t,
                                                           //which we will later use in the Arduino setup function.


portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;      //we will need to declare a variable of type portMUX_TYPE,
                                                           //which we will use to take care of the synchronization
                                                           //between the main loop and the ISR, when modifying a shared variable.

// ------------ setting variables ---------------------------------------
String settingsCheck1_status = "";
String settingsCheck3_status = "";
String settingsCheck2_status = "";
void settingsPageForm();
void mainPageForm();
bool is_page_settings = false;
bool is_page_print = false;
bool is_page_update = false;

//------------ Assign eeprom save addresses -----------------------------
const int line1_eeprom_addr = 0;
const int line2_eeprom_addr = 50;
const int line3_eeprom_addr = 100;
const int line4_eeprom_addr = 150;

const int checkbox1_eeprom_addr= 200;
const int checkbox2_eeprom_addr= 201;
const int checkbox3_eeprom_addr= 202;
const int checkbox4_eeprom_addr= 203;
const int serial_number_addr = 204;



//----------funtion prototypes -----------------------------------------
String char_replace_http(String str);                 // routine to remove unwanted characters from header file
void clear_output_buffer(void);
void clear_radio_rx_array(void);                      //routine to clear rx buffer from xbee radio
void print_ticket(void);                              //function to print the weigh ticket
void set_text_size(unsigned int size);                //oled set text size routine
void set_text_reverse(bool on_off);                   //oled set reverse text on/off
void processRadioString();                            //routine to process radio rx string
//void insertCSS(WiFiClient& client);                                     //insert the CSS style from css.ino

/****** SD card update funtion prototypes ***********/
void performUpdate(Stream &updateSource, size_t updateSize);  // perform the actual update from a given stream
void updateFromFS(fs::FS &fs, String updateFileName);  //  check given FS for valid update.bin and perform update if available
void updateFirmware();                                 //  Call this when you want the program to look for an update file on the SD Card
void rebootEspWithReason(String reason);               //  reboot and serial log the reason for failure

//-------------Interuput routines ----------------------------------------
void IRAM_ATTR onTimer()                            // (100 ms) this is the actual interrupt(place before void setup() code)
  {
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounter++;                                      //put code to perform during interrupt here
  read_keyboard_timer++;
  portEXIT_CRITICAL_ISR(&timerMux);
  }

/**
 * Checks the status of the checkbox in the header and changes the flag and status string
 * @param h          header string to look at
 * @param is_checked flag that indicates if checkbox is checked
 * @param status     string that indicates if checkbox is checked
 * @param number     String value that indicates the checkbox number
 */
void checkboxStatus(String h, bool& is_checked, String& status, String number) {
  if (h.indexOf("&checkbox" + number) >= 0) {                   //if 'checkbox1' shows in string the checkbox is checked
   is_checked = true;
   status = "checked";
  } else {
   is_checked = false;
   status = "";                                    //set to null value if not checked
  }
}

LiquidCrystal_I2C lcd(0x3F,20,4);                      // set the LCD address to 0x27 or 3f for a 20 chars and 4 line display


//--------------------------------------------------------------------------
//-------------Start of Program -----------------------------------------
//------------------------------------------------------------------------
void setup()
    {

     lcd.init();
     lcd.backlight();
     lcd.clear();                                          //clear the display
     lcd.setCursor(0,0);                                   //set cursor position
     lcd.print(F("Agri-Tronix Corp"));                     //print text to display


     //---- declare input buttons with pullup --------------------------
     pinMode(button_PRINT,INPUT_PULLUP);                       //print button
     pinMode(button_F1,INPUT_PULLUP);                          //F1
     pinMode(button_F2,INPUT_PULLUP);                          //F2
     pinMode(button_F3,INPUT_PULLUP);                          //F3
     pinMode(button_F4,INPUT_PULLUP);                          //F4

    //----------- setup 1us counter ---------
    timer = timerBegin(0, 80, true);                     //"0" is the timer to use, '80' is the prescaler,true counts up 80mhz divided by 80 = 1 mhz or 1 usec
    timerAttachInterrupt(timer,&onTimer,true);            //"&onTimer" is the int function to call when intrrupt occurs,"true" is edge interupted
    timerAlarmWrite(timer, 100000, true);                //interupt every 1000000 times
    timerAlarmEnable(timer);                              //this line enables the timer declared 3 lines up and starts it
    ticket = 0;

   //-------------  declare serial ports -----------------------------
   // Note the format for setting a serial port is as follows: Serial2.begin(baud-rate, protocol, RX pin, TX pin);

   Serial1.begin(9600, SERIAL_8N1,33,32);                   //RADIO, tx =32 rx = 33
   Serial2.begin(9600, SERIAL_8N1,16,17);                   //THERMAL PRINTER, TX = pin 17 RX = pin 16
   Serial.begin(115200);                                    //start serial port 0 (debug monitor and programming port)

  //--- initialize the EEPROM ---------------------------------------
   if (!EEPROM.begin(EEPROM_SIZE))                          //set aside memory for eeprom size
       {
       Serial.println("failed to intialize EEPROM");        //display error to monitor
       }


  Serial.print("Setting AP (Access Point)…\n");                         // Connect to Wi-Fi network with SSID and password
  /* Remove the password parameter, if you want the AP (Access Point) to be open
  if pin 2 is pulled low,(print button pressed) a temporary password will be displayed on the remote
  dispay and the password will be printed out on the printer*/
  if (!digitalRead(13))                                                  // if print button is held down during power up
       {
        Serial.println("password = 987654321");

        lcd.setCursor(0,0);
        lcd.print("Temporary Password");
        lcd.setCursor(0,1);
        lcd.print("987654321");
        while(!digitalRead(button_PRINT))                                              //loop until button is released
             {delay(30);}
        WiFi.softAP(ssid,"987654321");

        //------------ print a ticket with the temp password ----------------------
        Serial2.println("________________________________________");
        Serial2.println("Temporary password to use ");
        Serial2.println(" ");
          set_text_size(0x00);
        Serial2.println("987654321");
        set_text_size(0x00);               //normal text size
        Serial2.println("Reset your password with an 8-digit password\n\rmade up of letters and numbers");
        //-------------- cut paper-----------------------------
        Serial2.write(0x1D);                // "GS" cut paper
        Serial2.write('V');                 //"V"
        Serial2.write(0x42);                //decimal 66
        Serial2.write(0xB0);                //length to feed before cut (mm)
       }

  else                                                                     //if print button is not pressed
      {
      WiFi.softAP(ssid,password);                                        //ssid declared in setup, pw recalled from eeprom or use default
      Serial.println("password = 123456789");
      }

        //.softAP(const char* ssid, const char* password, int channel, int ssid_hidden, int max_connection)

  IPAddress IP = WiFi.softAPIP();                                       //get the ip address
  Serial.print("AP IP address: ");                                      //print ip address to SM
  Serial.println(IP);
  server.begin();                                                       //start server
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("SSID= ProTournament");

  char ip_string[30];                                                   //declare a character array
  sprintf(ip_string,"IP = %d.%d.%d.%d",WiFi.softAPIP()[0],WiFi.softAPIP()[1],WiFi.softAPIP()[2],WiFi.softAPIP()[3]);   //this creates the ip address format to print (192.169.4.1)
    lcd.setCursor(0,1);
    lcd.print(ip_string);
  delay(5000);                                                          //leave ssid and ip on oled sceen for this delay
               line1 = (EEPROM.readString(line1_eeprom_addr));          //recall values saved in eeprom
               line2 = (EEPROM.readString(line2_eeprom_addr));
               line3 = (EEPROM.readString(line3_eeprom_addr));
               line4 = (EEPROM.readString(line4_eeprom_addr));
               serial_number = EEPROM.readUInt(serial_number_addr);     //get ticket serial number

               cb_print_2_copies = (EEPROM.readBool(checkbox1_eeprom_addr));  //recall checkbox status (boolean)
               cb_print_signature_line = (EEPROM.readBool(checkbox2_eeprom_addr));
               cb_serial_ticket = (EEPROM.readBool(checkbox3_eeprom_addr));
               cb_print_when_locked = (EEPROM.readBool(checkbox4_eeprom_addr));

               cb_print_2_copies ? checkbox1_status = "checked" : checkbox1_status = "";    //set 'checkbox#_is_checked' to match 'checkbox#_status'
               cb_print_signature_line ? checkbox2_status = "checked" : checkbox2_status = "";
               cb_serial_ticket ? checkbox3_status = "checked" : checkbox3_status = "";
               cb_print_when_locked ? checkbox4_status = "checked" : checkbox4_status = "";

               line1.toCharArray(temp_str1,30);                               //must convert string to a character array for drawStr() to work
               line2.toCharArray(temp_str2,30);
               line3.toCharArray(temp_str3,30);
               line4.toCharArray(temp_str4,30);

                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print(temp_str1);
                lcd.setCursor(0,1);
                lcd.print(temp_str2);
                lcd.setCursor(0,2);
                lcd.print(temp_str3);
                lcd.setCursor(0,3);
                lcd.print(temp_str4);

      WiFi.macAddress(Imac);
        Serial.print("MAC");
        for(int i=5;i>=0;i--)
      {
      Serial.print(":");
        Serial.print(Imac[i],HEX);                                   //print the mac address to serial monitor
      }

// Check if SD card is present
isSDCardPresent = isSDCard();
}//void setup() ending terminator



//&&&&&&&&&&&&&&&&&&&&&&&&&   Start of Program Loop  &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
void loop(){

   if (interruptCounter > 0)                          //every one second 100 msec int is generated
      {
      portENTER_CRITICAL(&timerMux);
      interruptCounter--;                             //reset counter to zero
      portEXIT_CRITICAL(&timerMux);
      totalInterruptCounter++;                        //increment counter for ints generated every second
      }


   if (totalInterruptCounter >=50)
            { totalInterruptCounter = 0;                //reset counter
              if (++ no_signal_timer >= 2)               //if no signal this timer times out
                 { statt = 0;                            //set display mode to 0 so "No Signal" will be displayed
                   lcd.clear();
                   lcd.setCursor(5,1);
                   lcd.print("No Signal");
                 }
            }
   //--------------- read print button routine -------------------------------------------------------

if (read_keyboard_timer >= 2)                          //read keypad every 200 ms
     {read_keyboard_timer = 0;                         //reset timer

     if (!digitalRead(button_PRINT))                //if pushbutton is pressed (low condition), print the ticket
      { print_ticket();                              //print the weight ticket
        delay(300);
        if (checkbox1_status == "checked")           //if checkbox "print 2 tickets" is checked
            {print_ticket();}                        //print second ticket if print 2 copies is selected
        while (!digitalRead(button_PRINT))           //loop while button is held down
            {delay(80);}

       if (checkbox3_status == "checked")            //if check box 'print serial number' is checked
          {serial_number++;                             //increment serial number
            EEPROM.writeUInt(serial_number_addr,serial_number);} //save serial number to eeprom
      }
     lcd.setCursor(0,3);
     if (!digitalRead(button_F1))                   //F1 button
       {lcd.print("F1");}
     else
        {lcd.print("   ");}

     lcd.setCursor(5,3);
     if (!digitalRead(button_F2))                   //F2 button
       {lcd.print("F2");}
     else
       {lcd.print("   ");}


       lcd.setCursor(10,3);
     if (!digitalRead(button_F3))                   //F3 button
       {lcd.print("F3");}
     else
        {lcd.print("   ");}

     lcd.setCursor(15,3);
     if (!digitalRead(button_F4))                   //F4 button
       {lcd.print("F4");}
     else
       {lcd.print("   ");}


      }


//--------------- radio uart recieve ---------------------------------------------------------------
      if (Serial1.available() > 0)                  //if data in recieve buffer, send to serial monitor
          {char c;
           c = (char)Serial1.read();                //get byte from uart buffer
           radio_rx_array[radio_rx_pointer] += c;   //add character to radio rx buffer
           radio_rx_pointer ++;                     //increment pointer
           if (radio_rx_pointer >=30)               //buffer overflow
              {clear_radio_rx_array();              //clear rx radio buffer
               radio_rx_pointer = 0;                //reset the rx radio buffer
              }
            if (c == 0x0D || c == 0x0A)             //if character is CR or LF then process buffer
              {
             //-------------display weight on LCD-----------------------------------
               lcd.clear();
               lcd.setCursor(4,1);
               lcd.print(radio_rx_array);

             //------------------------------------------------------------------------
             processRadioString();
               }
          }



//-------------- start client routine ----------------------------------------------------------------

    WiFiClient client = server.available();   // Listen for incoming clients
    String updateMessage = "";
    if (client)
    {                                 // If a new client connects (tablet or cell phone logs on)
        Serial.println("New Client.");          // print a message out in the serial port monitor
        String currentLine = "";                // make a String to hold incoming data from the client
        while (client.connected())
        {            // loop while the client's connected
            if (client.available())
            {         // if there's bytes to read from the client,
                char c = client.read();     // read a byte, then
                Serial.write(c);            // print it out the serial monitor
                header += c;                //add character to the header string
                if (c == '\n')
                {                // if the byte is a newline character
                    // if the current line is blank, you got two newline characters in a row.
                    // that's the end of the client HTTP request, so send a response:
                    if (currentLine.length() == 0)
                    {
                        // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                        // and a content-type so the client knows what's coming, then a blank line:
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println("Connection: close");
                        client.println();

                        //-----read the returned information from form when submit button is pressed and save to memory--------
                        String headerT = header.substring(0,header.indexOf("Host:")); //create substring from begining to the word 'Host:'

                        Serial.println("headerT:");            //print substring to serial monitor
                        Serial.println(headerT);
                        if(!(header.indexOf("favicon") >= 0))            //id header does not contin "favicon"
                        {
                            if (headerT.indexOf("settings?") >= 0)      //if header contains "settings"
                            {
                                is_page_settings = true;
                                is_page_print = false;
                                is_page_update = false;
                            } else if (headerT.indexOf("print?") >= 0)  //if header contains "print?"
                                {
                                print_ticket();                         //print weigh ticket
                                Serial.println("PRINT BUTTON WAS PRESSED ON WEB PAGE");
                                is_page_settings = false;
                                is_page_print = true;
                                is_page_update = false;
                                }
                             else if (headerT.indexOf("update?") >= 0)
                                {
                                is_page_settings = false;
                                is_page_print = false;
                                is_page_update = true;
                                }
                            else if (headerT.indexOf("checkForUpdate?") >= 0)
                                {
                                checkForUpdateFirmware(updateMessage);
                                }
                            else if (headerT.indexOf("doUpdate") >= 0)
                            {
                                String strIndex =  header.substring(header.indexOf("doUpdate")+8,header.indexOf("?"));//parse out the varible strings for the the 4 lines
                                int index = strIndex.toInt();
                                updateFirmware(updateMessage, arrayOfUpdateFiles[index]);
                            }
                            else
                                {
                                is_page_settings = false;
                                is_page_print = false;
                                is_page_update = false;
                                }
                        }
                        // Looks for Line1 in header and then processes the SETTINGS results if found
                        if ((headerT.indexOf("Line1=") >= 0)&& !(header.indexOf("favicon") >= 0)) //if text 'Line1=' is found and text 'favicon' is not found
                        {
                            Serial.println("********found it (screen one) *********************************************");
                            line1 =  header.substring(header.indexOf("Line1=")+6,header.indexOf("&Line2="));//parse out the varible strings for the the 4 lines
                            line2 =  header.substring(header.indexOf("Line2=")+6,header.indexOf("&Line3="));
                            line3 =  header.substring(header.indexOf("Line3=")+6,header.indexOf("&Line4="));
                            // Check if line 4 is end of data or if checkbox info follows
                            if (headerT.indexOf("&check") >= 0)
                            {line4 =  headerT.substring(headerT.indexOf("Line4=")+6,headerT.indexOf("&check"));}
                            else
                            {line4 =  headerT.substring(headerT.indexOf("Line4=")+6,headerT.indexOf(" HTTP"));}

                            // Check if checkbox is checked
                            checkboxStatus(headerT, cb_print_2_copies, checkbox1_status, "1");
                            checkboxStatus(headerT, cb_print_signature_line, checkbox2_status, "2");
                            checkboxStatus(headerT, cb_serial_ticket, checkbox3_status, "3");
                            checkboxStatus(headerT, cb_print_when_locked, checkbox4_status, "4");

                            line1 = char_replace_http(line1); //remove and replace http characters with space
                            line2 = char_replace_http(line2);
                            line3 = char_replace_http(line3);
                            line4 = char_replace_http(line4);
                            //-----save varibles to eeprom---------------------------
                            EEPROM.writeString(line1_eeprom_addr, line1.substring(0,40)); //save input box info after to trimming
                            EEPROM.writeString(line2_eeprom_addr, line2.substring(0,40));
                            EEPROM.writeString(line3_eeprom_addr, line3.substring(0,40));
                            EEPROM.writeString(line4_eeprom_addr, line4.substring(0,40));

                            EEPROM.writeBool(checkbox1_eeprom_addr,cb_print_2_copies); //boolean true if checked false if not checked
                            EEPROM.writeBool(checkbox2_eeprom_addr,cb_print_signature_line);
                            EEPROM.writeBool(checkbox3_eeprom_addr,cb_serial_ticket);
                            EEPROM.writeBool(checkbox4_eeprom_addr,cb_print_when_locked);
                            EEPROM.commit();                         ////save to eeprom

                            cb_print_2_copies ? checkbox1_status = "checked" : checkbox1_status = "";
                            cb_print_signature_line ? checkbox2_status = "checked" : checkbox2_status = "";
                            cb_serial_ticket ? checkbox3_status = "checked" : checkbox3_status = "";
                            cb_print_when_locked ? checkbox4_status = "checked" : checkbox4_status = "";
                            //-------------- display varibles on serial monitor  -----------------------------------
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

                        }
                        else    //if header did not contain text "line1" then run code in else statment below
                        {
                                // do some stuff
                        }
                        //--------------- Display the HTML web page---------------------------
                        // HTML head
                        client.println(R"(
                            <!DOCTYPE html>
                            <html>
                            <head>
                                <meta name="viewport" content="width=device-width, initial-scale=1">
                                <link rel="icon" href="data:,">
                                <style>.middle-form{max-width: 500px; margin:auto;padding:10px;}
                            )");
                            insertCSS(client);
                        client.println("</head>");
                        client.println("<body>");

                        // svg PTS logo
                        printPTSLogo(client);

                        if (is_page_settings) { //------------ First Screen HTML code ------------------------------------------
                                //-------------Form to enter information-----------------------------------------
                            pageTitle(client, "Settings");
                            //startForm(client, "[action]")
                            startForm(client, "/");
                            //inputBox(client, "[string name of variable]", [actual variable], "[label]", [smalltext? BOOL], "[small text string]")
                            inputBox(client, "Line1", line1, "Top Line", true, "Example: The Tournament Name");
                            inputBox(client, "Line2", line2, "Second Line", true, "Example: The Tournament Location");
                            inputBox(client, "Line3", line3, "Third Line", true, "Example: The Tournament Dates");
                            inputBox(client, "Line4", line4, "Bottom Line", true, "Example: A sponsor message");

                            checkBox(client, "checkbox1", checkbox1_status, "Print 2 Copies");
                            checkBox(client, "checkbox2", checkbox2_status, "Print signature line");
                            checkBox(client, "checkbox3", checkbox3_status, "Serialized ticket");
                            checkBox(client, "checkbox4", checkbox4_status, "Optional Parameter");

                            button(client, "submit", "primary");
                            endForm(client);
                            endDiv(client);

                            // if the startup flag that determined if an SD card is present then display the update button
                            if(isSDCardPresent)
                            {
                                client.println(R"(
                                    <div class="middle-form">
                                        <form action="/update" method="GET">
                                            <input type="submit" value="Update Firmware" class="btn btn-success btn-lg btn-block">
                                        </form>
                                    </div>
                                )");
                            }
                        }
                        //   Update page HTML
                        else if (is_page_update) {
                            pageTitle(client, "Update Firmware");
                            //  Add breif instructions
                            client.println("<div class=\"alert alert-primary\" role=\"alert\">");
                            client.println("This will update the firmare on your device.<br>Insert an SD card with a version of the firmware loaded and click \"Check for Update\".");
                            client.println("<hr>");
                            client.println("<p class=\"mb-0\">NOTE: you may need to reconnect to this wifi network after updating.</p>");
                            client.println("</div>");
                            // Update now button
                            if(updateMessage == ""){
                                client.println("<div class=\"middle-form\">");
                                client.println("<form action=\"/checkForUpdate\" method=\"GET\">");
                                client.println("<input type=\"submit\" value=\"Check for Update\" class=\"btn btn-success btn-lg btn-block\">");
                                client.println("</form>");
                                client.println("</div>");
                            }
                            // Print message to user dynamically
                            if(updateMessage != ""){
                                client.println("<div class=\"alert alert-danger\" role=\"alert\">");
                                client.println("<h4 class=\"alert-heading\">Error!</h4>");
                                client.println("<p>" + updateMessage + "</p>");
                                client.println("<hr>");
                                client.println("<p class=\"mb-0\">Please make sure you have loaded the update software in the root directory of the SD card.</p>");
                                client.println("</div>");
                                client.println("<div class=\"middle-form\">");
                                client.println("<form action=\"/checkForUpdate\" method=\"GET\">");
                                client.println("<input type=\"submit\" value=\"Retry\" class=\"btn btn-success btn-lg btn-block\">");
                                client.println("</form>");
                                client.println("</div>");
                            }
                            if (arrayOfUpdateFiles[0] != ""){
                                printTableOfUpdateFiles(client, arrayOfUpdateFiles);
                            }
                            //------------ Cancel BUTTON ------------------------------
                            client.println("<div class=\"middle-form\">");
                            client.println("<form action=\"/\" method=\"GET\">");
                            client.println("<input type=\"submit\" value=\"Cancel\" class=\"btn btn-danger btn-lg btn-block\">");
                            client.println("</form>");
                            client.println("</div></div>");
                        }
                        //---------------------  Home screen
                        else
                        {
                        pageTitle(client, "HotSpot Printer");

                            client.println("<div class=\"middle-form\">");
                            client.println("<form action=\"/print\" method=\"GET\">");
                            // Print Button
                            client.println(R"(
                                <button type="submit" value="Print" id="print" style="height:250px;" class="btn btn-danger btn-lg btn-block">
<div style="margin:auto 35%;color:white;fill:white;">
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512"><path d="M432 192h-16v-82.75c0-8.49-3.37-16.62-9.37-22.63L329.37 9.37c-6-6-14.14-9.37-22.63-9.37H126.48C109.64 0 96 14.33 96 32v160H80c-44.18 0-80 35.82-80 80v96c0 8.84 7.16 16 16 16h80v112c0 8.84 7.16 16 16 16h288c8.84 0 16-7.16 16-16V384h80c8.84 0 16-7.16 16-16v-96c0-44.18-35.82-80-80-80zM320 45.25L370.75 96H320V45.25zM128.12 32H288v64c0 17.67 14.33 32 32 32h64v64H128.02l.1-160zM384 480H128v-96h256v96zm96-128H32v-80c0-26.47 21.53-48 48-48h352c26.47 0 48 21.53 48 48v80zm-80-88c-13.25 0-24 10.74-24 24 0 13.25 10.75 24 24 24s24-10.75 24-24c0-13.26-10.75-24-24-24z"/></svg>
                        </div>
                        <br>Print Ticket
                                </button>)");
                            client.println("</form>");
                            client.println("</div>");

                            // Settings Button
                            client.println("<div class=\"middle-form\" style=\"margin-top:25px;\">");
                            client.println("<form action=\"/settings\" method=\"GET\">");
                            client.println("<input type=\"submit\" value=\"Settings\" class=\"btn btn-warning btn-lg btn-block\">");
                            client.println("</form>");
                            client.println("</div>");
                        }
                        // Version number in bottom right of all pages
                        client.println(R"(
                            </div>
                        <nav class="navbar bottom navbar-light bg-light">
                            <div class="middle-form">
                                <a class="navbar-brand text-right" href="#"><p class="text-right text-muted">version: )" + String(VERSION_NUMBER[0]) + R"(.)" + String(VERSION_NUMBER[1]) + R"(.)" + String(VERSION_NUMBER[2]) + R"(</p></a>
                            </div>
                        </nav>
                        )");
                        client.println("</body>");
                        client.println("</html>");
                        client.println();                         // The HTTP response ends with another blank line
                        // Break out of the while loop
                        break;
                } else {                                          // if you got a newline, then clear currentLine
                        currentLine = "";
                }
            }
            else if (c != '\r') {                                     // if you got anything else but a carriage return character,
                currentLine += c;                                 // add it to the end of the currentLine
            }

        }// END if (client.available())
    }// END while (client.connected())

        header = "";                                                               // Clear the header variable
        save_header = "";

        client.stop();                                                             // Close the connection
        Serial.println("Client disconnected.");                                    //send status message to serial debug window
        Serial.println("");

    }  //end of 'If (Client)'

}//end of program 'loop()'

//%%%%%%%%%%%%%%%%%%%%%% functions %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

//-------------------------- Print Ticket ----------------------------------------------
void print_ticket(void)
              {
               int i=0;
               String temp_string = "";            //create a temp string
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

               Serial2.write(0x1B);                 //bold mode on
               Serial2.write(0x21);
               Serial2.write(0x38);


               Serial2.write(0x1D);                 //turn smoothing on
               Serial2.write(0x62);
               Serial2.write('1');
               set_text_size(0x00);               //1x text size
               if (line4 != "")
                      {Serial2.println(line4);}      //print sponsor line if anything is in it


               if (checkbox2_status == "checked")
                      { Serial2.println("Sign________________________________________");}  //print signature line

               set_text_size(0x44);               //5x text size
               i=0;
               while (i++ <= 8)
                  {Serial2.write(0xC4);}              //horizontal line
               Serial2.write(0x0A);


              //----------save data to database --------------------------------------------------
              // weight = "112.56";
               //line4.toCharArray(temp_str1,30);
              // *database [ticket][0] = temp_str1;     //save name to data base
             //  *database [ticket][1] = weight;        //save the weight

//                  for (i = 0; i< 30;i++)
//                      {Serial.print(output_string[i]);}   //*** diagnostic
//                   Serial.println("line 654- statt =" + String(statt));       //***diag print the statt value

               if(statt == 1 )                          //h2 lb mode
                  {
                    Serial2.print(output_string);       //send weight value
                    set_text_size(0x11);                //2x text size
                    Serial2.println("Lbs");             //print "Lbs"
                    clear_output_buffer();;             //clear the output string
                  }


               else if (statt == 2)                     //H2 lb/oz mode
                    {
                     Serial2.write(output_string[1]);          //send out string one byte at a time.
                     Serial2.write(output_string[2]);          //print lb value
                     Serial2.write(output_string[3]);
                     set_text_size(0x00);
                     Serial2.printf("Lb");
                     set_text_size(0x44);                     //5x text size
                     Serial2.write(output_string[5]);         //print oz value
                     Serial2.write(output_string[6]);
                     Serial2.write(output_string[7]);
                     Serial2.write(output_string[8]);
                     set_text_size(0x00);                     //normal text size
                     Serial2.print("oz\n");                   //print the oz label with return
                     clear_output_buffer();;                  //clear the output string
                  }

               else if ( statt == 3)                          //357 lb mode
                  {
                     Serial2.write(output_string[0]);         //send weight
                     Serial2.write(output_string[1]);
                     Serial2.write(output_string[2]);
                     Serial2.write(output_string[3]);         //decimal point
                     Serial2.write(output_string[4]);
                     Serial2.write(output_string[5]);
                     set_text_size(0x11);
                     Serial2.print("Lbs\n");
                     clear_output_buffer();;                 //clear the output string
         // weight = "";
                  }

               else if (statt == 4)                         //357 lb/oz mode
                  {
                     Serial2.write(output_string[0]);
                     Serial2.write(output_string[1]);      //send lbs
                     Serial2.write(output_string[2]);

                     set_text_size(0x11);                   //2x text size
                     Serial2.printf("Lb");                  //print "lb" label
                     set_text_size(0x44);                   //5x text size
                     Serial2.write(output_string[6]);
                     Serial2.write(output_string[7]);
                     Serial2.write(output_string[8]);
                     Serial2.write(output_string[9]);
                     Serial2.write(output_string[10]);
                     set_text_size(0x11);
                     Serial2.print("oz\n");
                    clear_output_buffer();                  //clear the output string
                   }
                else if (statt == 0)                        //no signal
                  {
                  Serial2.printf("No Signal");
                  }

               set_text_size(0x44);                        //set text to 5x
               Serial2.write(0x0A);                        //line feed
               i=0;
               while (i++ <= 8)
                  {Serial2.write(0xC4);}                   //horizontal line character
               Serial2.write(0x0A);                        //line feed


               set_text_size(0x00);                        //set text to 1x

               if (checkbox3_status == "checked")          //is serialized ticket check box checked
                   {Serial2.printf("S/N # %08d",serial_number);  //print ticket sequence number
                   Serial2.write(0x0A);
                   }


               set_text_size(0x11);                       //character size(horiz x2   vertical x2)


              //------bottom of box-------------------------------
               Serial2.write(0xC8);                       //bottom of double line square box
                i=0;
               while (i++ <= 14)
                  {Serial2.write(0xCD);}
               Serial2.write(0xBC);                     //right bottom corner character
               Serial2.write(0x0A);
               //---------------Box with 'Official Weight' printed in it ----------------------
               Serial2.write(0xBA);                     //left side line
               Serial2.printf("     ");
               Serial2.printf("WEIGHT");
               Serial2.printf("    ");
               Serial2.write(0xBA);                     //verical line character
               Serial2.write(0x0A);
               Serial2.write(0xBA);                     //left side line
               Serial2.printf("    ");
               Serial2.printf("OFFICIAL");
               Serial2.printf("   ");
               Serial2.write(0xBA);                     //right side double line
               Serial2.write(0x0A);
               Serial2.write(0xC9);                     //top of double line square box
               i=0;
               while (i++ <= 14)
                    {  Serial2.write(0xCD);}
               Serial2.write(0xBB);
               Serial2.write(0x1D);
               Serial2.write(0x00);

               //--------------area to insert tournament name and address and date--------------
               if (line1!= "")                         //if line 1 is not blank
                     {
                     set_text_size(0x00);            //normal text size
                     Serial2.write(0x0A);            //line feeds
                     Serial2.write(0x0A);
                     Serial2.write(0x0A);

                     Serial2.print(line3);           //print 3rd line of text
                     Serial2.write(0x0A);

                     Serial2.print(line2);           //print second line of text
                     Serial2.write(0x0A);

                     Serial2.print(line1);           //print first line of text
                     Serial2.write(0x0A);
                    }
               else              //--------print pts name in reverse text---------------
                   {

                    set_text_size(0x00);
                    set_text_reverse(true);
                    Serial2.printf("\n\n\n        Pro Tournament Scales         \n");
                    set_text_reverse(false);
                    Serial2.printf("stat = %d\n",stat); //diagnostic
                   }
               //-------------- cut paper-----------------------------
                 Serial2.write(0x1D);                // "GS" cut paper
                 Serial2.write('V');                 //"V"
                 Serial2.write(0x42);                //decimal 66
                 Serial2.write(0xB0);                //length to feed before cut (mm)
              // delay_ms(200);
              // while (input(Pin_B1 == 0))          //wait for switch to be released if pressed
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
    }


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//=======================================================Sub Routines===============================================================
void set_text_size(unsigned int size)               //set font size on printer
      {
      Serial2.write(0x1D);                 // set text size to small size
      Serial2.write(0x21);
      Serial2.write(size);                 // sizes - 00= 1,11 = 2x,22 = 3x,33 = 4x ,44= 5x
      }

void set_text_reverse(bool on_off)                  //set or clear reverse text
      {
      Serial2.write(0x1D);
      Serial2.write('B');
      if (on_off)
          Serial2.write('1');                 //1= on 0= off
      else
          Serial2.write('0');
      }

void clear_radio_rx_array(void)                          //routine to clear radio rx buffer
     {int i=0;
     while(i <= 30)
      {radio_rx_array[i] = 0x00;                            //set all 30 locations to 0x00
      i=i+1;
      }
     radio_rx_pointer= 0;                             //reset  pointer
    }


//----------------------Process radio string if flag is set--------------------------------
void processRadioString()
{ int i = 25;
  no_signal_timer = 0;                                //reset the no signal timer since a reading was sensed
  lock_flag = false;                                  //preset lock flag to false
  while(i >= 3)                                       //search from  the 7 to the 15th character in array
      { if (radio_rx_array[i] == 'H')                 //check for locked value in string
           {Serial.println("**Locked**");
            lock_flag = true;                         //an 'H' was found so set lock flag
            break;                                    //exit the while loop
           }
        i--;
      }
 if(radio_rx_array[radio_rx_pointer-1]==0x0D && ((radio_rx_array[0] == 0x02) || radio_rx_array[0] == 0x0A))//end of string and start of string accepted
  {
  if (radio_rx_array[7] == 0x2E)                      //lb mode if decimal is in 7th position
     {
     if (radio_rx_array[0] == 0x02)                   //look for stx command
         {statt=1;                                    //lb  mode in H2
         memmove(output_string,radio_rx_array+3,7);   //parse out the output string
         clear_radio_rx_array();                       //clear the array
         }
     else
         {statt=3;                                    //lb mode in 357
         memmove(output_string,radio_rx_array+4,6);   //copy weight numbers to 'output_string'
         clear_radio_rx_array();                      //clear the radio rx buffer
         }
     }

  else
     {
      if (radio_rx_array[14] == 0x7A)                 //if z is in 14 position
         {statt=4;                                    //lb/oz mode in 357
         memmove(output_string,radio_rx_array+3,10);  //copy the weight numbers to 'output_string'
         clear_radio_rx_array();                      //clear the radio rx buffer
         }
      else
         { statt=2;                                   //lb oz mode in H2
         memmove(output_string,radio_rx_array+2,12);  //create output string from radio rx
         clear_radio_rx_array();                      //clear the radio rx buffer
         }
     }
  sprintf(weight,"%s",output_string);                 //save value to string named weight
  radio_rx_pointer=0;                                 //reset pointer
  clear_radio_rx_array();                             //clear the radio rx buffer
  }
  else                                                //send error to SM (serial monitor)
  {
   Serial.println("unable to process radio rx string");
   clear_radio_rx_array();
  // Serial.println("if(radio_rx_array[radio_buff_pointer-1]==0x0D && ((radio_rx_array[0] == 0x02) || radio_rx_array[0] == 0x0A))");

  }
}//void processRadioString()

//void write_string(int  address,int  string[])
//     {
//     int ctr = 0;                                    //set counter to zero
//     while(string[ctr] !=0)
//        {write_eeprom (address++, string[ctr])       //write string to eeprom
//        ctr++;
//        }
//     }
//
//void read_string(int  address, char data)
//     {
//      int i = 0;
//      char string[120];
//      for(i=0;i<= strlen(data)-1; i++)
//         {string[i] = read_eeprom(address++);}
//    }
//
//void clear_232_buffer(void)
//    {int i=0;
//     while (i <= 30)
//         {
//          buffer[i] = 0;                                    //set to zero
//          i=i+1;                                            //increment pointer
//         }
//     next_in = 0;
//    }


//-------------------------------------------




//void convert_string(void)
//   {                                      //take 11 byte input string and convert to 14 byte ts string
//
//   if(buffer[0] == 0xFF)                 //if string starts with 0xFF this is a weight value to send to remotes and taysys
//       {
//
//       output_string[14] = 0x00;
//       output_string[13] = 0x0D;               //"cr" carriage return
//       if (buffer[3] ==0x4c)
//          {
//           output_string[12] = 0x48;             //(changes to H when locked)
//          }
//       else
//          {output_string[12] = 0x4D;}             //"M" indicates movement
//       output_string[11] = 0x47;                //"G" or "N" for gross mode or net mode
//       output_string[10] = 0x4C;                //"L"  indicates lbs
//       output_string[9] =  buffer[9];           //hundreths column
//       output_string[8] =  buffer[8];           //tenths column
//       output_string[7] =  buffer[7];                //"." decimal point
//       output_string[6] =  buffer[6];           //value of weight
//       output_string[5] =  buffer[5];
//       output_string[4] =  buffer[4];
//       output_string[3] =  buffer[3];
//       output_string[2] =  0x20;
//       output_string[1] =  0x20;
//       output_string[0] =  0x02;
//       fprintf(ComB,"%s",output_string);                     //send data out    x = 0;
//       clear_output_buffer();
//       }
//    else
//       {
//
//      fprintf(ComB,"%s",temp_buffer);
//     }
//
//   }*/



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


/*---------------------------Code to be added ------------------------------------------------

1. Add Setting input box to enter user password
   a.save and recall password form eeprom
2. Save all weights to array to print out at end of Tournament
3. Save all names printed on Line 4 with weight to be printed out at end of tournament
4.

//-----------------CCS code from pic hotspot printer (delete when done with) tlc-------------------------
 //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++                    //
//                         H2 radio board
// Read signal form H2 indicator, convert to TS string and send out through uart                           //
//reads rs-232 string and displays the number in the TS style string                                       //
//                                                                                                         //
//                              ___________________                                                        //
//                             [                   ]
//                   5v--2.2k->|1 MCLR       B7  28|<---
//                            >|2 A0         B6  27|<---
//                         --->|3 A1         B5  26|<---
//                         --->|4 A2         B4  25|<---
//                            >|5 A3         B3  24|<---
//                            >|6 A4         B2  23|<---
//                            >|7 A5         B1  22|<---
//                   GND-------|8 Vss     INT/B0 21|<--- rx from radio (interupt driven)
//                        -----|9 OSC1       Vdd 20|-----+5 VOLTS
//                         ----|10 OSC2      Vss 19|------GND
//         xmit to scale  ----<|11 C0        C7  18|>-----rs-232 rx (from scale)
//         rx (from radio) ---<|12 C1        C6  17|>----- rs-232 tx to radio
//                        ----<|13 C2        C5  16|>-----
//                        ----<|14 C3        C4  15|>-----
//                             |___________________|
//
//




//#include <18F25k20.h>    //production choice as of 10/1/09
#include <18F23k22.h>
#include <stdlib.h>


#rom 0xF00000 = {0x0003}                                 //eeprom for 18f series

#use delay(clock=32000000,restart_wdt)                      //20mhz crystal on target board, restart watchdog
//#use fixed_io(c_outputs=PIN_C0,PIN_C1,PIN_C2,PIN_C3,PIN_C4,PIN_C5,PIN_C6)
//#use fixed_io(b_outputs=PIN_B1,PIN_B2,PIN_B3,PIN_B4,PIN_B5,PIN_B6)
#use RS232(BAUD=9600,RCV=PIN_C7,XMIT=PIN_C6,PARITY=N,BITS=8,ERRORS,stream=ComA)            //set baud rate,parity and bits 9600,8n,recieve on c7
#use RS232(Baud=9600,XMIT=PIN_C0,RCV=PIN_C1,PARITY=N,BITS=8,ERRORS,stream=ComB)             //software uart

#fuses WDT,BROWNOUT,PUT,NOMCLR,NOLVP,CPD,WRT,NODEBUG,PROTECT,INTRC_IO
#fuses wdt2048

char buffer[31];                                         //incoming data buffer                                                                                        //reserve ram for buffer
char radio_rx_array[31];
char temp_buffer[31];                                    //temporary register to hold values while calculating
char output_string[31];                                  //converted data to send out
char Line3[45];
char Line2[45];
char Line1[45];
char next_in;
char data_in,delay_constant;
int1 stx_flag,eox_flag,process_buffer_flag;
int1 B_status;
int16 graphic_word,timer_one,light_delay;
int32 timer_100us;
char x,out,radio_buff_pointer;
char large_text[4];
char weight[15];
int i;
int stat;      //1 = h2 lb   2= h2 lb/oz     3 = 357 lb     4 = 357 lb/oz
//*****************************************  Declare sub-routines  *****************************************************************

void display_graphic(void);
void clear_232_buffer(void);
void convert_string(void);
void clear_output_buffer(void);
void clear_radio_rx_array(void);
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

//**************************************** Start of Main Program *******************************************************************
void main()
 { setup_oscillator(OSC_8MHZ|OSC_PLL_ON);                 //set for 32 mhz operation

   setup_adc_ports(NO_ANALOGS);
   setup_adc(ADC_OFF);
   setup_spi(FALSE);
   setup_timer_0(RTCC_INTERNAL);
   setup_timer_1(T1_INTERNAL|T1_DIV_BY_1);                  //.2 us resolution 13.1 ms overflow // timer used to control light intesity of display
   setup_timer_2(T2_DIV_BY_16,15,1);                        //51.2 us clock
   enable_interrupts(INT_RDA);
 //enable_interrupts(INT_TIMER2);
   enable_interrupts(INT_TIMER1);
   enable_interrupts(GLOBAL);
   port_b_pullups(TRUE);

   stx_flag=0;                                      //initialize flags
   eox_flag =0;
   next_in=0;
   radio_buff_pointer = 0;
   process_buffer_flag = 0;
   enable_interrupts(INT_RDA);


   Line3 = "10/17/18";
   Line2 = "Wilbur Lake";

    Line1 = "Pro Tournament Scales Elite Series 1st Day";
//++++++++++++++++++++++++++++++++++++++++++++++++++  Start of main program loop  ++++++++++++++++++++++++++++++++++++++++++++++++++
   while(1)                                                //Once program enters this loop it will stay here until powered off
      {

          if (timer_one >=100)                       //if no signal this timer times out
            {stat = 0;
            timer_one = 1510;
            }


           restart_wdt();
           if (input(Pin_B0)== 0)               //if switch is pressed
              {//output_toggle(PIN_A4);
               fputc(0x1B,ComB);                //initialize printer
               fputc('@',ComB);

               fputc(0x1B,ComB);                //upside down printing
               fputc('{',ComB);
               fputc('1',ComB);

               fputc(0x1B,ComB);                //B Font 12x24
               fputc('M',ComB);
               fputc('1',ComB);

               fputc(0x1B,ComB);                //justification: center text
               fputc('a',ComB);
               fputc('1',ComB);

               fputc(0x1D,ComB);                //character size : hoizontal x2, vertical x1
               fputc(0x21,ComB);
               fputc(0x11,ComB);

               fputc(0x1B,ComB);                 //bold mode on
               fputc(0x21,ComB);
               fputc(0x38,ComB);

               fputc(0x1D,ComB);                 //multiply text size by x4 by x4
               fputc(0x21,ComB);
               fputc(0x44,ComB);

               fputc(0x1D,ComB);                 //turn smoothing on
               fputc(0x62,ComB);
               fputc('1',ComB);

               i=0;
               while (I++ <= 8)
                  {
                  fputc(0xC4,ComB);              //horizontal line
                  }
               fputc(0x0A,ComB);
              // weight = "112.56";

               if(stat == 1 )                     //h2 lb mode
                  {//sprintf(output_string, "%S",weight);
                   fprintf(ComB,"%s",output_string);  //send weight value
                   fputc(0x1D,ComB);                 //2x text size
                   fputc(0x21,ComB);
                   fputc(0x11,ComB);
                   fprintf(ComB,"Lbs\n");             //print "Lbs"
                   output_string = "";                //clear the output string
                  }


               else if (stat == 2)                     //h2 lb/oz mode
                  {
                   //sprintf(output_string, "%S",weight); //copy weight to output string
                   fputc(output_string[1],ComB);          //send out string one byte at a time.
                   fputc(output_string[2],ComB);          //print lb value
                   fputc(output_string[3],ComB);
                   //fputc(output_string[4],ComB);
                   //fputc(output_string[5],ComB);
                   fputc(0x1D,ComB);                 //normal text size
                   fputc(0x21,ComB);
                   fputc(0x00,ComB);
                   fprintf(ComB,"Lb");
                   fputc(0x1D,ComB);                 //large text size
                   fputc(0x21,ComB);
                   fputc(0x44,ComB);
                   fputc(output_string[5],ComB);     //print oz value
                   fputc(output_string[6],ComB);
                   fputc(output_string[7],ComB);
                   fputc(output_string[8],ComB);

                   fputc(0x1D,ComB);                 //normal text size
                   fputc(0x21,ComB);
                   fputc(0x00,ComB);
                   fprintf(ComB,"oz\n");              //print the oz label with return

                   output_string = "";               //clear string
                  }

               else if ( stat == 3)               //357 lb mode
                  {
                   fputc(output_string[0],ComB);  //send weight
                   fputc(output_string[1],ComB);
                   fputc(output_string[2],ComB);
                   fputc(output_string[3],ComB);  //decimal point
                   fputc(output_string[4],ComB);
                   fputc(output_string[5],ComB);


                   fputc(0x1D,ComB);                 //normal text size
                   fputc(0x21,ComB);
                   fputc(0x11,ComB);
                   fprintf(ComB,"Lbs\n");
                   output_string = "";                //clear the output string
                   weight = "";
                  }

               else if (stat == 4)                     //357 lb/oz mode
                  {
                   fputc(output_string[0],ComB);
                   fputc(output_string[1],ComB);      //send lbs
                   fputc(output_string[2],ComB);

                   fputc(0x1D,ComB);                 //normal text size
                   fputc(0x21,ComB);
                   fputc(0x11,ComB);
                   fprintf(ComB,"Lb");                //print "lb" label
                   fputc(0x1D,ComB);                 //large text size
                   fputc(0x21,ComB);
                   fputc(0x44,ComB);
                   fputc(output_string[6],ComB);
                   fputc(output_string[7],ComB);
                   fputc(output_string[8],ComB);
                   fputc(output_string[9],ComB);
                   fputc(output_string[10],ComB);
                   fputc(0x1D,ComB);                 //normal text size
                   fputc(0x21,ComB);
                   fputc(0x11,ComB);
                   fprintf(ComB,"oz\n");

                   output_string = "";               //clear the output string
                   weight = "";
                  }
                else if (stat == 0)                //no signal
                  {
                  fprintf(ComB,"No Signal");
                  }

               fputc(0x1D,ComB);                 //multiply text size by 4 by 4
               fputc(0x21,ComB);
               fputc(0x44,ComB);


               i=0;
               while (I++ <= 8)
                  {
                  fputc(0xC4,ComB);               //horizontal line
                  }
               fputc(0x0A,ComB);

               fputc(0x1D,ComB);                //character size(horiz x2   vertical x2)
               fputc('!',ComB);
               fputc(0x11,ComB);

              //------bottom of box--------------------
               fputc(0xC8,ComB);               //bottom of double line square box
                i=0;
               while (I++ <= 14)
                  {
                  fputc(0xCD,ComB);
                  }
               fputc(0xBC,ComB);             //right bottom corner
               fputc(0x0A,ComB);
               //-------------------------------------------------

               //fputc(0x0A,ComA);

               fputc(0xBA,ComB);            //left side line
               fprintf(ComB,"     ");
               fprintf(ComB,"WEIGHT");
               fprintf(ComB,"    ");
               fputc(0xBA,ComB);            //right side line
               fputc(0x0A,ComB);
               fputc(0xBA,ComB);            //left side line
               fprintf(ComB,"    ");
               fprintf(ComB,"OFFICIAL");
               fprintf(ComB,"   ");
               fputc(0xBA,ComB);              //right side double line
               fputc(0x0A,ComB);

                fputc(0xC9,ComB);               //top of double line square box
               i=0;
               while (I++ <= 14)
                  {fputc(0xCD,ComB);}
               fputc(0xBB,ComB);


               fputc(0x1D,ComB);                 //set text size (normal)
               fputc(0x21,ComB);
               fputc(0x00,ComB);

               //--------------area to insert tournament name and address and date--------------
               if (Line1!= "")

                   { fputc(0x0A,ComB);
                     fputc(0x0A,ComB);
                     fputc(0x0A,ComB);
                     fprintf(ComB, Line3);
                     fputc(0x0A,ComB);
                     fprintf(ComB, Line2);
                     fputc(0x0A,ComB);
                     fprintf(ComB, Line1);
                     fputc(0x0A,ComB);


                   }
               else
                  {
                  //--------print pts name in reverse text---------------
                  fputc(0x1D,ComB);                //reverse text toggle on
                  fputc('B',ComB);
                  fputc('1',Comb);                 //1 = turn on reverse 0= turn off reverse

                  fprintf(ComB,"\n\n\n        Pro Tournament Scales         \n");

                  fputc(0x1D,ComB);                //reverse text toggle off
                  fputc('B',ComB);
                  fputc('0',ComB);
                  fprintf(ComB,"stat = %d\n",stat); //diagnostic
                  }
               //---------------------------------------------------------------
               weight = "";                     //clear the weight value
               fputc(0x1D,ComB);                // "GS" cut paper
               fputc('V',ComB);                 //"V"
               fputc(0x42,ComB);                //decimal 66
               fputc(0xB0,ComB);                //length to feed before cut (mm)
               delay_ms(200);
               while (input(Pin_B1 == 0))          //wait for switch to be released
                   {delay_ms(5);}
           }//if (input(Pin_B0)== 0)



       if (next_in >= 30)                                   //limit serial recieve buffer to this value
          {next_in = 0;                                    //reset everything if buffer overruns
           stx_flag = 0;
           eox_flag = 0;
          }



  ///-----code below is the software uart that reads the radio and looks for commands coming through the air
       if (kbhit(ComB) ==1)                                        //check for input from radio on software rs232
           { timer_one = 0;                                         //    reset no signal timer
           radio_rx_array[radio_buff_pointer] = fgetc(ComB);                         //get character from software uart

            if(radio_rx_array[radio_buff_pointer]== 0x02)                           //check for start of string for scale command
              {radio_buff_pointer=0;
               radio_rx_array[radio_buff_pointer]=0x02;
              }
             if(radio_rx_array[radio_buff_pointer]== 0x0A)                           //check for start of string for 357 scale command
              {radio_buff_pointer=0;                                               //reset pointer
               radio_rx_array[radio_buff_pointer]=0x0A;
              }



           if(++radio_buff_pointer >30)                                          //increment pointer, reset and clear buffer if overflow
              {radio_buff_pointer=0;
              clear_radio_rx_array();                               //clear buffer on overflow
              }                                                   //buffer overflow, reset pointer to start

           if(radio_rx_array[radio_buff_pointer-1]==0x0D && ((radio_rx_array[0] == 0x02) || radio_rx_array[0] == 0x0A))//end of string and start of string accepted
              {
              if (radio_rx_array[7] == 0x2E)                        //lb mode if decimal is in 7th position
                 {
                 if (radio_rx_array[0] == 0x02)
                         {stat=1;                                 //lb  mode in H2
                         output_string = "";                      //clear the string
                         memmove(output_string,radio_rx_array+3,7);
                         clear_232_buffer();
                         clear_radio_rx_array();
                         }
                   else
                         {stat=3;                                //lb mode in 357
                         output_string = "";
                         memmove(output_string,radio_rx_array+4,6);
                         clear_232_buffer();
                         clear_radio_rx_array();
                         }
                 }

              else
                 {

                  if (radio_rx_array[14] == 0x7A)                //if z is in 14 position
                         {stat=4;                              //lb/oz mode in 357
                         output_string = "";
                         memmove(output_string,radio_rx_array+3,10);
                         clear_232_buffer();
                         clear_radio_rx_array();
                         }
                  else
                         { stat=2;                              //lb oz mode in H2
                         output_string = "";
                         memmove(output_string,radio_rx_array+2,12);
                         clear_232_buffer();
                         clear_radio_rx_array();
                         }
                 }
              sprintf(weight,"%s",output_string);                 //save value to string named weight
              radio_buff_pointer=0;                                               //reset pointer
              output_toggle(PIN_A4);                               //flash led
              clear_radio_rx_array();                               //clear buffer
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
                   fputc(buffer[x],ComB);                //send character out the software uart
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

void clear_radio_rx_array(void)
     {int i=0;
     while(i <= 30)
      {radio_rx_array[i] = 0x00;
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

   }



*/

// sample html code for tabs
//      <ul class="nav nav-pills dark-tabs">
//      <li role="presentation" class="active dark-tabs-li">lt;a href="#" class="dark-tabs-a">Create</a><</li>
//      <li role="presentation" class="dark-tabs-li"><a href="#" class="dark-tabs-a">Design</a>></li>
//      <li role="presentation" class="dark-tabs-li"><a href="#" class="dark-tabs-a">settings</a></li>
//      <li role="presentation" class="dark-tabs-li"><a href="#" class="dark-tabs-a">share</a></li>
//      <li role="presentation" class="dark-tabs-li"><a href="#" class="dark-tabs-a">results</a></li>
//    </ul>

//add code to read radio serial number
