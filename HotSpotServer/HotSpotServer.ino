
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
F1 + F4 = system reboot
HOld F1 down on cold boot to enter diagnostic mode.

                                                                                                                  __________________________
pin assignment                                      5 volt--------------------------------------------------------|                         |
                5v   Gnd          |-------------|     GND  -------------------------------------------------------|                         |
              ___|____|___   _____|             |_______                                                          |                         |
              |           | |3.3V                   GND |                                                         |                         |
              |   RTC     | |EN                    IO23 | ---- SPI MOSI to SD card--------------------------------|                         |
              |___________| |SVP                   IO22 | ---- SCL pin to 4x20 LCD display --------------|----|   |       SD CARD           |
                    |  |    |SVN                   TXD0 | ---- Serial TX Monitor and programming uart0   |  L |   |                         |
      ___________   22 21   |IO34                  RXD0 | ---- Serial RX Monitor and programming uart0   |  C |   |                         |
     /           \          |IO35                  IO21 | ---- SDA pin to 4x20 LCD display --------------|  D |   |                         |
    |             |---------|IO32                   GND |                                                ------   |                         |
    |             |---------|IO33                  IO19 | ---- SPI MISO to SD card--------------------------------|                         |
    |   XBEE      |         |IO25                  IO18 | ---- clock on SD card-----------------------------------|                         |
    |             |    F2---|IO26                  IO5  | ---- CS on SD card--------------------------------------|                         |
    |             |    F4---|IO27                  IO17 | ---- TX Uart2 Printer                                   |                         |
    |             |         |IO14                  IO16 | ---- RX Uart2 Printer                                   |_________________________|
    |             |         |IO12                  IO4  | ---- F3
     -------------          |GND                   IO0  | ---- on board reset button
                        F1--|IO13                  IO2  | ---- PRT button
                    X-------|SD2                   IO15 |
                    X-------|SD3                   SD1  | -----X
                    X-------|CMD                   SD0  | -----X
           5 volts in   ----|5V                    CLK  | -----X
                            _____________________________



*/

//------ Include files ---------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>            //database engine
#include <WiFi.h>               // Load Wi-Fi library
#include <EEPROM.h>             //driver for eeprom
//------ files for sd card -----------------------------------------------------
#include <Update.h>             //firmware uploader
#include <FS.h>
#include <SD.h>                 //routines for SD card reader/writer
//------ Other include files ---------------------------------------------------
#include <SPI.h>                //SPI functions
#include <Wire.h>               //i2C function
#include <LiquidCrystal_I2C.h>  //4x20 lcd display
#include <NetBIOS.h>
#include <RTClib.h>             //library for  RTC routines
#include "css.h"                // refrence to css file to bring in CSS styles
#include "SDfunc.h"             // refrence the SD card functions
#include "html.h"               // refrence to HTML generation functions
#include "sqlite.h"             // sqlite3 database functions

#define EEPROM_SIZE 1024        //rom reserved for eeprom storage

//----------- assign processor port pins to buttons --------------------------------------
#define button_F1 13            //
#define button_F2 26            //
#define button_F3 4             //
#define button_F4 27            //
#define button_PRINT 2          //

//////////////////////////////////////////////////////
//DEFINE VARIABLES
//////////////////////////////////////////////////////


//------------ Assign eeprom save addresses ------------------------------------
const int line1_eeprom_addr = 0;        // line 1      -     0 to  49 - 50 bytes
const int line2_eeprom_addr = 50;       // line 2      -    50 to  99 - 50 bytes
const int line3_eeprom_addr = 100;      // line 3      -   100 to 149 - 50 bytes
const int line4_eeprom_addr = 150;      // line 4      -   150 to 199 - 50 bytes

const int checkbox1_eeprom_addr = 200;  // checkbox1   -   200
const int checkbox2_eeprom_addr = 201;  // checkbox2   -   201
const int checkbox3_eeprom_addr = 202;  // checkbox3   -   202
const int checkbox4_eeprom_addr = 203;  // checkbox4   -   203
const int serial_number_addr = 204;     // checkbox5   -   204
const int password_addr = 215;          // password    -   215


//------------- an integer array to hold the version number---------------------
const int VERSION_NUMBER[3] = {0,0,4};   // [MAJOR, MINOR, PATCH]

//----------------- Replace with network credentials ---------------------------
const char* ssid     = "ProTournament";
//char* password = "123456789";
String passwordString = "123456789";
WiFiServer server(80);          // Set web server port number to 80
//------------------------------------------------------------------------------

//-----------------Define varibles----------------------------------------------
String header;                  // Variable to store the HTTP request header
String save_header;             //
String line1 = "";              // String to hold value of Line 1 input box
String line2 = "";              // String to hold value of Line 2 input box
String line3 = "";              // String to hold value of Line 3 input box
String line4 = "";              // String to hold value of Line 4 input box
String last_weight = "";        //
char radio_rx_array[31];        // array being recieved on xbee radio
bool radio_rx_ready = false;    // whether the rec radio string is complete
int radio_rx_pointer;           //pointer for radio rx buffer
int statt;                      // 1 = h2 lb,   2= h2 lb/oz,     3 = 357 lb,     4 = 357 lb/oz
int serial_number;              // Stores device serial number
int read_keyboard_timer;         // timer that reads the keyboard every 200ms
char output_string[31];         // converted data to send out
char temp_str[31];              //
int rc = 0;                     // use in SQL routines
String temp_val = "";           //
char weight[15];                //
int ClockTimer = 0;             //
bool no_sig_flag = 0;           // flag to prevent display from updating on no change of No Signal message
bool cb_print_2_copies;         // If checkbox should show checked or not
bool cb_print_signature_line;   // If checkbox should show checked or not
bool cb_serial_ticket;          // If checkbox should show checked or not
bool cb_print_when_locked;      // If checkbox should show checked or not
bool checkbox5_is_checked;      // If checkbox should show checked or not
bool lock_flag = false;         // flag that indicates weight is a locked value
bool cb_print_on_lock;          // check box flag for print on lock
bool isSDCardPresent = false;   // Flag checked on startup true if SD card is found
bool diagnostic_flag = false;   // Flag to send all Serial Monitor diagnostic to printer
String passwordMessage = "";    //
bool passSuccess = false;       //
int ticket;            // ticket serial number
byte Imac[6];                   // array to hold the mac address
String checkbox1_status = "";   // Holds chekbox status "checked" or "" to be injected in HTML
String checkbox2_status = "";   // Holds chekbox status "checked" or "" to be injected in HTML
String checkbox3_status = "";   // Holds chekbox status "checked" or "" to be injected in HTML
String checkbox4_status = "";   // Holds chekbox status "checked" or "" to be injected in HTML
String checkbox5_status = "";   // Holds chekbox status "checked" or "" to be injected in HTML
volatile int interruptCounter;  // varible that tracks number of interupts
volatile int totalInterruptCounter;      // counter to track total number of interrupts
int no_signal_timer;            // timeout counter used to display No Signal on display
char *database[100][2];         // database array to hold anglers name and weight
hw_timer_t * timer = NULL;      /* in order to configure the timer, we will need
                                   a pointer to a variable of type hw_timer_t,
                                   which we will later use in the Arduino setup function. */
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
                                /* we will need to declare a variable of type portMUX_TYPE,
                                   which we will use to take care of the synchronization
                                   between the main loop and the ISR, when modifying a shared variable. */
bool settingsPageFlag = false;  // True if on settings page
bool printPageFlag = false;     // True if on print page
bool updatePageFlag = false;    // True if on update page
bool changePasswordPageFlag = false; // True if on change password page
bool setTimePageFlag = false;   // True if on set time and date page
bool allowUserDefinedDate = true;  // set to false to turn off date entry form
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


//----------funtion prototypes -------------------------------------------------
void recall_eeprom_values(void);
void clear_output_buffer(void);
void clear_radio_rx_array(void);        // routine to clear rx buffer from xbee radio
void print_ticket(void);                // function to print the weigh ticket
void set_text_size(unsigned int size);  // oled set text size routine
void set_text_reverse(bool on_off);     // oled set reverse text on/off
void processRadioString();              // routine to process radio rx string
void Set_Clock(byte Year, byte Month, byte Date, byte DoW, byte Hour, byte Minute, byte Second);
// Checks status of checkbox and sets flags for proper HTML display
void checkboxStatus(String h, bool& is_checked, String& status, String number);
//int openDb(const char *filename, sqlite3 **db);
char* string2char(String str)
{
    if(str.length()!= 0)
    {
        char *p = const_cast<char*>(str.c_str());
        return p;
    }
}

//--------- start lcd and rtc ----------------------------------------------------
LiquidCrystal_I2C lcd(0x3F,20,4);                      // set the LCD address to 0x27 or 3f for a 20 chars and 4 line display
RTC_DS3231 rtc;                                        //start an instance of the real time clock named 'rtc'

//-------------Interuput routines ----------------------------------------------
void IRAM_ATTR onTimer()                // (100 ms) this is the actual interrupt(place before void setup() code)
  {
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounter++;                   //put code to perform during interrupt here
  read_keyboard_timer++;
  ClockTimer++;                          //timer used to read clock every second
  portEXIT_CRITICAL_ISR(&timerMux);
  }


//--------------------------------------------------------------------------
//-------------Start of Program -----------------------------------------
//------------------------------------------------------------------------
void setup(){
    listDir(SD, "/", 2);                      //SD card directory listing, '2' = 2 levels deep
    sqlite3_initialize();                     //start the database engine
    sqlite3 *db1;                            //declare varible as a sqlite3 file 
    Wire.begin();                             //start i2c for RTC

    //---------- declare input buttons with pullup -----------------------------
    pinMode(button_PRINT,INPUT_PULLUP);    // print button
    pinMode(button_F1,INPUT_PULLUP);       // F1
    pinMode(button_F2,INPUT_PULLUP);       // F2
    pinMode(button_F3,INPUT_PULLUP);       // F3
    pinMode(button_F4,INPUT_PULLUP);       // F4

    //----------- setup 1us counter --------------------------------------------
    timer = timerBegin(0, 80, true);            // "0" is the timer to use, '80' is the prescaler,true counts up 80mhz divided by 80 = 1 mhz or 1 usec
    timerAttachInterrupt(timer,&onTimer,true);  // "&onTimer" is the int function to call when intrrupt occurs,"true" is edge interupted
    timerAlarmWrite(timer, 100000, true);       // interupt every 1000000 times
    timerAlarmEnable(timer);                    // this line enables the timer declared 3 lines up and starts it
    ticket = 0;

    //-------------  declare serial ports and start up LCD module --------------------------------------
    /*   Note the format for setting a serial port is as follows:
        Serial2.begin(baud-rate, protocol, RX pin, TX pin);  */
    Serial1.begin(9600, SERIAL_8N1,33,32);     // XB RADIO,  RX = 33, TX = 32,
    Serial2.begin(9600, SERIAL_8N1,16,17);     // THERMAL PRINTER, RX = 16,  TX = 17
    Serial.begin(115200);                      // start serial port 0 (debug monitor and programming port)
    delay(1000);                               //time for the serial ports to setup
    lcd.init();                               //initialize the LCD display
    lcd.backlight();                          //turn on backlight


//--------------Initialize printer for upside down print -----------------------------
     Serial2.write(0x1B);                //initialize pos2 printer
     Serial2.write('@');

     Serial2.write(0x1B);                //upside down printing
     Serial2.write('{');
     Serial2.write('1');                 //change to zero for right side printing

     Serial2.write(0x1B);                //B Font 12x24
     Serial2.write('M');
     Serial2.write('1');
     Serial2.write(0x0A);
     set_text_size(0x00);               //set for small font


//--------------- diagnostic mode if F1 is held on cold boot ------------------------------------------

    if (!digitalRead(button_F1))                                     //^^^ If button 1 held on cold start, turn on diagnostic mode
        {diagnostic_flag = true;
         lcd.clear();
         lcd.setCursor(2,1);
         lcd.print("  Diagnostic  Mode");                           //display message on LCD screen
         Serial2.println("Turn printer 'OFF' and then 'ON' to exit diagnostic mode");

         Serial2.println("------------- Entering Diagnostic Mode ----------------");                             //^^^ send message to printer
         Serial2.write(0x0A);                                       //line feed
         while (!digitalRead(button_F1))                           //loop while F1 is held down
              {delay(50);}
         lcd.clear();
        }



//------------- initialize the EEPROM --------------------------------------
    if (!EEPROM.begin(EEPROM_SIZE))                                 //set aside memory for eeprom size
        {
        time_stamp_serial_monitor();
        Serial.println("failed to intialize EEPROM");               //display error to monitor
        Serial2.println("Error - failed to intialize EEPROM");      //send error code to printer
        }
    else{
         if (diagnostic_flag){
         Serial2.println("EEprom initized successfully");}
        }  
  
//-----------------Hold Print button on cold start to bring in temporary password to log on ---------------------
 /* Remove the password parameter, if you want the AP (Access Point) to be open
       if 'button_PRINT' is pulled low,(print button pressed) a temporary password will
       be displayed on the remote dispay and the password will be printed out on
       the printer  */

    if (!digitalRead(button_PRINT))                               // if print button is held down during power up
        {
        // TODO All this `if` needs to be deleted and maybe replaced with enter diagnostic mode
        Serial.println("password = 987654321");                   //display temp password to serial monitor
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Temporary Password");
        lcd.setCursor(0,1);
        lcd.print("987654321");
        while(!digitalRead(button_PRINT))                         //loop until button is released
            {delay(30);}
        WiFi.softAP(ssid,"987654321");                            //start wifi hub and require temporary password

        //------------ print a ticket with the temp password -------------------
        Serial2.println("______________________________________________");

        Serial2.println(" ");
        set_text_size(0x22);
        Serial2.println("** 987654321 **");
        set_text_size(0x00);                                      //normal text size
        Serial2.print("  ");
        Serial2.println("                 Temporary password");
        Serial2.println("  ");
        Serial2.println("Reset your WiFi password using phone or tablet.");
        Serial2.println("______________________________________________");
       }

    else                                                          //if print button is not pressed
        {
        WiFi.softAP(ssid,string2char(passwordString));           //ssid declared in setup, pw recalled from eeprom or use default
        // TODO password logging should be deleted before production
        Serial.println("password = " + passwordString);
        }

    //.softAP(const char* ssid, const char* password, int channel, int ssid_hidden, int max_connection)

    IPAddress IP = WiFi.softAPIP();                               //get the ip address
    Serial.print("AP IP address: ");                              //print ip address to SM
    Serial.println(IP);
    server.begin();                                               //start server
    lcd.clear();                                                  //clear LCD
    lcd.setCursor(0,0);
    lcd.print("WiFi network:");
    lcd.setCursor(0,1);
    lcd.print("ProTournament");
    char ip_string[30];                                          //declare a character array to hold ip address
    sprintf(ip_string,"IP = %d.%d.%d.%d",WiFi.softAPIP()[0],WiFi.softAPIP()[1],WiFi.softAPIP()[2],WiFi.softAPIP()[3]);   //this creates the ip address format to print (192.169.4.1)
    set_text_size(0X00);                                         //set printer font size to small
    Serial2.println("----------------------------------------------------------");
    set_text_size(0x11);                                         //set printer font size to 2X
    Serial2.write(0x0A);                                          //line feed
    Serial2.println(ip_string);                                  //display ip value
    Serial2.write(0x0A);                                          //line feed
    set_text_size(0x00);                                         //set text to small
    Serial2.println("in the address bar at top of the browser screen.");
    Serial2.println("Open your browser and enter the following IP address");
    Serial2.write(0x0A);                                          //line feed
    Serial2.println("---------------------------------------------------------");

    set_text_size(0X11);
    Serial2.println("WiFi network = ProTournament");
    Serial2.write(0x0A);
    set_text_size(0X00);
    Serial2.println("Use phone or tablet to log onto the following network site");
    lcd.setCursor(0,2);
    lcd.print(ip_string);

    Serial2.write(0x0A);
    Serial2.println("----------------------------------------------------------");
    if (!diagnostic_flag)
       {cut_paper();}

    char verString[10];
    sprintf(verString,"Ver. = %d.%d.%d", VERSION_NUMBER[0],VERSION_NUMBER[1],VERSION_NUMBER[2]);
    lcd.setCursor(0,3);
    lcd.print(verString);                                                 //print software version
    if (diagnostic_flag == true)                                                  //^^^diagnostic mode
        { Serial2.print(">>Software ");
          Serial2.println(verString);}
    

    delay(2000);                                                          //leave ssid and ip on oled sceen for this delay
    recall_eeprom_values();  

    cb_print_2_copies ? checkbox1_status = "checked" : checkbox1_status = "";    //set 'checkbox#_is_checked' to match 'checkbox#_status'
    cb_print_signature_line ? checkbox2_status = "checked" : checkbox2_status = "";
    cb_serial_ticket ? checkbox3_status = "checked" : checkbox3_status = "";
    cb_print_when_locked ? checkbox4_status = "checked" : checkbox4_status = "";

    lcd.clear();                                           //clear lcd display
    lcd.setCursor(0,0);
    lcd.print(line1);
    lcd.setCursor(0,1);
    lcd.print(line2);
    lcd.setCursor(0,2);
    lcd.print(line3);
    delay(3000);
    lcd.clear();
   // lcd.setCursor(0,3);
   // lcd.print(line4);                                   //line 4 on lcd used to display button functions

//-------- get the MAC address of the WiFi module ----------
    WiFi.macAddress(Imac);
    Serial.print("MAC");
    if (diagnostic_flag == true)
        {Serial2.print(">>MAC address");}
    for(int i=5;i>=0;i--)
      {
      Serial.print(":");
      Serial.print(Imac[i],HEX);                        //print the mac address to serial monitor
      if (diagnostic_flag == true)                      //^^^ print mac address to printer when in diagnostic mode
         { Serial2.print(":");
           Serial2.print(Imac[i],HEX);}                //send mac adress to printer
      }
    Serial.print("\n");
    Serial2.println("");
    
    // Check if SD card is present
    isSDCardPresent = isSDCard();                          //set flag if sd card is present
    if (!isSDCardPresent)
      {// time_stamp_serial_monitor();
        Serial.print("SD card not present");
        if (diagnostic_flag)                              //^^^ diagnostic message
          {Serial2.println(">>setup() -SD card not present");}
      }
   
    if (diagnostic_flag)                                //^^^ diagnostic message 
          {Serial2.println(">>setup() - SD card present");
           Serial2.printf(">>Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
           Serial2.printf(">>Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
          
          }

////-------------test code for data base---------------------

//note - data base used in this was created with DB browser and loaded onto sd card 
   sqlite3 *db3;                                                                 //delclare a pointer to the data base
   openDb("/sd/PTS.db", &db3);                                                  //open database on SD card, assign to 'db3'
     
   db_exec(db3, "SELECT name FROM sqlite_master WHERE type='table'");            //list tables in data base
   db_exec(db3, "INSERT INTO Angler (name,WeighInId) Values ('John Smith','55')");
   sqlite3_close(db3);                                                          //close database

 
 
    
//---------------------------------------------------------------                               
}//void setup() ending terminator



//&&&&&&&&&&&&&&&&&&&&&&&&&   Start of Program Loop  &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
void loop(){

//--------- 100 ms timer---------
   if (interruptCounter > 0)                            //every one second 100 msec int is generated
      {
      portENTER_CRITICAL(&timerMux);
      interruptCounter--;                               //reset counter to zero
      portEXIT_CRITICAL(&timerMux);
      totalInterruptCounter++;                          //increment counter for ints generated every second
      }

//-------- 1 second timer------
  if (ClockTimer >=10)                                  //update clock every one second
      {lcd_display_time();                              //display time upper left corner of lcd
       lcd_display_date();                              //display date upper right corner of lcd
       ClockTimer = 0;                                  //reset one second timer used for clock updates
      }


//--------- no signal timer -------
   if (totalInterruptCounter >=50)
      { lcd_display_time();
        totalInterruptCounter = 0;                        //reset counter
        if (++ no_signal_timer >= 2)                      //if no signal this timer times out
           { statt = 0;                                   //set display mode to 0 so "No Signal" will be displayed
             if (!no_sig_flag)                            //if flag is not set
               {
               lcd.setCursor(2,1);
               lcd.print("** No  Signal **");
               }
             no_sig_flag = 1;                             //set flag so display will not update every loop
           }
      }
//--------------- read  button routines -------------------------------------------------------

if (read_keyboard_timer >= 2)                                             //read keypad every 200 ms
     {read_keyboard_timer = 0;                                            //reset key scan timer

     if (!digitalRead(button_PRINT))                                      //if pushbutton is pressed (low condition), print the ticket
      { no_sig_flag = 0 ;                                                 //clear flag so that 'no signal' message can appear if needed
        if (diagnostic_flag)                                              //^^^ diagnostic message
          {Serial.println(">>Print button  pressed");
            Serial2.println(">>Print button pressed");}
        print_ticket();                                                   //print the weight ticket
        delay(300);
        if (checkbox1_status == "checked")                                //if checkbox "print 2 tickets" is checked
            {print_ticket();}                                             //print second ticket if print 2 copies is selected
        while (!digitalRead(button_PRINT))                                //loop while button is held down
            {delay(30);}

       if (checkbox3_status == "checked")                                 //if check box 'print serial number' is checked
          {serial_number++;                                               //increment serial number
            EEPROM.writeInt(serial_number_addr,serial_number);            //save serial number to eeprom
            Serial.println(EEPROM.readInt(serial_number_addr));           //*** diagnostic
          }
       lcd.clear();                                                       //clear lcd display
//       lcd.setCursor(3,1);
//       lcd.print("PRINTING...");                                        //display 'Printing' message to lcd
//       delay(2000);



       lcd.clear();
      }
     lcd.setCursor(0,3);
     if (!digitalRead(button_F1))                                         //F1 button
       {lcd.print("F1");
       if (diagnostic_flag)
          {Serial2.println(">>Button F1 pressed");                        //^^^ send button press diag to printer
           Serial.println(">>Button F1 pressed");
          }
       }
     else
        {lcd.print("   ");}

     lcd.setCursor(6,3);
     if (!digitalRead(button_F2))                                         //F2 button
       {lcd.print("F2");
       if (diagnostic_flag)
          {Serial.println(">>Button F2 pressed");
            Serial2.println(">>Button F2 pressed");}                      //^^^ send button press diag to printer
       }
     else
       {lcd.print("   ");}


       lcd.setCursor(11,3);
     if (!digitalRead(button_F3))                                         //F3 button
       {lcd.print("F3");
        if (diagnostic_flag)
           {Serial.println(">>Button F3 pressed");
            Serial2.println(">>Button F3 pressed");}                      //^^^ send button press diag to printer
       }
     else
        {lcd.print("   ");}

    
     if (!digitalRead(button_F4))                                       //F4 button
       { lcd.setCursor(17,3);
         lcd.print("F4");
         if (diagnostic_flag){
            Serial.println(">>Button F4 pressed");
            Serial2.println(">>Button F4 pressed");}                    //^^^ send button press diag to printer
       }
     else
       {lcd.print("   ");}


//-------------- F1 + F4 key press will reboot computer ---------------------------
  if (!digitalRead(button_F1) &&  !digitalRead(button_F4))              // If button 1 and 4 are pressed at same time reboot
       {
        delay(2000);
        if (!digitalRead(button_F1) &&  !digitalRead(button_F4))        //if still holding after 2 seconds
            {
            lcd.clear();
            lcd.setCursor(0,1);
            lcd.print("Rebooting");                                     //display 'rebooting'  on Lcd
           
            for (int i = 0; i < 11; i++){                              // delay and print dots 11 times.
                delay(100);
                lcd.print(".");
                }
             rebootEspWithReason("manual reboot");                      //reboot computer
            }
        } 
    }//end if read_keyboard_timer = 0


//--------------- radio uart recieve ---------------------------------------------------------------
      if (Serial1.available() > 0)                                  //if data in recieve buffer, send to serial monitor
          {char c;
           c = (char)Serial1.read();                                //get byte from uart buffer
           radio_rx_array[radio_rx_pointer] += c;                   //add character to radio rx buffer
           radio_rx_pointer ++;                                     //increment pointer
           if (radio_rx_pointer >=30)                               //buffer overflow
              {clear_radio_rx_array();                              //clear rx radio buffer
               radio_rx_pointer = 0;                                //reset the rx radio buffer
              }
           if (c == 0x0D || c == 0x0A)                              //if character is CR or LF then process buffer
              {c = 0x00;                                            //set this character to a null zero
               //-------------display weight on LCD-----------------------------------
               no_sig_flag = 0;                                     //clear flag used in no sig message routine
               int inc = 0;
               while (inc <= 15)
                 {if (radio_rx_array[inc] >= 31)                    //do not display characters with ascaii value of 30 or less
                        {lcd.write(radio_rx_array[inc]);            //write character to screen
                         if (radio_rx_array[inc] == 'H' && radio_rx_array[inc+1] != 'O')  //locked value and not 'HOLD'?
                             {lcd.setCursor(3,2);
                              lcd.print(" *** LOCKED ***");         //Display "locked" message on lcd
                             }
                         if (radio_rx_array[inc] == 'M')
                             {lcd.setCursor(0,2);
                              lcd.print("                  ");     //erase 'LOCKED" message if not locked
                             }
                     }
                  inc++;                                            //increment pointer
                 }

               processRadioString();
               }
          }

//-------------- start client routine ----------------------------------------------------------------



    WiFiClient client = server.available();           // Listen for incoming clients
    String updateMessage = "";                        //create a varible string  
    if (client)                                       //if client is connected
    {                                                 // If a new client connects (tablet or cell phone logs on)
        Serial.println("New Client.");                // print a message out in the serial port monitor
        String currentLine = "";                      // make a String to hold incoming data from the client
        if (client.connected()){
            if (diagnostic_flag){                     // ^^^ diagnostic code
                Serial2.println(">>loop()- Client connected...");}
            }
        while (client.connected())
        {                                             // loop while the client's connected
          
          
            if (client.available())
            {         // if there's bytes to read from the client,
                char c = client.read();               // read a byte, then
                Serial.write(c);                      // print it out the serial monitor
                header += c;                          //add character to the header string
                if (c == '\n')
                //if (header.length() > 500)
                {                                     // if the byte is a newline character
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

                        Serial.println("--------headerT:------------");            //print substring to serial monitor
                        Serial.println(headerT);
                        if (diagnostic_flag){
                           Serial2.print(">>loop() - ");
                           Serial2.println(headerT);
                           }
                        Serial.println("----------------------------");
                        // TODO Delete this line before production
                        Serial.println("password = " + passwordString);
                        // Don't display date forms if not on HTML5
                        if(!(header.indexOf("Mozilla/5.") >= 0)) {
                          bool allowUserDefinedDate = false;
                        }
                        if(!(header.indexOf("favicon") >= 0))            //id header does not contin "favicon"
                        {
                            if (headerT.indexOf("settings?") >= 0)      //if header contains "settings"
                            {
                                settingsPageFlag = true;
                                printPageFlag = false;
                                updatePageFlag = false;
                                changePasswordPageFlag = false;
                                setTimePageFlag = false;
                            } 
                            else if (headerT.indexOf("print?") >= 0)  //if header contains "print?"
                                {
                                print_ticket();                         //print weigh ticket
                                Serial.println("PRINT BUTTON WAS PRESSED ON WEB PAGE");
                                settingsPageFlag = false;
                                printPageFlag = true;
                                updatePageFlag = false;
                                setTimePageFlag = false;
                                changePasswordPageFlag = false;
                                }
                             else if (headerT.indexOf("update?") >= 0)
                                {
                                settingsPageFlag = false;
                                printPageFlag = false;
                                updatePageFlag = true;
                                changePasswordPageFlag = false;
                                setTimePageFlag = false;
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
                            else if (headerT.indexOf("changePassword?") >= 0)
                               {
                               settingsPageFlag = false;
                               printPageFlag = false;
                               updatePageFlag = false;
                               changePasswordPageFlag = true;
                               setTimePageFlag = false;
                               }
                            else if (headerT.indexOf("setDateTime?") >= 0)
                                {
                                settingsPageFlag = false;
                                printPageFlag = false;
                                updatePageFlag = false;
                                changePasswordPageFlag = false;
                                setTimePageFlag = true;
                                }
                            else
                                {
                                settingsPageFlag = false;
                                printPageFlag = false;
                                updatePageFlag = false;
                                changePasswordPageFlag = false;
                                setTimePageFlag = false;
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

                            // ATC: I don't think we need the next 4 lines anymore as I belive the checkboxStatus
                            //      function 4 blocks above sets the indivudule status. both the bool variable and
                            //      the string variable.  I leave this REMed out for now but should be deleted if no
                            //      adverse effects are detected.
                            // cb_print_2_copies ? checkbox1_status = "checked" : checkbox1_status = "";
                            // cb_print_signature_line ? checkbox2_status = "checked" : checkbox2_status = "";
                            // cb_serial_ticket ? checkbox3_status = "checked" : checkbox3_status = "";
                            // cb_print_when_locked ? checkbox4_status = "checked" : checkbox4_status = "";
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
                        // Looks for pw in header and then processes the password results if found
                        else if ((headerT.indexOf("pw=") >= 0)&& !(header.indexOf("favicon") >= 0)) //if text 'pw=' is found and text 'favicon' is not found
                        {
                            String pass1;
                            String pass2;
                            // Process password change logic
                            pass1 =  header.substring(header.indexOf("pw=")+3,header.indexOf("&pw2="));//parse out the varible strings for the the 2 passwords
                            pass2 =  header.substring(header.indexOf("pw2=")+4,header.indexOf(" HTTP"));

                            // check passwords match and are long enough
                            if (pass1 != pass2)
                            {
                                passwordMessage = "Passwords Don't Match";
                                passSuccess = false;
                                // Fail Banner Passwords don't match
                            } else
                            {
                                if (pass1.length() < 8) {
                                    passwordMessage = "Passwords needs to be 8 or more characters.";
                                    passSuccess = false;
                                    // Fail Banner Password too short
                                } else if (pass1.length() > 20) {
                                    passwordMessage = "Passwords needs to be 20 or less characters.";
                                    passSuccess = false;
                                    // Fail Banner Password too long
                                } else {
                                    passwordMessage = "Password was successfully changed.";
                                    passSuccess = true;
                                // success banner
                                // Save password to password Variable
                                //pass1.toCharArray(password,30);
                                passwordString = pass1;                           
                                // Save password to EEPROM
                                EEPROM.writeString(password_addr, pass1.substring(0,20));            //save password to eeprom
                                EEPROM.commit();                                                     //commit to eeprom
                                }
                            }

                        }
                        // Looks for date in header and then processes the date results if found
                        else if ((headerT.indexOf("date=") >= 0)&& !(header.indexOf("favicon") >= 0)) //if text 'pw=' is found and text 'favicon' is not found
                        {
                            String date;
                            // Parse EPCOH time from header
                            date =  header.substring(header.indexOf("date=")+5,header.indexOf(" HTTP"));
                            date = char_replace_http(date);
                            Serial.println("date is: " + date);

                            // Date in form Thu Nov 15 2018 15:54:19 GMT-0500 (EST)
                            //              012345678901234567890123456789012345678
                            //              Fri Aug 03 2018 02:57:46 GMT-0400 (EDT)
                            int year = date.substring(11,15).toInt();
                            String monthStr = date.substring(4,7);
                            int month;
                            if (monthStr == "Jan") {month = 1;}
                            else if (monthStr == "Feb") {month = 2;}
                            else if (monthStr == "Mar") {month = 3;}
                            else if (monthStr == "Apr") {month = 4;}
                            else if (monthStr == "May") {month = 5;}
                            else if (monthStr == "Jun") {month = 6;}
                            else if (monthStr == "Jul") {month = 7;}
                            else if (monthStr == "Aug") {month = 8;}
                            else if (monthStr == "Sep") {month = 9;}
                            else if (monthStr == "Oct") {month = 10;}
                            else if (monthStr == "Nov") {month = 11;}
                            else {month = 12;}
                            int day = date.substring(8,10).toInt();
                            int hour = date.substring(16,18).toInt();
                            int minute = date.substring(19,21).toInt();
                            int second = date.substring(22,24).toInt();
                            Serial.println("year is: " + String(year));
                            Serial.println("month is: " + String(month));
                            Serial.println("day is: " + String(day));
                            Serial.println("hour is: " + String(hour));
                            Serial.println("minute is: " + String(minute));
                            Serial.println("second is: " + String(second));

                            rtc.adjust(DateTime(year, month, day, hour, minute, second));
                        }
                        // Looks for userDate in header and then processes the date results if found
                        else if ((headerT.indexOf("UserDate=") >= 0)&& !(header.indexOf("favicon") >= 0)) //if text 'pw=' is found and text 'favicon' is not found
                        {
                            String date;
                            String time;
                            // Parse user date and time from header
                            date = header.substring(header.indexOf("UserDate=")+9,header.indexOf("&UserTime"));
                            time = header.substring(header.indexOf("UserTime=")+9,header.indexOf(" HTTP"));
                            date = char_replace_http(date);
                            time = char_replace_http(time);
                            int year = date.substring(0,4).toInt();
                            int month = date.substring(5,7).toInt();
                            int day = date.substring(8).toInt();
                            Serial.println("date is: " + date);
                            Serial.println("time is: " + time);
                            Serial.println("year is: " + String(year));
                            Serial.println("month is: " + String(month));
                            Serial.println("day is: " + String(day));
                            int hour = time.substring(0,2).toInt();
                            int minute = time.substring(3).toInt();
                            Serial.println("hour is: " + String(hour));
                            Serial.println("minute is: " + String(minute));

                            rtc.adjust(DateTime(year, month, day, hour, minute, 0));
                        }
                        // ATC: This else statement is totally unnecessary and only
                        //      serves as a place holder for future expansion
                        else    //if header did not contain text "line1" then run code in else statment below
                        {
                                // do some stuff
                        }
//--RENDER HTML-----------------------------------------------------------------
                        htmlHead(client);
                        // svg PTS logo
                        printPTSLogo(client);
//--RENDER SETTINGS PAGE--------------------------------------------------------
                        if (settingsPageFlag) {
                            pageTitle(client, "Settings");

                            // Set date and time button
                            startForm(client, "/setDateTime");
                            button(client, "Set the Date and Time", "info");
                            endForm(client);
                            //startForm(client, "[action]")
                            startForm(client, "/");
                            //inputBox(client, "[string name of variable]", [actual variable], "[label]", [smalltext? BOOL], "[small text string]")
                            inputBox(client, "Line1", line1, "Top Line", false, "Example: The Tournament Name");
                            inputBox(client, "Line2", line2, "Second Line", true, "Example: The Tournament Location");
                            inputBox(client, "Line3", line3, "Third Line", true, "Example: The Tournament Dates");
                            inputBox(client, "Line4", line4, "Bottom Line", true, "Example: A sponsor message");
                            //checkBox(client, "[String name of variable]", [actual variable], "[String label]")
                            checkBox(client, "checkbox1", checkbox1_status, "Print 2 Copies");
                            checkBox(client, "checkbox2", checkbox2_status, "Print signature line");
                            checkBox(client, "checkbox3", checkbox3_status, "Serialized ticket");
                            checkBox(client, "checkbox4", checkbox4_status, "Optional Parameter");
                            // Submit Button
                            button(client, "submit", "primary");
                            endForm(client);

                            // if the startup flag that determined if an SD card is present then display the update button
                            if(isSDCardPresent)
                            {
                              // update button
                              startForm(client, "/update");
                              button(client, "Update Firmware", "success");
                              endForm(client);
                            }
                            // change password button
                            startForm(client, "/changePassword");
                            button(client, "Change Password", "info");
                            endForm(client);
                        }
//--SET TIME PAGE---------------------------------------------------------------
                        else if (setTimePageFlag) {
                            pageTitle(client, "Set Time and Date");
                            //startForm(client, "[action]")
                            startForm(client, "/settings");

                            // Pull the date from the device and send through header
                            client.println(R"###(
                            <button style="margin-bottom:5px;" type="submit" value="Use Date/Time from this device" class="btn btn-success btn-lg btn-block" onclick="getElementById('date').value=Date()">Use Date/Time from this device</button>
                            <input type="hidden" style="visibility: hidden;" class="form-control" name="date" id="date">
                            )###");
                            endForm(client);

                            if(allowUserDefinedDate){
                                // Allow user to enter date and time then send
                                startForm(client, "/settings");
                                //inputBox(client, "[string name of variable]", [actual variable], "[label]", [smalltext? BOOL], "[small text string]")
                                inputBox(client, "UserDate", "", "Date", false, "", "date");
                                inputBox(client, "UserTime", "", "Time", false, "", "time");
                                button(client, "Update Date", "primary");
                                endForm(client);
                            }

                            // Cancel button
                            startForm(client, "/settings");
                            button(client, "Cancel", "danger");
                            endForm(client);
                        }


//--RENDER UPDATE PAGE----------------------------------------------------------
                        else if (updatePageFlag) {
                            pageTitle(client, "Update Firmware");
                            //  Add breif instructions
                            alert(client, "primary", "This will update the firmare on your device.<br>Insert an SD card with a version of the firmware loaded and click \"Check for Update\".", "" , "NOTE: you may need to reconnect to this wifi network after updating.");
                            // Update now button
                            if(updateMessage == ""){
                              startForm(client, "/checkForUpdate");
                              button(client, "Check for Update", "success");
                              endForm(client);
                            }
                            // Print message to user dynamically
                            if(updateMessage != ""){
                                alert(client, "danger", updateMessage, "ERROR!", "Please make sure you have loaded the update software in the root directory of the SD card." );
                                startForm(client, "checkForUpdate");
                                button(client, "Retry", "success");
                                endForm(client);
                            }
                            // Print table of update files
                            if (arrayOfUpdateFiles[0] != ""){
                                printTableOfUpdateFiles(client, arrayOfUpdateFiles);
                            }
                            // Cancel button
                            startForm(client, "/settings");
                            button(client, "Cancel", "danger");
                            endForm(client);
                        }
//--CHANGE PASSWORD PAGE--------------------------------------------------------

                        else if (changePasswordPageFlag) {
                            pageTitle(client, "Change Password");
                            if (passwordMessage != "") {
                                if (passSuccess) {
                                    // succes banner
                                    alert(client, "success", passwordMessage, "SUCCESS!");

                                } else {
                                    // fail banner
                                    alert(client, "danger", passwordMessage, "TRY AGAIN!");
                                }
                            }
                            if(passSuccess)
                            {
                                alert(client, "info", "The next time you login to the this WiFi Hotspot you will be required to enter your new password.", "NOTICE!");

                                // return home
                                startForm(client, "/");
                                button(client, "Return to Home Page", "success");
                                endForm(client);
                            } else
                            {
                                startForm(client, "/changePassword");
                                inputBox(client, "pw", "", "Enter New Password", true, "Must be at least 8 characters and no more than 20", "password");
                                inputBox(client, "pw2", "", "Please Reenter Password", true, "Passwords must match", "password");
                                button(client, "submit", "primary");
                                endForm(client);

                                startForm(client, "/");
                                button(client, "Cancel", "danger");
                                endForm(client);
                            }
                            passwordMessage = "";
                            passSuccess = false;
                        }
//--RENDER HOME SCREEN----------------------------------------------------------
                        else
                        {
                        pageTitle(client, "HotSpot Printer");
                        startForm(client, "/print");
                        // Big print button
                        printButton(client);
                        endForm(client);
                        // Settings Button
                        startForm(client, "/settings");
                        button(client, "Settings", "secondary");
                        endForm(client);
                        }
                        // Version number on bottom of all pages
                        bottomNav(client, VERSION_NUMBER[0], VERSION_NUMBER[1], VERSION_NUMBER[2]);
//--END RENDER HTML-------------------------------------------------------------
                        client.println();       // The HTTP response ends with another blank line
                        break;                  // Break out of the while loop
                } else {                        // if you got a newline, then clear currentLine
                        currentLine = "";
                }
            }
                else if (c != '\r') {               // if you got anything else but a carriage return character,
                    currentLine += c;               // add it to the end of the currentLine
                }
            } // END if (client.available())
        } // END while (client.connected())

        header = "";                            // Clear the header variable
        save_header = "";
        client.stop();                         // Close the connection
        Serial.println("Client disconnected.");//send status message to serial debug window
        Serial.println("");

    }  //end of 'If (Client)'

} //end of program 'loop()'

//%%%%%%%%%%%%%%%%%%%%%% functions %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
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

//----------------- Read Time date and send to serial monitor -----------------------------------
void ReadTime(void){
    DateTime now = rtc.now();
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);

    Serial.print(" (");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(") ");

    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
    }

//------------- display time on LCD upper left corner---------------------------------------------
void lcd_display_time(void){
    DateTime now = rtc.now();
    lcd.setCursor(0,0);
    if (now.hour()<10)                               //add leading zero
      {lcd.print("0");}
    lcd.print(now.hour(), DEC);
    lcd.print(':');
    if (now.minute()<10)                               //add leading zero
      {lcd.print("0");}
    lcd.print(now.minute(), DEC);
    lcd.print(':');
    if (now.second()<10)                               //add leading zero
      {lcd.print("0");}
    lcd.print(now.second(), DEC);
    lcd.print("  ");
   }
//-------------display date on LCD upper right corner ----------------------------------------------
void lcd_display_date(void){
    DateTime now = rtc.now();
    lcd.setCursor(10,0);
    if (now.month() <10)                                  //leading zero for months
       {lcd.print("0");}
    lcd.print(now.month(), DEC);
    lcd.print('/');
    if (now.day() <10)                                    //leading zero for days
       {lcd.print("0");}
    lcd.print(now.day(), DEC);
    lcd.print('/');
    lcd.print(now.year(), DEC);
}
//-------------- send time stamp to Serial Monitor --------------------------------
void time_stamp_serial_monitor(void){
    DateTime now = rtc.now();                        //get current time and date
    if (now.hour()<10)                               //add leading zero
      {Serial.print("0");}
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    if (now.minute()<10)                               //add leading zero
      {Serial.print("0");}
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    if (now.second()<10)                               //add leading zero
      {Serial.print("0");}
    Serial.print(now.second(), DEC);


    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print('/');
    Serial.print(now.year(), DEC);
}

//-------------------------- Print Ticket ----------------------------------------------
void print_ticket(void)
              {
               int i=0;
               String temp_string = "";                     //create a temp string
               Serial2.write(0x1B);                         //initialize printer
               Serial2.write('@');

               Serial2.write(0x1B);                         //upside down printing
               Serial2.write('{');
               Serial2.write('1');

               Serial2.write(0x1B);                          //B Font 12x24
               Serial2.write('M');
               Serial2.write('1');

               Serial2.write(0x1B);                         //justification: center text
               Serial2.write('a');
               Serial2.write('1');

               Serial2.write(0x1B);                         //bold mode on
               Serial2.write(0x21);
               Serial2.write(0x38);


               Serial2.write(0x1D);                         //turn smoothing on
               Serial2.write(0x62);
               Serial2.write('1');
               set_text_size(0x00);                         //1x text size
               if (line4 != "")
                      {Serial2.println(line4);}             //print sponsor line if anything is in it

//----------- signature line ------------------------------------
               if (checkbox2_status == "checked")
                      { Serial2.println("Sign________________________________________");}  //print signature line

               set_text_size(0x44);                       //5x text size
               i=0;
               while (i++ <= 8)
                  {Serial2.write(0xC4);}                  //horizontal line
               Serial2.write(0x0A);



//--------- print weight H2 & CS-19 lb mode ---------------------------------------------------
               if(statt == 1 )                                //h2 lb mode
                  {
                    Serial2.print(output_string);             //send weight value
                    set_text_size(0x11);                      //2x text size
                    Serial2.println("Lbs");                   //print "Lbs"
                    last_weight = output_string;              //save value to recall
                  }

//------------print weight H2 & CS-19 lb/oz mode ----------------------------------------------
               else if (statt == 2){                           //H2 lb/oz mode

                     Serial2.write(output_string[1]);         //send out string one byte at a time.
                     Serial2.write(output_string[2]);         //print lb value
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
                     last_weight = output_string[1]+output_string[2]+output_string[3]+"Lb"+output_string[5]+output_string[6]+output_string[7]+output_string[8]+ 0x00;
                  }
//------------ print weight 357 lb mode -----------------------------------------------
               else if ( statt == 3){                          //357 lb mode

                     Serial2.write(output_string[0]);         //send weight
                     Serial2.write(output_string[1]);
                     Serial2.write(output_string[2]);
                     Serial2.write(output_string[3]);         //decimal point
                     Serial2.write(output_string[4]);
                     Serial2.write(output_string[5]);
                     set_text_size(0x11);
                     Serial2.print("Lbs\n");
                  }

//------------print weight 357 lb/oz mode ---------------------------------------------------
               else if (statt == 4){                         //357 lb/oz mode

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
               ///      clear_output_buffer();                  //clear the output string
                   }
//-------------- print No Signal -------------------------------------------------
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

//------------ print serial number --------------------------------------------------
               if (checkbox3_status == "checked"){          //is serialized ticket check box checked
                   Serial2.printf("S/N # %08d",serial_number);  //print ticket sequence number
                   Serial2.write(0x0A);
                    if (checkbox3_status == "checked")
                    lcd.setCursor(0,0);
                   // lcd.print("Ticket# "+ String(serial_number));
                    lcd.printf("S/N # %08d       ",serial_number);
                    Serial.printf("S/N # %08d       ",serial_number);
                   }

               set_text_size(0x11);                       //character size(horiz x2   vertical x2)

//------bottom of box---------------------------------------------------------------
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
               Serial2.write(0x0A);            //line feeds

//--------------print time date stamp on ticket -------------
                set_text_size(0x00);            //normal text size
                DateTime now = rtc.now();

                if (now.hour()<10)                               //add leading zero
                  {Serial2.print("0");}
                Serial2.print(now.hour(), DEC);
                Serial2.print(':');
                if (now.minute()<10)                               //add leading zero
                  {Serial2.print("0");}
                Serial2.print(now.minute(), DEC);
                Serial2.print(':');
                if (now.second()<10)                               //add leading zero
                  {Serial2.print("0");}
                Serial2.print(now.second(), DEC);
                Serial2.print("       ");
                Serial2.print(now.month(), DEC);
                Serial2.print('/');
                Serial2.print(now.day(), DEC);
                Serial2.print('/');
                Serial2.print(now.year(), DEC);


//--------------area to insert tournament name and address and date--------------
               if (line1!= "")                         //if line 1 is not blank
                     {
                     set_text_size(0x00);            //normal text size
                     Serial2.write(0x0A);            //line feeds

                     Serial2.write(0x0A);            //line feed
                     Serial2.write(0x0A);

                     Serial2.print(line3);           //print 3rd line of text
                     Serial2.write(0x0A);

                     Serial2.print(line2);           //print second line of text
                     Serial2.write(0x0A);

                     Serial2.print(line1);           //print first line of text
                     Serial2.write(0x0A);
                    }
//--------  print pts name in reverse text --------------------------------------
               else
                   {
                    set_text_size(0x00);
                    set_text_reverse(true);
                    Serial2.printf("\n\n\n        Pro Tournament Scales         \n");
                    set_text_reverse(false);
                    Serial2.printf("stat = %d\n",stat); //diagnostic
                   }
//-------------- cut paper-----------------------------------------
               if (!diagnostic_flag)                                                 //do not cut paper in diagnostic mode
                 {cut_paper();}

               Serial2.write(0x1B);                //justification: left border
               Serial2.write('a');
               Serial2.write('0');

//-------- print ticket to LCD screen --------------------------------
           lcd.clear();
           lcd.setCursor((20-(line1.length()))/2,0);                //print line 1 and center
           lcd.print(line1);
           lcd.setCursor((20-(line2.length()))/2,1);                //print line 1 and center
           lcd.print(line2);
           lcd.setCursor((20-(line3.length()))/2,2);                //print line 1 and center
           lcd.print(line3);
           lcd.setCursor((20-(String(weight).length()))/2,3);

           lcd.setCursor(0,3);
             if (now.hour()<10)                                     //add leading zero
              {lcd.print("0");}
           lcd.print(now.hour(), DEC);
           lcd.print(':');
           if (now.minute()<10)                                     //add leading zero
              {lcd.print("0");}
           lcd.print(now.minute(), DEC);
           lcd.print(':');
           if (now.second()<10)                                     //add leading zero
              {lcd.print("0");}
           lcd.print(now.second(), DEC);
           lcd.print("  ");

           lcd.setCursor(10,3);
           if (now.month() <10)                                     //leading zero for months
               {lcd.print("0");}
           lcd.print(now.month(), DEC);
           lcd.print('/');
           if (now.day() <10)                                       //leading zero for days
               {lcd.print("0");}
           lcd.print(now.day(), DEC);
           lcd.print('/');
           lcd.print(now.year(), DEC);
           delay(2000);
 //--------- 2nd lcd screen of weigh ticket info ----------------------------------
           lcd.clear();
           lcd.setCursor((20-(line4.length()))/2,0);                //print line 4 and center
           lcd.print(line4);
           if (statt == 0)                                          //no signal
                  {
                  lcd.setCursor(0,2);
                  lcd.print("     No Signal");
                  }
           else if (statt == 1)                                     //if H2 mode
              {
                lcd.setCursor(0,2);                                 //center text on 3rd line
                lcd.print(output_string);
              }
          else if (statt ==2)
              {
               lcd.write(output_string[1]);                       //send out string one byte at a time.
               lcd.write(output_string[2]);                       //print lb value
               lcd.write(output_string[3]);
               lcd.printf("Lb");
               lcd.write(output_string[5]);                       //print oz value
               lcd.write(output_string[6]);
               lcd.write(output_string[7]);
               lcd.write(output_string[8]);
               lcd.print("oz");                                   //print the oz label with return
              }
         else if (statt == 3)
              {
               lcd.write(output_string[0]);                       //send weight
               lcd.write(output_string[1]);
               lcd.write(output_string[2]);
               lcd.write(output_string[3]);                       //decimal point
               lcd.write(output_string[4]);
               lcd.write(output_string[5]);
               lcd.print("Lbs");
              }

             delay(2000);
             // ticket = ticket + 1;                              //pointer for weigh tickets
          clear_output_buffer();                                  //clear the output string

           } //end of routine
//--------- cut paper on printer-----------------------------------------------------------------------
 void cut_paper(void)
     {Serial2.write(0x1D);                                        // "GS" cut paper
     Serial2.write('V');                                          //"V"
     Serial2.write(0x42);                                         //decimal 66
     Serial2.write(0xB0);                                         //length to feed before cut (mm)
     }
//--------- clear output buffer ---------------------------------------------------------------------------
void clear_output_buffer(void)
    {int i=0;
     while(i <= 30)
         {output_string[i] = 0;
          i=i+1;
         }
    }
//--------- set printer text size ---------------------------------
void set_text_size(unsigned int size)                           //set font size on printer
      {
      Serial2.write(0x1D);                                      // set text size to small size
      Serial2.write(0x21);
      Serial2.write(size);                                      // sizes - 00= 1,11 = 2x,22 = 3x,33 = 4x ,44= 5x
      }


//----- set printer for reverse text ------------------------------------
void set_text_reverse(bool on_off){                            //set or clear reverse text for printer epson command
      
      Serial2.write(0x1D);
      Serial2.write('B');
      if (on_off)
          Serial2.write('1');                                 //1= on 0= off
      else
          Serial2.write('0');
      }

//------------------------------------------------------------------------------
void recall_eeprom_values(void){
    line1 = (EEPROM.readString(line1_eeprom_addr));                       //recall values saved in eeprom
    line2 = (EEPROM.readString(line2_eeprom_addr));
    line3 = (EEPROM.readString(line3_eeprom_addr));
    line4 = (EEPROM.readString(line4_eeprom_addr));
    serial_number = EEPROM.readInt(serial_number_addr);                  //get ticket serial number
    Serial.println("Load S/N "+ String(serial_number));                  //print serial number to serial monitor
    cb_print_2_copies = (EEPROM.readBool(checkbox1_eeprom_addr));        //recall checkbox status (boolean)
    cb_print_signature_line = (EEPROM.readBool(checkbox2_eeprom_addr));
    cb_serial_ticket = (EEPROM.readBool(checkbox3_eeprom_addr));
    cb_print_when_locked = (EEPROM.readBool(checkbox4_eeprom_addr));
    passwordString = (EEPROM.readString(password_addr));           //retrieve password stored in eeprom
}

      
//-----------------------------------------------------------------------------
void clear_radio_rx_array(void){                               //routine to clear radio rx buffer
     int i=0;
     while(i <= 30)
      {radio_rx_array[i] = 0x00;                            //set all 30 locations to 0x00
      i=i+1;
      }
     radio_rx_pointer= 0;                                   //reset  pointer
    }


//----------------------Process radio string if flag is set--------------------------------
void processRadioString(){
  int i = 25;
  no_signal_timer = 0;                                //reset the no signal timer since a reading was sensed
  lock_flag = false;                                  //preset lock flag to false
  while(i >= 3)                                       //search from  the 7 to the 15th character in array
      { if (radio_rx_array[i] == 'H')                 //check for locked value in string
           {Serial.println("**Locked**");             //send locked value to serial monitor
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

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// sample html code for tabs
//      <ul class="nav nav-pills dark-tabs">
//      <li role="presentation" class="active dark-tabs-li">lt;a href="#" class="dark-tabs-a">Create</a><</li>
//      <li role="presentation" class="dark-tabs-li"><a href="#" class="dark-tabs-a">Design</a>></li>
//      <li role="presentation" class="dark-tabs-li"><a href="#" class="dark-tabs-a">settings</a></li>
//      <li role="presentation" class="dark-tabs-li"><a href="#" class="dark-tabs-a">share</a></li>
//      <li role="presentation" class="dark-tabs-li"><a href="#" class="dark-tabs-a">results</a></li>
//    </ul>

//add code to read radio serial number
