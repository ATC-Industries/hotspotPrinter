


/**************************************************************
  Terry Clarkson & Adam Clarkson
  11/02/18
***************************************************************


This program uses a serial port to read an XBee radio board connected to UART 1
and send the weight value recieved from a scale to a printer connected to UART 2
UART 0 is used for a debug monitor.

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


                    IO 34 & 35 do not have internal pullups

Database tables
//---------------------------------------------
db_exec(db3, "CREATE TABLE weighin(ID INTEGER NOT NULL UNIQUE,TotalFish INTEGER NOT NULL DEFAULT 0,LiveFish INTEGER DEFAULT 0,ShortFish INTEGER DEFAULT 0,Late INTEGER DEFAULT 0,weight INTEGER DEFAULT 0,adj_weight INTEGER DEFAULT 0,TimeStamp TEXT,DateStamp TEXT, PRIMARY KEY (ID)");

weighin
      ID         Int not null unique Primary Key
      TotalFish  Int not null
      LiveFish   Int Default 0
      ShortFish  Int Default 0
      Late       Int Defalut 0
      weight     Int Default 0
      adj_weight Int Default 0
      TimeStamp  text
      DateStamp  text
//-----------------------------------------------
db_exec(db3, "CREATE TABLE Angler(ID INTEGER UNIQUE NOT NULL,FirstName TEXT,LastName TEXT,MiddleInit TEXT,Address1 TEXT,Address2 TEXT,City   TEXT,State TEXT,Zip INTEGER,CellPhone INTEGER,Telephone INTEGER,SSN INTEGER,DOB INTEGER,DateStamp INTEGER,ISW9Filed INTEGER,Email TEXT,PRIMARY KEY (ID))");

Angler
      ID          int unique not null (primary key)
      FirstName   text
      LastName    text 
      MiddleInit  text
      Address1    text
      Address2    text
      City        text
      State       text
      Zip         int
      Cellphone   int
      Telephone   int
      SSN         int
      DOB         int
      DateStamp   int
      ISW9filed   int
      Email       text
      


*/
//------ Include files ---------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>            //database engine https://github.com/siara-cc/esp32_arduino_sqlite3_lib
#include <WiFi.h>               // Load Wi-Fi library
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <EEPROM.h>             //driver for eeprom
//------ files for SD card -----------------------------------------------------
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
#include "OutputPrint.h"        //routines to print data results to printer
#include <ArduinoJson.h>        // https://arduinojson.org/v5/example/generator/

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

const char* PARAM_MESSAGE = "message";
//------------- a string to hold the version numbe r---------------------
String VERSION_NUMBER = "0.0.5";   // [MAJOR, MINOR, PATCH]

//----------------- Replace with network credentials ---------------------------
const char* ssid     = "ProTournament2";
//char* password = "123456789";
String passwordString = "123456789";
//WiFiServer server(80);          // Set web server port number to 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");


//------------------------------------------------------------------------------

//-----------------Define varibles----------------------------------------------
bool bump_mode;
int Total_fish;         //bump sink values for screen entry
int Total_alive;
int Total_short;
int Total_late;



String verString;
int pnt;
long start_micro;
String old_time;
String current_date;
String current_time;
 char* system_time;
 char sSQL[200];                 //varible that holds the sql string
char lcd_buffer[25];
 
String results[75][9];          //array the holds sql data 251 results 6 columns
int rec;                        //number of records in database
String header;                  // Variable to store the HTTP request header
String save_header;             //
//String line1 = "";              // String to hold value of Line 1 input box
//String line2 = "";              // String to hold value of Line 2 input box
//String line3 = "";              // String to hold value of Line 3 input box
//String line4 = "";              // String to hold value of Line 4 input box
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
volatile int ClockTimer = 0;             //
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
bool addAnglerPageFlag = false;  // True if on add angler page
bool allowUserDefinedDate = true;  // set to false to turn off date entry form
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
byte UpArrow[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00100,
  0b01110,
  0b11111
  
};

byte DownArrow[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b11111,
  0b01110,
  0b00100
  
};

//----------funtion prototypes -------------------------------------------------
void get_date(void);
void get_time(void);
void cut_paper(void);
void check_sd_mem(void);
void checkboxStatus(String h, bool& is_checked, String& status, String number);
void lcd_display_date(void);
void lcd_display_time(void);
void save_weighin_to_database();
void recall_eeprom_values(void);
void clear_output_buffer(void);
void recall_eeprom_values(void);
void clear_output_buffer(void);
void clear_radio_rx_array(void);        // routine to clear rx buffer from xbee radio
void print_ticket(void);                // function to print the weigh ticket
//void set_text_size(unsigned int size);  // oled set text size routine
void set_text_reverse(bool on_off);     // oled set reverse text on/off
void processRadioString();              // routine to process radio rx string
void Set_Clock(byte Year, byte Month, byte Date, byte DoW, byte Hour, byte Minute, byte Second);
// Checks status of checkbox and sets flags for proper HTML display
void checkboxStatus(String h, bool& is_checked, String& status, String number);
//int openDb(const char *filename, sqlite3 **db);
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
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

//-------------Timer Interuput routines ----------------------------------------------
//void IRAM_ATTR onTimer()                // (100 ms) this is the actual interrupt(place before void setup() code)
//  {
//  portENTER_CRITICAL_ISR(&timerMux);
//  interruptCounter++;                   //put code to perform during interrupt here
//  read_keyboard_timer++;                //timer to scan keyboard keys for press
//  ClockTimer++;                          //timer used to read clock every second
//  portEXIT_CRITICAL_ISR(&timerMux);
//  }


//--------------------------------------------------------------------------
//-------------Start of Program -----------------------------------------
//------------------------------------------------------------------------
void setup(){
    listDir(SD, "/", 2);                      //SD card directory listing, '2' = 2 levels deep
    sqlite3_initialize();                     //start the database engine
    Wire.begin();                             //start i2c for RTC

    //---------- declare input buttons with pullup -----------------------------
    pinMode(button_PRINT,INPUT_PULLUP);    // print button
    pinMode(button_F1,INPUT_PULLUP);       // F1
    pinMode(button_F2,INPUT_PULLUP);       // F2
    pinMode(button_F3,INPUT_PULLUP);       // F3
    pinMode(button_F4,INPUT_PULLUP);       // F4

    //----------- setup 1us counter --------------------------------------------
//    timer = timerBegin(0, 80, true);            // "0" is the timer to use, '80' is the prescaler,true counts up 80mhz divided by 80 = 1 mhz or 1 usec
//    timerAttachInterrupt(timer,&onTimer,true);  // "&onTimer" is the int function to call when intrrupt occurs,"true" is edge interupted
//    timerAlarmWrite(timer, 100000, true);       // interupt every 1000000 times
//    timerAlarmEnable(timer);                    // this line enables the timer declared 3 lines up and starts it
//    ticket = 0;

    //-------------  declare serial ports and start up LCD module --------------------------------------
    /*   Note the format for setting a serial port is as follows:
        Serial2.begin(baud-rate, protocol, RX pin, TX pin);  */
    Serial1.begin(9600, SERIAL_8N1,33,32);     // XB RADIO,  RX = 33, TX = 32,
    Serial2.begin(9600, SERIAL_8N1,16,17);     // THERMAL PRINTER, RX = 16,  TX = 17
    Serial.begin(115200);                      // start serial port 0 (debug monitor and programming port)
    delay(1000);                               //time for the serial ports to setup
    lcd.init();                               //initialize the LCD display
    lcd.backlight();                          //turn on backlight
    lcd.createChar(0, UpArrow);               //assign custom character to location 0
    lcd.createChar(1, DownArrow);             //assign custom character to location 1

   
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
    if (!EEPROM.begin(EEPROM_SIZE)){                                 //set aside memory for eeprom size
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


    IPAddress IP = WiFi.softAPIP();                               //get the ip address
    Serial.print("AP IP address: ");                              //print ip address to SM
    Serial.println(IP);
    server.begin();                                               //start server
    lcd.clear();                                                  //clear LCD
    lcd.setCursor(0,0);
    lcd.print("WiFi network:");
    lcd.setCursor(0,1);
    lcd.print(ssid);                                             //the ssid is declared at top of code on this page
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
    Serial2.println("WiFi network = " + String(ssid));                                 //ssid declared at top of this form
    Serial2.write(0x0A);
    set_text_size(0X00);
    Serial2.println("Use phone or tablet to log onto the following network site");
    lcd.setCursor(0,2);
    lcd.print(ip_string);

    Serial2.write(0x0A);
    
   // Serial2.println("------- Software "+ VERSION_NUMBER +" --------");
    if (!diagnostic_flag)
       {cut_paper();}


    lcd.setCursor(0,3);
    lcd.print(VERSION_NUMBER);                                                 //print software version
    if (diagnostic_flag == true)                                                  //^^^diagnostic mode
        { Serial2.print(">>Software ");
          Serial2.println(VERSION_NUMBER);
         }


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
 //   delay(3000);
    lcd.clear();

//-------- get the MAC address of the WiFi module ----------
    WiFi.macAddress(Imac);
    Serial.print("MAC");
    if (diagnostic_flag == true)
        {Serial2.print(">>MAC address");}
    for(int i=5;i>=0;i--){
      Serial.print(":");
      Serial.print(Imac[i],HEX);                        //print the mac address to serial monitor
      if (diagnostic_flag == true)                      //^^^ print mac address to printer when in diagnostic mode
         { Serial2.print(":");
           Serial2.print(Imac[i],HEX);}                //send mac adress to printer
      }
    Serial.print("\n");
    Serial2.println("");
    get_date();                                        //get date on startup
    // Check if SD card is present
    isSDCardPresent = isSDCard();                          //set flag if sd card is present
    if (!isSDCardPresent)
      {
        Serial.print("SD card not present");
        if (diagnostic_flag)                              //^^^ diagnostic message
          {Serial2.println(">>setup() -SD card not present");}
      }

     else if (diagnostic_flag){                                //^^^ diagnostic message
           check_sd_mem();                            //send to printer, card memory statistics
          }

////-------------test code for data base---------------------

//note - data base used for this was created with DB browser and loaded onto sd card
   sqlite3 *db3;                                                                        //declare a pointer to the data base
   openDb("/sd/PTS.db", &db3);                                                          //open database on SD card, assign to 'db3'



 //-- add tables if they do not exist ----------

  // db_exec(db3, "DROP TABLE weighin");                        //unrem this line to erase old table and create new table
   db_exec(db3, "CREATE TABLE weighin(ID INTEGER NOT NULL UNIQUE,TotalFish INTEGER NOT NULL DEFAULT 0,LiveFish INTEGER DEFAULT 0,ShortFish INTEGER DEFAULT 0,Late INTEGER DEFAULT 0,weight INTEGER DEFAULT 0,adj_weight INTEGER DEFAULT 0,TimeStamp TEXT,DateStamp TEXT, PRIMARY KEY (ID))");
  // db_exec(db3, "DROP TABLE Angler");                              //unrem this line to erase old table and create new table
   db_exec(db3, "CREATE TABLE Angler(ID INTEGER UNIQUE NOT NULL,FirstName TEXT,LastName TEXT,MiddleInit TEXT,Address1 TEXT,Address2 TEXT,City   TEXT,State TEXT,Zip INTEGER,CellPhone INTEGER,Telephone INTEGER,SSN INTEGER,DOB INTEGER,DateStamp INTEGER,ISW9Filed INTEGER,Email TEXT,PRIMARY KEY (ID))");

 

  // db_exec(db3, "DROP TABLE Angler");                        //unrem this line to erase old table and create new table
  // // db_exec(db3, "DROP TABLE Id");
  //  db_exec(db3, "CREATE TABLE Angler(ID INTEGER UNIQUE NOT NULL,FirstName TEXT,LastName TEXT,MiddleInit TEXT,Address1 TEXT,Address2 TEXT,City   TEXT,State TEXT,Zip INTEGER,CellPhone INTEGER,Telephone INTEGER,SSN INTEGER,DOB INTEGER,DateStamp INTEGER,ISW9Filed INTEGER,Email TEXT,PRIMARY KEY (ID))");
  //  //
  //  //  db_exec(db3, "INSERT INTO Angler(FirstName,LastName,MiddleInit,Address1,Address2,City,State,Zip,CellPhone,Telephone,SSN,DOB,DateStamp,ISWFiled,Email)Values('98','John','Smith','B','555 West Street','Apt C','Memphis','TN','54678','5553954678','','321569876','11/13/61','12/18/18','1','John@google.com')");
  //    db_exec(db3, "INSERT INTO Angler(FirstName,LastName,MiddleInit) Values ('Bill','Brown','K')");
  //    db_exec(db3, "INSERT INTO Angler(FirstName,LastName,MiddleInit) Values ('Carl','Sager','W')");
  //    db_exec(db3, "INSERT INTO Angler(FirstName,LastName,MiddleInit) Values ('Steve','Phillips','A')");
  //    db_exec(db3, "INSERT INTO Angler(FirstName,LastName,MiddleInit) Values ('Brian','RedStone','C')");
  //    db_exec(db3, "INSERT INTO Angler(FirstName,LastName,MiddleInit) Values ('Mike','Bluewater','D')");
  //    db_exec(db3, "INSERT INTO Angler(FirstName,LastName,MiddleInit) Values ('Mitch','Calmer','E')");
  //    db_exec(db3, "INSERT INTO Angler(FirstName,LastName,MiddleInit) Values ('Shawn','Shipner','F')");
  //    db_exec(db3, "INSERT INTO Angler(FirstName,LastName,MiddleInit) Values ('Kim','Yellow','K')");
  //    db_exec(db3, "INSERT INTO Angler(FirstName,LastName,MiddleInit) Values ('Larry','Bager','W')");
  //    db_exec(db3, "INSERT INTO Angler(FirstName,LastName,MiddleInit) Values ('Shawn','Killmore','A')");
  //    db_exec(db3, "INSERT INTO Angler(FirstName,LastName,MiddleInit) Values ('Ernie','Pyle','C')");
  //    db_exec(db3, "INSERT INTO Angler(FirstName,LastName,MiddleInit) Values ('Roger','Pence','D')");
  //    db_exec(db3, "INSERT INTO Angler(FirstName,LastName,MiddleInit) Values ('Jeremy','Junston','E')");
  //    db_exec(db3, "INSERT INTO Angler(FirstName,LastName,MiddleInit) Values ('Fred','Widows','F')");

   // db_exec(db3, "INSERT INTO weighin (id,totalfish,livefish,shortfish,late,weight,adj_weight) Values ('1','5','4','0','5','359','570')");     //add records
   // db_exec(db3, "INSERT INTO weighin (id,totalfish,livefish,shortfish,late,weight,adj_weight) Values ('2','4','4','1','3','790','650')");
   // db_exec(db3, "INSERT INTO weighin (id,totalfish,livefish,shortfish,late,weight,adj_weight) Values ('3','5','5','3','6','1220','1098')");
   // db_exec(db3, "INSERT INTO weighin (id,totalfish,livefish,shortfish,late,weight,adj_weight) Values ('4','4','3','0','8','689','550')");
   // db_exec(db3, "INSERT INTO weighin (id,totalfish,livefish,shortfish,late,weight,adj_weight) Values ('5','4','4','3','4','389','880')");
   // db_exec(db3, "INSERT INTO weighin (id,totalfish,livefish,shortfish,late,weight,adj_weight) Values ('6','2','2','2','2','769','770')");
   // db_exec(db3, "INSERT INTO weighin (id,totalfish,livefish,shortfish,late,weight,adj_weight) Values ('7','5','4','1','5','359','444')");     //add records
   // db_exec(db3, "INSERT INTO weighin (id,totalfish,livefish,shortfish,late,weight,adj_weight) Values ('8','4','4','1','3','560','555')");
   // db_exec(db3, "INSERT INTO weighin (id,totalfish,livefish,shortfish,late,weight,adj_weight) Values ('9','5','2','3','6','1686','1666')");
   // db_exec(db3, "INSERT INTO weighin (id,totalfish,livefish,shortfish,late,weight,adj_weight) Values ('10','5','3','0','8','875','770')");
   // db_exec(db3, "INSERT INTO weighin (id,totalfish,livefish,shortfish,late,weight,adj_weight) Values ('11','5','4','3','4','890','789')");
   // db_exec(db3, "INSERT INTO weighin (id,totalfish,livefish,shortfish,late,weight,adj_weight) Values ('12','5','3','2','1','1012','912')");
   // db_exec(db3, "INSERT INTO weighin (id,totalfish,livefish,shortfish,late,weight,adj_weight) Values ('13','5','4','0','5','359','570')");     //add records
   // db_exec(db3, "INSERT INTO weighin (id,totalfish,livefish,shortfish,late,weight,adj_weight) Values ('14','4','4','1','3','790','650')");
   // db_exec(db3, "INSERT INTO weighin (id,totalfish,livefish,shortfish,late,weight,adj_weight) Values ('15','5','5','3','6','1220','1098')");
   // db_exec(db3, "INSERT INTO weighin (id,totalfish,livefish,shortfish,late,weight,adj_weight) Values ('16','4','3','0','8','689','567')");
   // db_exec(db3, "INSERT INTO weighin (id,totalfish,livefish,shortfish,late,weight,adj_weight) Values ('17','4','4','3','4','389','879')");
   // db_exec(db3, "INSERT INTO weighin (id,totalfish,livefish,shortfish,late,weight,adj_weight) Values ('18','2','2','2','2','769','789')");
   // db_exec(db3, "INSERT INTO weighin (id,totalfish,livefish,shortfish,late,weight,adj_weight) Values ('19','5','4','1','5','359','456')");     //add records
   // db_exec(db3, "INSERT INTO weighin (id,totalfish,livefish,shortfish,late,weight,adj_weight) Values ('20','4','4','1','3','560','567')");
   // db_exec(db3, "INSERT INTO weighin (id,totalfish,livefish,shortfish,late,weight,adj_weight) Values ('21','5','2','3','6','1686','1678')");
   // db_exec(db3, "INSERT INTO weighin (id,totalfish,livefish,shortfish,late,weight,adj_weight) Values ('22','5','3','0','8','875','789')");
   // db_exec(db3, "INSERT INTO weighin (id,totalfish,livefish,shortfish,late,weight,adj_weight) Values ('23','5','4','3','4','900','897')");
   // db_exec(db3, "INSERT INTO weighin (id,totalfish,livefish,shortfish,late,weight,adj_weight) Values ('24','4','3','2','1','1012','987')");


   Serial.printf("----List Tables ---------\n\r");
     db_exec(db3, "SELECT name FROM sqlite_master WHERE type='table'");                 //list tables in data base
    Serial.printf("----End of Tables ---------\n\r");

//     db_exec(db3, "SELECT * FROM Angler ORDER BY WeighInId DESC");                  //list entire data base
     Serial.println("--- weighin results by adj_weight ------");
     db_exec(db3, "SELECT * FROM weighin Order BY adj_weight DESC");
     db_exec(db3, "SELECT * FROM Angler");                                 //total number of records in table
//     db_exec(db3,"SELECT * FROM Angler WHERE ROWID = 7");

 //-----  example to pass varibles to a sql query---------
     char *namev = "Mike Joes";
     char *IDv = "4";


//     sprintf(sSQL,"SELECT * FROM Angler WHERE ROWID = %s",IDv);                  //search database by rowid
//     db_exec(db3,sSQL);                                                          //this is theactual query to database
//      sprintf(sSQL,"SELECT * FROM Angler WHERE name = '%s'",namev);              //search database by name
//     db_exec(db3,sSQL);
//      sprintf(sSQL,"SELECT * FROM Angler WHERE WeighInId = %s",IDv);             //search database by WEighin id
//     db_exec(db3,sSQL);
    sqlite3_close(db3);                                                         //close database

     int r = 0;
//     Serial.printf("----array values -----  %d records ------\n\r",rec);
//
//     while (r <= rec-1){                                                          //Print all records found in query
//        int i = 0;
//        while (i <= 6)                                                            //display the 7 columns
//         {
//            Serial.print( results[r][i]+"\t");                                    //display all the column values (add tab)
//           i++;
//          }
//          Serial.println(" ");
//        r++;                                                                      //advance to next record
//        }
//     Serial.printf("---------- end of array ---------------------");

 //Serial2.print("\0x1D\0x49\0x01");    //respond with printer model number



// ASYNC Testing

if(!SPIFFS.begin()) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
}

ws.onEvent(onWsEvent);
server.addHandler(&ws);

events.onConnect([](AsyncEventSourceClient *client){
        client->send("hello!",NULL,millis(),1000);
});
server.addHandler(&events);

server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", String(ESP.getFreeHeap()));
});

server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

server.on("/addangler", HTTP_ANY, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/addangler.html", "text/html");
});

//  Take in add angler form data and do stuff with it
server.on("/add", HTTP_POST, [](AsyncWebServerRequest *request){
        request->redirect("/addangler");
        // Build sql insertion string
        sprintf(sSQL,"INSERT INTO Angler(FirstName,LastName,MiddleInit,Address1,Address2,City,State,Zip,CellPhone,Telephone,DOB,Email)Values('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s')",
                request->getParam(0)->value().c_str(),request->getParam(1)->value().c_str(),request->getParam(2)->value().c_str(),request->getParam(3)->value().c_str(),request->getParam(4)->value().c_str(),
                request->getParam(5)->value().c_str(),request->getParam(6)->value().c_str(),request->getParam(7)->value().c_str(),request->getParam(8)->value().c_str(),request->getParam(9)->value().c_str(),
                request->getParam(10)->value().c_str(),request->getParam(11)->value().c_str());
        addToAnglerDB(sSQL);
});

server.on("/login", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/login.html", "text/html");
});

// Route to load signin.css file
server.on("/signin.css", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/signin.css", "text/css");
});

// Route to load style.css file
server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/style.css", "text/css");
});

// Route to load bootstrap.css file
server.on("/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/bootstrap.min.css", "text/css");
});

// Route to load logo file
server.on("/pts.png", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/pts.png", "image/png");
});

// Route to load logo file
server.on("/pts.svg", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/pts.svg", "image/svg+xml");
});
server.on("/pts-white.svg", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/pts-white.svg", "image/svg+xml");
});

// Route to load favicon.ico file
server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/favicon.ico", "image/x-icon");
});

// Route to load jQuery js file
server.on("/jquery-3.3.1.slim.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/jquery-3.3.1.slim.min.js", "application/javascript");
});

// Route to load popper js file
server.on("/popper.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/popper.min.js", "application/javascript");
});

// Route to load bootstrap.js file
server.on("/bootstrap.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/bootstrap.min.js", "application/javascript");
});

// Route to load jquery js file
server.on("/jquery-slim.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/jquery-slim.min.js", "application/javascript");
});

server.onNotFound([](AsyncWebServerRequest *request){
        request->send(404);
});
server.begin();


//---------------------------------------------------------------

int val = heap_caps_get_free_size(MALLOC_CAP_8BIT);                            //get available stack size
 Serial.println("Available stack" + val);


 start_micro = micros();
 Total_fish = 0;
 Total_alive = 0;
 Total_short = 0;
 Total_late = 0;
}//void setup() ending terminator



//&&&&&&&&&&&&&&&&&&&&&&&&&   Start of Program Loop  &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
void loop(){

if((micros() - start_micro) >= 100000)
    {
    ++ClockTimer;
    ++totalInterruptCounter;
    ++read_keyboard_timer;
    start_micro = micros();
    }
//--------- 100 ms timer-----------------------------
//   if (interruptCounter > 0)                            //every one second 100 msec int is generated
//      {
//      portENTER_CRITICAL(&timerMux);
//      interruptCounter--;                               //reset counter to zero
//      portEXIT_CRITICAL(&timerMux);
//      totalInterruptCounter++;                          //increment counter for ints generated every second
//      }

//-------- 1 second timer-----------------------------
  if (ClockTimer >=10)                                  //update clock every one second
      {get_time();
      // get_date();
       //lcd_display_time();                              //display time upper left corner of lcd
      // lcd_display_date();                              //display date upper right corner of lcd

       ClockTimer = 0;                                  //reset one second timer used for clock updates

     // int val = heap_caps_get_free_size(MALLOC_CAP_8BIT); //diagnnostic to display available heap size
     // Serial.println(val);
      }


//--------- no signal timer -------(5 seconds)----------
   if (totalInterruptCounter >=50)
      { //lcd_display_time();
        totalInterruptCounter = 0;                        //reset counter
        if (++ no_signal_timer >= 2)                      //if no signal this timer times out
           { statt = 0;                                   //set display mode to 0 so "No Signal" will be displayed
             if (!no_sig_flag)                            //if flag is not set
               {
                if (bump_mode == false)
                   {lcd.clear();                                //clear lcd screen
                    lcd.setCursor(2,1);
                    lcd.print("** No  Signal **");}       //only display if not in bump sink mode
                   }
             no_sig_flag = 1;                             //set flag so display will not update every loop
           }
      }
//--------------- read  button routines -------------------------------------------------------

if (read_keyboard_timer >= 2)                                             //read keypad every 200 ms
     {

//----  PRINT button pressed?  -------------
     if (!digitalRead(button_PRINT))                                      //if pushbutton is pressed (low condition), print the ticket
      { no_sig_flag = 0 ;                                                 //clear flag so that 'no signal' message can appear if needed
        if (diagnostic_flag)                                              //^^^ diagnostic message
          {Serial.println(">>Print button  pressed");
            Serial2.println(">>Print button pressed");}
       if ((Total_fish != 0) ) ///print ticket if bump sink info has been entered
           {print_ticket();                                                   //print the weight ticket
            save_weighin_to_database();                                      //save bumbsink infor weight and adj weight to database
            bump_mode = false;                                              //get out of bump mode after ticket is printed
            delay(300);
            if (checkbox1_status == "checked")                                //if checkbox "print 2 tickets" is checked
               {print_ticket();}                                             //print second ticket if print 2 copies is selected
            while (!digitalRead(button_PRINT))                                //loop while button is held down
               {delay(30);}
            lcd.clear();
            lcd.setCursor(3,1);
            lcd.print("PRINTING...         ");                                       //display 'Printing' message to lcd
            delay(2000);
            lcd.clear();
            Total_fish = 0;
            Total_alive = 0;
            Total_short = 0;
            Total_late = 0;
           }
      else           //else code if all bump sink info has not been entered
         {
          if (bump_mode == false)                                         //toggle bump mode on or off
             { bump_mode = true;
             Total_fish = 0;
             Total_alive = 0;
             Total_short = 0;
             Total_late = 0;
             lcd.setCursor(0,1);
             lcd.print("                    ");
             lcd.setCursor(0,2);
             lcd.print("Tot Alive Short Late");
             lcd.setCursor(0,3);
             sprintf(lcd_buffer,"  %d    %d    %d    %d",Total_fish,Total_alive,Total_short,Total_late);
             lcd.print(lcd_buffer);
             
             Serial.println("bump_mode = true");
             }
          else
             {bump_mode = false;   
              lcd.setCursor(0,1);
              lcd.print("                    ");                            //clear lines 2,3,4
              lcd.setCursor(0,2);
              lcd.print("                    ");
              lcd.setCursor(0,3);
              lcd.print("                    ");
              Serial.println("bump_mode = false");

             }
        }
        read_keyboard_timer = 0;
      }

//---- F1 button pressed? ----------------
     lcd.setCursor(1,3);
     if (!digitalRead(button_F1))                                        //F1 button
       { read_keyboard_timer = 0;
        if (bump_mode == true)
           {
            if(++Total_fish > 5)                                     //increment and hold at 5 max
               {Total_fish = 0;}
            lcd.setCursor(2,3);
            lcd.print(Total_fish);
            delay(150);
           }
        else
        {
        lcd.setCursor(2,3);
        lcd.write(byte(1));                                             //show down arrow icon
        lcd.print("  ");

         sqlite3 *db3;
         openDb("/sd/PTS.db", &db3);                                     //open the database
      // db_exec(db3, "DELETE FROM Angler WHERE lastName = ''");       //delete records with no last name
         db_exec(db3, "SELECT * FROM Angler ");   //query the angler list and sort by boat numb decending
         sqlite3_close(db3);                                             //close database

         print_results();                                                //send results to printer
         delay(1000);                                                    //delay, so 2 copies do not print
         if (diagnostic_flag)
          {Serial2.println(">>Button F1 pressed");                       //^^^ send button press diag to printer
           Serial.println(">>Button F1 pressed");
          }
       }
//     else{
//         lcd.setCursor(0,3);
//         lcd.print("Res");
//        }
       }
//----------F2 button press ---------------
     lcd.setCursor(6,3);
     if (!digitalRead(button_F2))                                         //F2 button
       {read_keyboard_timer = 0;
         if (bump_mode == true)
           {
            if(++Total_alive > Total_fish)                                     //increment and hold at 5 max
               {Total_alive = 0;}
            lcd.setCursor(7,3);
            lcd.print(Total_alive);
           delay(150);
           }
        else
        {
        lcd.print("F2");
        sqlite3 *db3;
         openDb("/sd/PTS.db", &db3);                                     //open the database
         
         db_exec(db3, "SELECT Angler.id,Angler.FirstName,Angler.LastName,weighin.totalFish,weighin.liveFish,weighin.shortFish,weighin.late,weighin.weight,weighin.adj_weight FROM angler INNER JOIN Weighin on Weighin.ID = Angler.ID ORDER BY adj_weight DESC"); //query the results list and sort by final weight numb decending
         sqlite3_close(db3);                                             //close database
         Serial.println("Database closed at 669");
        print_weigh_results();

       if (diagnostic_flag)
          {Serial.println(">>Button F2 pressed");
            Serial2.println(">>Button F2 pressed");}                      //^^^ send button press diag to printer
       delay(1000);                                                        //switch debounce
       }
       }
    

//--------- F3 button pressed? -------------------
       lcd.setCursor(11,3);
     if (!digitalRead(button_F3))                                         //F3 button
       {
        if (bump_mode == true)
           {
            if(++Total_short > Total_fish)                                     //increment and hold at total fish max
               {Total_short = 0;}
            lcd.setCursor(12,3);
            lcd.print(Total_short);
            delay(150);
           }
        else
          {
           pnt = pnt+1;                                                      //increment pointer 
           read_keyboard_timer = 0;
           sqlite3 *db3;
           openDb("/sd/PTS.db", &db3);                                      //open the database
           sprintf(sSQL,"SELECT ID, LastName, FirstName FROM Angler WHERE id = %d",pnt);                  //search database by rowid
           db_exec(db3,sSQL);
           sqlite3_close(db3);
           lcd.setCursor(0,0);
           lcd.print("                    ");
           lcd.setCursor(0,0);
           lcd.print(results[0][0]+ "   "+ results[0][1]+", "+ results[0][2]);  //print column 1,2,3 to lcd screen
           bump_mode = false;                                                   //reset mode to non bump mode when new angler is selected
           if (rec == 0)                                                        // 'rec' is the number of records returned from query
                {pnt = pnt-1;
                lcd.setCursor(0,0);
                lcd.print("  -- End of file -- ");                              //if last record then display end of file
                }
           if (diagnostic_flag)
             {Serial.println(">>Button F3 pressed");
              Serial2.println(">>Button F3 pressed");                        //^^^ send button press diag to printer
             }
           
          lcd.setCursor(13,3);
          lcd.write(byte(0));                                               //up arrow key
              
           }
     }// if (!digitalRead(button_F3))
      

//-------- F4 button pressed? ---------------------

     if (!digitalRead(button_F4))                                       //F4 button
       {
        if (bump_mode == true)
           {
            if(++Total_late > 15)                                     //increment and hold at total fish max
               {Total_late = 0;}
            lcd.setCursor(17,3);
            lcd.print(Total_late);
            lcd.print(" ");
            delay(150);
           }
        else
          {
           if (pnt >1)
               {pnt = pnt-1;}
         sqlite3 *db3;
         openDb("/sd/PTS.db", &db3);
         sprintf(sSQL,"SELECT ID, LastName,FirstName FROM Angler WHERE id = %d",pnt);                  //search database by rowid
         db_exec(db3,sSQL);
         sqlite3_close(db3);
         lcd.setCursor(0,0);
         lcd.print("                    ");                                       //clear line
         lcd.setCursor(0,0);
         lcd.print(results[0][0]+ "   "+ results[0][1]+", "+ results[0][2]);
         bump_mode = false;                                                   //unset bump mode when new angler is selected
         read_keyboard_timer = 0;
      
         if (diagnostic_flag){
            Serial.println(">>Button F4 pressed");
            Serial2.println(">>Button F4 pressed");                    //^^^ send button press diag to printer
            }
       else
         {lcd.setCursor(18,3);
          lcd.write(byte(1));                                               //down arrow key
          //lcd.print("  ");
          }
       }//else
     }// if (!digitalRead(button_F4)) 

//-------------- F1 + F4 key press will reboot computer ---------------------------
  if (!digitalRead(button_F1) &&  !digitalRead(button_F4))              // If button 1 and 4 are pressed at same time reboot
       {read_keyboard_timer = 0;
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


//--------------- printer uart recieve ---------------------------------------------------------------
      if (Serial2.available() > 0)                                   //feedback from the printer
           {char c;
           c = (char)Serial1.read();
           Serial.print(c);                                            //send to serial monitor
           if (c == 0x00)                                           //if null zero
             Serial.println("");
           }

//------------ radio uart -----------------------------------------------------------------------------
      if (Serial1.available() > 0)                                  //if data in radio recieve buffer, send to serial monitor
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
               lcd.setCursor(2,1);
               while (inc <= 15)
                 {if (radio_rx_array[inc] >= 31)                    //do not display characters with ascaii value of 30 or less
                        {                       //center up weight
                          lcd.write(radio_rx_array[inc]);            //write character to screen
                         if (radio_rx_array[inc] == 'H' && radio_rx_array[inc+1] != 'O')  //locked value and not 'HOLD'?
                             {lcd.setCursor(3,2);
                              lcd.print(" *** LOCKED ***");         //Display "locked" message on lcd
                             }
                         if (radio_rx_array[inc] == 'M')
                             {
                              lcd.setCursor(0,2);
                              lcd.print("                  ");     //erase 'LOCKED" message if not locked
                             }
                     }
                  inc++;                                            //increment pointer
                 }

               processRadioString();
               }//if (c == 0x0D || c == 0x0A)
          }// if (Serial1.available() > 0)
/*
//-------------- start client routine ----------------------------------------------------------------

//
//
//     WiFiClient client = server.available();           // Listen for incoming clients
//     String updateMessage = "";                        //create a varible string
//     if (client)                                       //if client is connected
//     {                                                 // If a new client connects (tablet or cell phone logs on)
//         Serial.println("New Client.");                // print a message out in the serial port monitor
//         String currentLine = "";                      // make a String to hold incoming data from the client
//         if (client.connected()){
//             if (diagnostic_flag){                     // ^^^ diagnostic code
//                 Serial2.println(">>loop()- Client connected...");}
//             }
//         while (client.connected())
//         {                                             // loop while the client's connected
//
//
//             if (client.available())
//             {         // if there's bytes to read from the client,
//                 char c = client.read();               // read a byte, then
//                 Serial.write(c);                      // print it out the serial monitor
//                 header += c;                          //add character to the header string
//                 if (c == '\n')
//                 //if (header.length() > 500)
//                 {                                     // if the byte is a newline character
//                                                       // if the current line is blank, you got two newline characters in a row.
//                                                       // that's the end of the client HTTP request, so send a response:
//                     if (currentLine.length() == 0)
//                     {
//                         // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
//                         // and a content-type so the client knows what's coming, then a blank line:
//                         client.println("HTTP/1.1 200 OK");
//                         client.println("Content-type:text/html");
//                         client.println("Connection: close");
//                         client.println();
//
//                         //-----read the returned information from form when submit button is pressed and save to memory--------
//                         String headerT = header.substring(0,header.indexOf("Host:")); //create substring from begining to the word 'Host:'
//                         Serial.print("running line 807");
//                         Serial.println("--------headerT:------------");            //print substring to serial monitor
//                         Serial.println(headerT);
//                         if (diagnostic_flag){
//                            Serial2.print(">>loop() - ");
//                            Serial2.println(headerT);
//                            }
//                         Serial.println("----------------------------");
//                         // TODO Delete this line before production
//                         Serial.println("password = " + passwordString);
//                         // Don't display date forms if not on HTML5
//                         if(!(header.indexOf("Mozilla/5.") >= 0)) {
//                           bool allowUserDefinedDate = false;
//                         }
//                         if(!(header.indexOf("favicon") >= 0))            //id header does not contin "favicon"
//                         {
//                             if (headerT.indexOf("settings?") >= 0)      //if header contains "settings"
//                             {
//                                 settingsPageFlag = true;
//                                 printPageFlag = false;
//                                 updatePageFlag = false;
//                                 changePasswordPageFlag = false;
//                                 setTimePageFlag = false;
//                                 addAnglerPageFlag = false;
//                             }
//                             else if (headerT.indexOf("print?") >= 0)  //if header contains "print?"
//                                 {
//                                 print_ticket();                         //print weigh ticket
//                                 Serial.println("PRINT BUTTON WAS PRESSED ON WEB PAGE");
//                                 settingsPageFlag = false;
//                                 printPageFlag = true;
//                                 updatePageFlag = false;
//                                 setTimePageFlag = false;
//                                 changePasswordPageFlag = false;
//                                 addAnglerPageFlag = false;
//                                 }
//                              else if (headerT.indexOf("update?") >= 0)
//                                 {
//                                 settingsPageFlag = false;
//                                 printPageFlag = false;
//                                 updatePageFlag = true;
//                                 changePasswordPageFlag = false;
//                                 setTimePageFlag = false;
//                                 addAnglerPageFlag = false;
//                                 }
//                             else if (headerT.indexOf("checkForUpdate?") >= 0)
//                                 {
//                                 checkForUpdateFirmware(updateMessage);
//                                 }
//                             else if (headerT.indexOf("doUpdate") >= 0)
//                             {
//                                 String strIndex =  header.substring(header.indexOf("doUpdate")+8,header.indexOf("?"));//parse out the varible strings for the the 4 lines
//                                 int index = strIndex.toInt();
//                                 updateFirmware(updateMessage, arrayOfUpdateFiles[index]);
//                             }
//                             else if (headerT.indexOf("changePassword?") >= 0)
//                                {
//                                settingsPageFlag = false;
//                                printPageFlag = false;
//                                updatePageFlag = false;
//                                changePasswordPageFlag = true;
//                                setTimePageFlag = false;
//                                addAnglerPageFlag = false;
//                                }
//                             else if (headerT.indexOf("setDateTime?") >= 0)
//                                 {
//                                 settingsPageFlag = false;
//                                 printPageFlag = false;
//                                 updatePageFlag = false;
//                                 changePasswordPageFlag = false;
//                                 setTimePageFlag = true;
//                                 addAnglerPageFlag = false;
//                                 }
//                             else if (headerT.indexOf("addAngler?") >= 0)
//                                 {
//                                 settingsPageFlag = false;
//                                 printPageFlag = false;
//                                 updatePageFlag = false;
//                                 changePasswordPageFlag = false;
//                                 setTimePageFlag = false;
//                                 addAnglerPageFlag = true;
//                                 }
//                             else
//                                 {
//                                 settingsPageFlag = false;
//                                 printPageFlag = false;
//                                 updatePageFlag = false;
//                                 changePasswordPageFlag = false;
//                                 setTimePageFlag = false;
//                                 addAnglerPageFlag = false;
//                                 }
//                         }
//                         // Looks for Line1 in header and then processes the SETTINGS results if found
//                         if ((headerT.indexOf("Line1=") >= 0)&& !(header.indexOf("favicon") >= 0)) //if text 'Line1=' is found and text 'favicon' is not found
//                         {
//                             Serial.println("********found it (screen one) *********************************************");
//                             line1 =  header.substring(header.indexOf("Line1=")+6,header.indexOf("&Line2="));//parse out the varible strings for the the 4 lines
//                             line2 =  header.substring(header.indexOf("Line2=")+6,header.indexOf("&Line3="));
//                             line3 =  header.substring(header.indexOf("Line3=")+6,header.indexOf("&Line4="));
//                             // Check if line 4 is end of data or if checkbox info follows
//                             if (headerT.indexOf("&check") >= 0)
//                             {line4 =  headerT.substring(headerT.indexOf("Line4=")+6,headerT.indexOf("&check"));}
//                             else
//                             {line4 =  headerT.substring(headerT.indexOf("Line4=")+6,headerT.indexOf(" HTTP"));}
//
//                             // Check if checkbox is checked
//                             checkboxStatus(headerT, cb_print_2_copies, checkbox1_status, "1");
//                             checkboxStatus(headerT, cb_print_signature_line, checkbox2_status, "2");
//                             checkboxStatus(headerT, cb_serial_ticket, checkbox3_status, "3");
//                             checkboxStatus(headerT, cb_print_when_locked, checkbox4_status, "4");
//
//                             line1 = char_replace_http(line1); //remove and replace http characters with space
//                             line2 = char_replace_http(line2);
//                             line3 = char_replace_http(line3);
//                             line4 = char_replace_http(line4);
//                             //-----save varibles to eeprom---------------------------
//                             EEPROM.writeString(line1_eeprom_addr, line1.substring(0,40)); //save input box info after to trimming
//                             EEPROM.writeString(line2_eeprom_addr, line2.substring(0,40));
//                             EEPROM.writeString(line3_eeprom_addr, line3.substring(0,40));
//                             EEPROM.writeString(line4_eeprom_addr, line4.substring(0,40));
//
//                             EEPROM.writeBool(checkbox1_eeprom_addr,cb_print_2_copies); //boolean true if checked false if not checked
//                             EEPROM.writeBool(checkbox2_eeprom_addr,cb_print_signature_line);
//                             EEPROM.writeBool(checkbox3_eeprom_addr,cb_serial_ticket);
//                             EEPROM.writeBool(checkbox4_eeprom_addr,cb_print_when_locked);
//                             EEPROM.commit();                         ////save to eeprom
//
//                             // ATC: I don't think we need the next 4 lines anymore as I belive the checkboxStatus
//                             //      function 4 blocks above sets the indivudule status. both the bool variable and
//                             //      the string variable.  I leave this REMed out for now but should be deleted if no
//                             //      adverse effects are detected.
//                             // cb_print_2_copies ? checkbox1_status = "checked" : checkbox1_status = "";
//                             // cb_print_signature_line ? checkbox2_status = "checked" : checkbox2_status = "";
//                             // cb_serial_ticket ? checkbox3_status = "checked" : checkbox3_status = "";
//                             // cb_print_when_locked ? checkbox4_status = "checked" : checkbox4_status = "";
//                             //-------------- display varibles on serial monitor  -----------------------------------
//                             Serial.println("********START HEADER*********************************************");
//                             Serial.println(header);
//                             Serial.println("********END HEADER*********************************************");
//                             Serial.println(line1);
//                             Serial.println(line2);
//                             Serial.println(line3);
//                             Serial.println(line4);
//                             Serial.println("Checkbox1: " + checkbox1_status);
//                             Serial.println("Checkbox2: " + checkbox2_status);
//                             Serial.println("Checkbox3: " + checkbox3_status);
//                             Serial.println("Checkbox4: " + checkbox4_status);
//
//                         }
//                         // Looks for pw in header and then processes the password results if found
//                         else if ((headerT.indexOf("pw=") >= 0)&& !(header.indexOf("favicon") >= 0)) //if text 'pw=' is found and text 'favicon' is not found
//                         {
//                             String pass1;
//                             String pass2;
//                             // Process password change logic
//                             pass1 =  header.substring(header.indexOf("pw=")+3,header.indexOf("&pw2="));//parse out the varible strings for the the 2 passwords
//                             pass2 =  header.substring(header.indexOf("pw2=")+4,header.indexOf(" HTTP"));
//
//                             // check passwords match and are long enough
//                             if (pass1 != pass2)
//                             {
//                                 passwordMessage = "Passwords Don't Match";
//                                 passSuccess = false;
//                                 // Fail Banner Passwords don't match
//                             } else
//                             {
//                                 if (pass1.length() < 8) {
//                                     passwordMessage = "Passwords needs to be 8 or more characters.";
//                                     passSuccess = false;
//                                     // Fail Banner Password too short
//                                 } else if (pass1.length() > 20) {
//                                     passwordMessage = "Passwords needs to be 20 or less characters.";
//                                     passSuccess = false;
//                                     // Fail Banner Password too long
//                                 } else {
//                                     passwordMessage = "Password was successfully changed.";
//                                     passSuccess = true;
//                                 // success banner
//                                 // Save password to password Variable
//                                 //pass1.toCharArray(password,30);
//                                 passwordString = pass1;
//                                 // Save password to EEPROM
//                                 EEPROM.writeString(password_addr, pass1.substring(0,20));            //save password to eeprom
//                                 EEPROM.commit();                                                     //commit to eeprom
//                                 }
//                             }
//
//                         }
//                         // Looks for date in header and then processes the date results if found
//                         else if ((headerT.indexOf("date=") >= 0)&& !(header.indexOf("favicon") >= 0)) //if text 'pw=' is found and text 'favicon' is not found
//                         {
//                             String date;
//                             // Parse EPCOH time from header
//                             date =  header.substring(header.indexOf("date=")+5,header.indexOf(" HTTP"));
//                             date = char_replace_http(date);
//                             Serial.println("date is: " + date);
//
//                             // Date in form Thu Nov 15 2018 15:54:19 GMT-0500 (EST)
//                             //              012345678901234567890123456789012345678
//                             //              Fri Aug 03 2018 02:57:46 GMT-0400 (EDT)
//                             int year = date.substring(11,15).toInt();
//                             String monthStr = date.substring(4,7);
//                             int month;
//                             if (monthStr == "Jan") {month = 1;}
//                             else if (monthStr == "Feb") {month = 2;}
//                             else if (monthStr == "Mar") {month = 3;}
//                             else if (monthStr == "Apr") {month = 4;}
//                             else if (monthStr == "May") {month = 5;}
//                             else if (monthStr == "Jun") {month = 6;}
//                             else if (monthStr == "Jul") {month = 7;}
//                             else if (monthStr == "Aug") {month = 8;}
//                             else if (monthStr == "Sep") {month = 9;}
//                             else if (monthStr == "Oct") {month = 10;}
//                             else if (monthStr == "Nov") {month = 11;}
//                             else {month = 12;}
//                             int day = date.substring(8,10).toInt();
//                             int hour = date.substring(16,18).toInt();
//                             int minute = date.substring(19,21).toInt();
//                             int second = date.substring(22,24).toInt();
//                             Serial.println("year is: " + String(year));
//                             Serial.println("month is: " + String(month));
//                             Serial.println("day is: " + String(day));
//                             Serial.println("hour is: " + String(hour));
//                             Serial.println("minute is: " + String(minute));
//                             Serial.println("second is: " + String(second));
//
//                             rtc.adjust(DateTime(year, month, day, hour, minute, second));
//                         }
//                         // Looks for userDate in header and then processes the date results if found
//                         else if ((headerT.indexOf("UserDate=") >= 0)&& !(header.indexOf("favicon") >= 0)) //if text 'pw=' is found and text 'favicon' is not found
//                         {
//                             String date;
//                             String time;
//                             // Parse user date and time from header
//                             date = header.substring(header.indexOf("UserDate=")+9,header.indexOf("&UserTime"));
//                             time = header.substring(header.indexOf("UserTime=")+9,header.indexOf(" HTTP"));
//                             date = char_replace_http(date);
//                             time = char_replace_http(time);
//                             int year = date.substring(0,4).toInt();
//                             int month = date.substring(5,7).toInt();
//                             int day = date.substring(8).toInt();
//                             Serial.println("date is: " + date);
//                             Serial.println("time is: " + time);
//                             Serial.println("year is: " + String(year));
//                             Serial.println("month is: " + String(month));
//                             Serial.println("day is: " + String(day));
//                             int hour = time.substring(0,2).toInt();
//                             int minute = time.substring(3).toInt();
//                             Serial.println("hour is: " + String(hour));
//                             Serial.println("minute is: " + String(minute));
//
//                             rtc.adjust(DateTime(year, month, day, hour, minute, 0));
//                         }
// //                         else if ((headerT.indexOf("firstName=") >= 0)&& !(header.indexOf("favicon") >= 0)) //if text 'first_name=' is found and text 'favicon' is not found
// //                         {
// //                             String firstName = header.substring(header.indexOf("firstName=")+11,header.indexOf("&lastName="));//parse out the varible strings for the the 4 lines
// //                             String lastName =  header.substring(header.indexOf("lastName=")+10,header.indexOf("&middleName="));
// //                             String middleName =  header.substring(header.indexOf("middleName=")+12,header.indexOf("&address1="));
// //                             String address1 =  header.substring(header.indexOf("address1=")+9,header.indexOf("&address2="));
// //                             String address2 =  header.substring(header.indexOf("address2=")+9,header.indexOf("&city="));
// //                             String city =  header.substring(header.indexOf("city=")+5,header.indexOf("&inputState="));
// //                             String inputState =  header.substring(header.indexOf("inputState=")+5,header.indexOf("&zip="));
// //                             String zip =  header.substring(header.indexOf("zip=")+4,header.indexOf("&cellPhone="));
// //                             String cellPhone =  header.substring(header.indexOf("cellPhone=")+11,header.indexOf("&homePhone="));
// //                             String homePhone =  header.substring(header.indexOf("homePhone=")+11,header.indexOf("&ssn="));
// //                             String ssn =  header.substring(header.indexOf("ssn=")+4,header.indexOf("&dob="));
// //                             String dob =  header.substring(header.indexOf("dob=")+4,header.indexOf("&email="));
// //                             String email =  header.substring(header.indexOf("email=")+6,header.indexOf(" HTTP"));
// //
// // //                            String tempStr = "INSERT INTO Angler(FirstName,LastName,MiddleInit,Address1,Address2,City,State,Zip,CellPhone,Telephone,SSN,DOB,Email)Values('" +   firstName + "','" + lastName + "','" + middleName + "','" + address1 + "','" + address2 + "','" + city + "','" + inputState + "','" + zip + "','" + cellPhone + "','" + homePhone + "','" + ssn + "','" + dob + "','" + email + "')";
// // //                            char* tempChar = string2char(tempStr);
// // //                            Serial.println(firstName);
// // //                            Serial.println("Testing and stuff");
// // //                            Serial.println(tempStr);
// // //                            sqlite3 *db3;                                                                        //declare a pointer to the data base
// // //                            openDb("/sd/PTS.db", &db3);                                                          //open database on SD card, assign to 'db3'
// // //                            Serial.println("before db_exec");
// // //                          db_exec(db3, "INSERT INTO Angler(FirstName,LastName,MiddleInit) Values ('Mike','Jones','Who')");
// // //                            //db_exec(db3, tempChar);
// // //
// // //                            Serial.println("after db_exec");
// // //                            sqlite3_close(db3);
// //                             //
// // //                            Serial.println("after sqlite3_close");
// //                                                  // Sample code to add to DB
// //                         //  db_exec(db3, "INSERT INTO Angler(FirstName,LastName,MiddleInit,Address1,Address2,City,State,Zip,CellPhone,Telephone,SSN,DOB,DateStamp,ISWFiled,Email)Values('98','John','Smith','B','555 West Street','Apt C','Memphis','TN','54678','5553954678','','321569876','11/13/61','12/18/18','1','John@google.com')");
// //
// //                            //  db_exec(db3, "INSERT INTO Angler(FirstName,LastName,MiddleInit) Values ('Bill','Brown','K')");
// //                         // ATC: This else statement is totally unnecessary and only
// //                         //      serves as a place holder for future expansion
// //                       }
//                         else    //if header did not contain text "line1" then run code in else statment below
//                         {
//                                 // do some stuff
//                         }
// //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// //--RENDER HTML-----------------------------------------------------------------
// //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//                         htmlHead(client);
//                         // svg PTS logo
//                         printPTSLogo(client);
// //--RENDER SETTINGS PAGE--------------------------------------------------------
//                         if (settingsPageFlag) {
//                             pageTitle(client, "Settings");
//
//                             // Set date and time button
//                             startForm(client, "/setDateTime");
//                             button(client, "Set the Date and Time", "info");
//                             endForm(client);
//                             //startForm(client, "[action]")
//                             startForm(client, "/");
//                             //inputBox(client, "[string name of variable]", [actual variable], "[label]", [smalltext? BOOL], "[small text string]")
//                             inputBox(client, "Line1", line1, "Top Line", false, "Example: The Tournament Name");
//                             inputBox(client, "Line2", line2, "Second Line", true, "Example: The Tournament Location");
//                             inputBox(client, "Line3", line3, "Third Line", true, "Example: The Tournament Dates");
//                             inputBox(client, "Line4", line4, "Bottom Line", true, "Example: A sponsor message");
//                             //checkBox(client, "[String name of variable]", [actual variable], "[String label]")
//                             checkBox(client, "checkbox1", checkbox1_status, "Print 2 Copies");
//                             checkBox(client, "checkbox2", checkbox2_status, "Print signature line");
//                             checkBox(client, "checkbox3", checkbox3_status, "Serialized ticket");
//                             checkBox(client, "checkbox4", checkbox4_status, "Optional Parameter");
//                             // Submit Button
//                             button(client, "submit", "primary");
//                             endForm(client);
//
//                             // if the startup flag that determined if an SD card is present then display the update button
//                             if(isSDCardPresent)
//                             {
//                               // update button
//                               startForm(client, "/update");
//                               button(client, "Update Firmware", "success");
//                               endForm(client);
//                             }
//                             // change password button
//                             startForm(client, "/changePassword");
//                             button(client, "Change Password", "info");
//                             endForm(client);
//                         }
// //--SET TIME PAGE---------------------------------------------------------------
//                         else if (setTimePageFlag) {
//                             pageTitle(client, "Set Time and Date");
//                             //startForm(client, "[action]")
//                             startForm(client, "/settings");
//
//                             // Pull the date from the device and send through header
//                             client.println(R"###(
//                             <button style="margin-bottom:5px;" type="submit" value="Use Date/Time from this device" class="btn btn-success btn-lg btn-block" onclick="getElementById('date').value=Date()">Use Date/Time from this device</button>
//                             <input type="hidden" style="visibility: hidden;" class="form-control" name="date" id="date">
//                             )###");
//                             endForm(client);
//
//                             if(allowUserDefinedDate){
//                                 // Allow user to enter date and time then send
//                                 startForm(client, "/settings");
//                                 //inputBox(client, "[string name of variable]", [actual variable], "[label]", [smalltext? BOOL], "[small text string]")
//                                 inputBox(client, "UserDate", "", "Date", false, "", "date");
//                                 inputBox(client, "UserTime", "", "Time", false, "", "time");
//                                 button(client, "Update Date", "primary");
//                                 endForm(client);
//                             }
//
//                             // Cancel button
//                             startForm(client, "/settings");
//                             button(client, "Cancel", "danger");
//                             endForm(client);
//                         }
//
//
// //--RENDER UPDATE PAGE----------------------------------------------------------
//                         else if (updatePageFlag) {
//                             pageTitle(client, "Update Firmware");
//                             //  Add breif instructions
//                             alert(client, "primary", "This will update the firmare on your device.<br>Insert an SD card with a version of the firmware loaded and click \"Check for Update\".", "" , "NOTE: you may need to reconnect to this wifi network after updating.");
//                             // Update now button
//                             if(updateMessage == ""){
//                               startForm(client, "/checkForUpdate");
//                               button(client, "Check for Update", "success");
//                               endForm(client);
//                             }
//                             // Print message to user dynamically
//                             if(updateMessage != ""){
//                                 alert(client, "danger", updateMessage, "ERROR!", "Please make sure you have loaded the update software in the root directory of the SD card." );
//                                 startForm(client, "checkForUpdate");
//                                 button(client, "Retry", "success");
//                                 endForm(client);
//                             }
//                             // Print table of update files
//                             if (arrayOfUpdateFiles[0] != ""){
//                                 printTableOfUpdateFiles(client, arrayOfUpdateFiles);
//                             }
//                             // Cancel button
//                             startForm(client, "/settings");
//                             button(client, "Cancel", "danger");
//                             endForm(client);
//                         }
// //--CHANGE PASSWORD PAGE--------------------------------------------------------
//
//                         else if (changePasswordPageFlag) {
//                             pageTitle(client, "Change Password");
//                             if (passwordMessage != "") {
//                                 if (passSuccess) {
//                                     // succes banner
//                                     alert(client, "success", passwordMessage, "SUCCESS!");
//
//                                 } else {
//                                     // fail banner
//                                     alert(client, "danger", passwordMessage, "TRY AGAIN!");
//                                 }
//                             }
//                             if(passSuccess)
//                             {
//                                 alert(client, "info", "The next time you login to the this WiFi Hotspot you will be required to enter your new password.", "NOTICE!");
//
//                                 // return home
//                                 startForm(client, "/");
//                                 button(client, "Return to Home Page", "success");
//                                 endForm(client);
//                             } else
//                             {
//                                 startForm(client, "/changePassword");
//                                 inputBox(client, "pw", "", "Enter New Password", true, "Must be at least 8 characters and no more than 20", "password");
//                                 inputBox(client, "pw2", "", "Please Reenter Password", true, "Passwords must match", "password");
//                                 button(client, "submit", "primary");
//                                 endForm(client);
//
//                                 startForm(client, "/");
//                                 button(client, "Cancel", "danger");
//                                 endForm(client);
//                             }
//                             passwordMessage = "";
//                             passSuccess = false;
//                         }
// //--RENDER ADD ANGLER----------------------------------------------------------
//                          else if(addAnglerPageFlag) {
//                           pageTitle(client, "Add Angler");
//                           //  db_exec(db3, "INSERT INTO Angler(FirstName,LastName,MiddleInit,Address1,Address2,City,State,Zip,CellPhone,Telephone,SSN,DOB,DateStamp,ISWFiled,Email)Values('98','John','Smith','B','555 West Street','Apt C','Memphis','TN','54678','5553954678','','321569876','11/13/61','12/18/18','1','John@google.com')");
//                           startForm(client, "/addAngler");
//                           inputBox(client, "firstName", "", "First Name", false, "", "text");
//                           inputBox(client, "lastName", "", "Last Name", false, "", "text");
//                           inputBox(client, "middleName", "", "Middle Name or Initial", false, "", "text");
//                           inputBox(client, "address1", "", "Address Line 1", false, "", "text");
//                           inputBox(client, "address2", "", "Address Line 2", false, "", "text");
//                           inputBox(client, "city", "", "City", false, "", "text");
//                           stateBox(client);
//                           inputBox(client, "zip", "", "Zip Code", false, "", "number");
//                           inputBox(client, "cellPhone", "", "Cell Phone", false, "", "tel");
//                           inputBox(client, "homePhone", "", "Home Phone", false, "", "tel");
//                           inputBox(client, "ssn", "", "Social Security Number", false, "", "number");
//                           inputBox(client, "dob", "", "Date of Birth", false, "", "date");
//                           inputBox(client, "email", "", "Email Address", false, "", "email");
//                           button(client, "submit", "primary");
//                           endForm(client);
//                           // Go back to home
//                           startForm(client, "/");
//                           button(client, "Exit", "danger");
//                           endForm(client);
//                         }
// //--RENDER HOME SCREEN----------------------------------------------------------
//                         else
//                         {
//                         pageTitle(client, "HotSpot Printer");
//                         startForm(client, "/print");
//                         // Big print button
//                         printButton(client);
//                         endForm(client);
//                         // Settings Button
//                         startForm(client, "/settings");
//                         button(client, "Settings", "secondary");
//                         endForm(client);
//
//                         // // Add Angler Button
//                         // startForm(client, "/addAngler");
//                         // button(client, "Add Angler", "danger");
//                         // endForm(client);
//                         }
//                         // Version number on bottom of all pages
//                         bottomNav(client, VERSION_NUMBER[0], VERSION_NUMBER[1], VERSION_NUMBER[2]);
// //--END RENDER HTML-------------------------------------------------------------
//                         client.println();       // The HTTP response ends with another blank line
//                         break;                  // Break out of the while loop
//                 } else {                        // if you got a newline, then clear currentLine
//                         currentLine = "";
//                 }
//             }
//                 else if (c != '\r') {               // if you got anything else but a carriage return character,
//                     currentLine += c;               // add it to the end of the currentLine
//                 }
//             } // END if (client.available())
//         } // END while (client.connected())
//
//         header = "";                            // Clear the header variable
//         save_header = "";
//         client.stop();                         // Close the connection
//         Serial.println("Client disconnected.");//send status message to serial debug window
//         Serial.println("");
//
//     }  //end of 'If (Client)'
*/
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

//---------------save bumpsink info and weight to database -------------------------
void save_weighin_to_database(){
       sqlite3 *db3;                                                              //create an instance of the data base
       openDb("/sd/PTS.db", &db3);                                                //open the database
       sprintf(sSQL,"Update WEIGHIN SET TotalFish = %d,LiveFish = %d,ShortFish = %d,Late = %d,TimeStamp = %s,DateStamp = %S WHERE ID = %d",Total_fish,Total_alive,Total_short,Total_late,current_time.c_str(),current_date.c_str(),pnt);             //build the sql command
       db_exec(db3,sSQL);                                                         //execute the command
       sqlite3_close(db3);                                                        //close the database

//  int Total_fish;         //bump sink values for screen entry
//int Total_alive;
//int Total_short;
//int Total_late;
}
//---------------- check availble size and memory left on SD card --------------------------
void check_sd_mem(void){
           Serial2.println(">>setup() - SD card present");
           Serial2.printf(">>Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
           Serial2.printf(">>Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
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
    lcd.setCursor(0,0);
    lcd.print(current_time);
    }

//--------------- get time from rtc -------------------------------------------------
void get_time(void){
    String h = "";                                   //varibles to hold leading zero
    String m = "";
    String s = "";
    DateTime now = rtc.now();
    if (now.hour()<10)                                //add leading zero
      {h= "0";}
    if (now.minute()<10)                               //add leading zero
      {m = "0";}
    if (now.second()<10)                               //add leading zero
      {s = "0";}
    current_time = h+String(now.hour()) + ":" + m+ String(now.minute()) + ":"+ s + String(now.second());
   }

//-------------display date on LCD upper right corner ----------------------------------------------
void lcd_display_date(void){
    lcd.setCursor(10,0);
    lcd.println(current_date);
    }

//---------- get date from rtc --------------------------------------------------------
void get_date(void){
    String m = "";
    String d = "";
    DateTime now = rtc.now();
    if (now.month() <10)                                  //leading zero for months
       {m = "0";}
    if (now.day() <10)                                    //leading zero for days
       {d= "0";}
    current_date = m + String(now.month()) + ":" + d+ String(now.day()) + ":" + String(now.year());
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
               Serial2.write('1');                           //0= left, 1 = center 2= right

               bold_on();


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
               bold_off();
             //  Serial2.print (current_time + "\0x09\0x09\0x09" + current_date);
//------------ print serial number --------------------------------------------------
//               if (checkbox3_status == "checked"){          //is serialized ticket check box checked
//                   Serial2.printf("S/N # %08d",serial_number);  //print ticket sequence number
//                   Serial2.write(0x0A);
//                    if (checkbox3_status == "checked")
//                    lcd.setCursor(0,0);
//                   // lcd.print("Ticket# "+ String(serial_number));
//                    lcd.printf("S/N # %08d       ",serial_number);
//                    Serial.printf("S/N # %08d       ",serial_number);
//                   }

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
               Serial2.print (current_time + "            " + current_date);
//                DateTime now = rtc.now();
//
//                if (now.hour()<10)                               //add leading zero
//                  {Serial2.print("0");}
//                Serial2.print(now.hour(), DEC);
//                Serial2.print(':');
//                if (now.minute()<10)                               //add leading zero
//                  {Serial2.print("0");}
//                Serial2.print(now.minute(), DEC);
//                Serial2.print(':');
//                if (now.second()<10)                               //add leading zero
//                  {Serial2.print("0");}
//                Serial2.print(now.second(), DEC);
//                Serial2.print("       ");
//                Serial2.print(now.month(), DEC);
//                Serial2.print('/');
//                Serial2.print(now.day(), DEC);
//                Serial2.print('/');
//                Serial2.print(now.year(), DEC);


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
//           lcd.clear();
//           lcd.setCursor((20-(line1.length()))/2,0);                //print line 1 and center
//           lcd.print(line1);
//           lcd.setCursor((20-(line2.length()))/2,1);                //print line 1 and center
//           lcd.print(line2);
//           lcd.setCursor((20-(line3.length()))/2,2);                //print line 1 and center
//           lcd.print(line3);
//           lcd.setCursor((20-(String(weight).length()))/2,3);
//
//           lcd.setCursor(0,3);
//             if (now.hour()<10)                                     //add leading zero
//              {lcd.print("0");}
//           lcd.print(now.hour(), DEC);
//           lcd.print(':');
//           if (now.minute()<10)                                     //add leading zero
//              {lcd.print("0");}
//           lcd.print(now.minute(), DEC);
//           lcd.print(':');
//           if (now.second()<10)                                     //add leading zero
//              {lcd.print("0");}
//           lcd.print(now.second(), DEC);
//           lcd.print("  ");
//
//           lcd.setCursor(10,3);
//           if (now.month() <10)                                     //leading zero for months
//               {lcd.print("0");}
//           lcd.print(now.month(), DEC);
//           lcd.print('/');
//           if (now.day() <10)                                       //leading zero for days
//               {lcd.print("0");}
//           lcd.print(now.day(), DEC);
//           lcd.print('/');
//           lcd.print(now.year(), DEC);
//           delay(2000);
 //--------- 2nd lcd screen of weigh ticket info ----------------------------------
//           lcd.clear();
//           lcd.setCursor((20-(line4.length()))/2,0);                //print line 4 and center
//           lcd.print(line4);
//           if (statt == 0)                                          //no signal
//                  {
//                  lcd.setCursor(0,2);
//                  lcd.print("     No Signal");
//                  }
//           else if (statt == 1)                                     //if H2 mode
//              {
//                lcd.setCursor(0,2);                                 //center text on 3rd line
//                lcd.print(output_string);
//              }
//          else if (statt ==2)
//              {
//               lcd.write(output_string[1]);                       //send out string one byte at a time.
//               lcd.write(output_string[2]);                       //print lb value
//               lcd.write(output_string[3]);
//               lcd.printf("Lb");
//               lcd.write(output_string[5]);                       //print oz value
//               lcd.write(output_string[6]);
//               lcd.write(output_string[7]);
//               lcd.write(output_string[8]);
//               lcd.print("oz");                                   //print the oz label with return
//              }
//         else if (statt == 3)
//              {
//               lcd.write(output_string[0]);                       //send weight
//               lcd.write(output_string[1]);
//               lcd.write(output_string[2]);
//               lcd.write(output_string[3]);                       //decimal point
//               lcd.write(output_string[4]);
//               lcd.write(output_string[5]);
//               lcd.print("Lbs");
//              }
//
//             delay(2000);
//             // ticket = ticket + 1;                              //pointer for weigh tickets
//          clear_output_buffer();                                  //clear the output string
//
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
//void set_text_size(unsigned int size)                           //set font size on printer
//      {
//      Serial2.write(0x1D);                                      // set text size to small size
//      Serial2.write(0x21);
//      Serial2.write(size);                                      // sizes - 00= 1,11 = 2x,22 = 3x,33 = 4x ,44= 5x
//      }


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
  Serial.println(radio_rx_array);
  no_signal_timer = 0;                                //reset the no signal timer since a reading was sensed
  lock_flag = false;                                  //preset lock flag to false
  while(i >= 3)                                       //search from  the 7 to the 15th character in array
      { if (radio_rx_array[i] == 'H')                 //check for locked value in string
           {//Serial.println("**Locked**");             //send locked value to serial monitor
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
   Serial.println("Unable to process radio rx string");
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
void addToAnglerDB(char* sSQL) {
    sqlite3 *db3;                       //declare a pointer to the data base
    openDb("/sd/PTS.db", &db3);         //open database on SD card, assign to 'db3'
    db_exec(db3,sSQL);                  // Execute the insertion
    sqlite3_close(db3);                 // Close the database
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("Hello Client %u :)", client->id());
    client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);

      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < info->len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for(size_t i=0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      Serial.printf("%s\n",msg.c_str());

      if(info->opcode == WS_TEXT)
        client->text("I got your text message");
      else
        client->binary("I got your binary message");
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if(info->index == 0){
        if(info->num == 0)
          Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
        Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);

      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < info->len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for(size_t i=0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      Serial.printf("%s\n",msg.c_str());

      if((info->index + len) == info->len){
        Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if(info->final){
          Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          if(info->message_opcode == WS_TEXT)
            client->text("I got your text message");
          else
            client->binary("I got your binary message");
        }
      }
    }
  }
}
