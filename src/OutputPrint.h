//routines to print data results to printer

//64 char wide in smallest font size



#ifndef __OutputPrint_H__
#define __OutputPrint_H__

#include <RTClib.h>             //library for  RTC routines

//define functions
void print_results(void);
void paper_cut(void);
void set_text_size(unsigned int size);  // oled set text size routine
void bold_on(void);
void bold_off(void);
void set_text_size(unsigned int size);


extern String current_time;
extern String current_date;
extern String results[75][9];          //array the holds sql data
extern String line1 = "";              // String to hold value of Line 1 input box
extern String line2 = "";              // String to hold value of Line 2 input box
extern String line3 = "";              // String to hold value of Line 3 input box
extern String line4 = "";              // String to hold value of Line 4 input box
extern String tim;




void print_results(){

    Serial2.write(0x1B);                          //A Font
    Serial2.write('M');
    Serial2.write('0x01');                        //(must be font 1 for pica print)

    Serial2.write(0x1B);                         //upside down printing turn off
    Serial2.write('{');
    Serial2.write('0');

    set_text_size(0X00);


   int r = 0;
   Serial2.print("--------------------------");
   // Serial2.print(rec);
   Serial2.println(" records----------------------");
   // while ( r <= rec-1){                                                //Print all records found

   //      Serial2.print( results[r][0]+"  \t");                         // print the column names from array
   //      Serial2.println( results[r][1]);
   //      r++;
   //      }
   Serial2.println("");
   Serial2.write(0x1B);                         //justification: center text
   Serial2.write('a');
   Serial2.write('1');                           //0= left, 1 = center 2= right
   Serial2.println(current_time + "-----  Report by Pro Tournament Scales -------"+ current_date);
   Serial2.write(0x1B);                         //justification: center text
   Serial2.write('a');
   Serial2.write('0');                           //0= left, 1 = center 2= right
   paper_cut();
   Serial2.write(0x1B);                         //upside down printing on
   Serial2.write('{');
   Serial2.write('1');
   }






//============================ Print Weigh in Results =======================================
void print_weigh_results(){
    int r = 0;
    Serial2.write(0x1B);                          //B Font
    Serial2.write('M');
    Serial2.write('0x01');                       //(must be font 01 for pica print)
    Serial2.write(0x1B);                         //upside down printing turn off
    Serial2.write('{');
    Serial2.write('0');
    set_text_size(0X11);                         // set text size to 2x size
    Serial2.write(0x1B);                         //justification: center text
    Serial2.write('a');
    Serial2.write('1');                           //0= left, 1 = center 2= right
    Serial2.println("Weigh in results");
    Serial2.println("");                         //line feed
    set_text_size(0X00);                         // set text size to small size
    Serial2.println(line1);
    Serial2.println(line2);
    Serial2.println(line3);
    Serial2.println("");
    Serial2.write(0x1B);                         //justification: left
    Serial2.write('a');
    Serial2.write('0');                           //0= left, 1 = center 2= right


    Serial2.print("--------------------------");
   //  Serial2.print(rec);
    Serial2.println(" records----------------------------");
    bold_on();
    Serial2.println("  ID  Name           T   A   S   L    Act      Adj   Place"); //print column titles
    Serial2.println("");
    r=0;
    bold_off();

   //  while (r <= rec-1){                                                       //Print all records found in query
   //      if(results[r][0].length() == 1)                                      //insert padding
   //         {Serial2.print("   ");}
   //      else if (results[r][0].length() == 2)
   //         {Serial2.print("  ");}
   //      else if (results[r][0].length() == 3)
   //         {Serial2.print(" ");}
   //      Serial2.print( results[r][0]+"  ");                                //id numberdisplay all the column values (add tab)
   //      Serial2.print( results[r][2]+" ");                                    //last name
   //      Serial2.print( results[r][1]+" \x09");                                //first name
   //      Serial2.print( results[r][3]+"   ");                                  //total fish
   //      Serial2.print( results[r][4]+"   ");                                  //live fish
   //      Serial2.print( results[r][5]+"   ");                                  //short fish
   //      Serial2.print( results[r][6]+"   ");                                  //late
   //      if(results[r][7].length() == 2)                                       //add padding if needed
   //         {Serial2.print("  ");}
   //      if(results[r][7].length() == 3)                                      //insert padding
   //         {Serial2.print(" ");}
   //      String tmpString;
   //      tmpString = results[r][7];
   //      float var = tmpString.toFloat();
   //      var = var/100;
   //      Serial2.print( var);                                                 //actual weight in decimal mode
   //      Serial2.print("   ");
   //      if(results[r][8].length() == 2)
   //         {Serial2.print("  ");}
   //      if(results[r][8].length() == 3)                                      //insert padding
   //         {Serial2.print(" ");}
   //      tmpString = results[r][8];
   //      var = tmpString.toFloat();
   //      var = var/100;                                                       //divide by 100 to add decimal point
   //      Serial2.print( var);                                                //print actual wieght with decimal
   //      Serial2.print("  ");
   //     Serial2.println(r+1);
   //      r++;                                                                  //advance to next record
   //  }
   Serial2.println("");                                                       //line feed
   Serial2.println(current_time + "-----  Report by Pro Tournament Scales -------"+ current_date);
   paper_cut();                                                               //cut paper

   Serial2.write(0x1B);                                                       //upside down printing on
   Serial2.write('{');
   Serial2.write('1');
   }


//--------------------------------------------------------------------------------------------
void bold_on (void){
    Serial2.write(0x1B);                         //emphasized on
    Serial2.write('E');
    Serial2.write('0x01');
}
//--------------------------------------------------------------
void bold_off (void){
    Serial2.write(0x1B);                         //emphasized off
    Serial2.write('E');
    Serial2.write('0x00');
}


///----------------------------------------------------------
 void paper_cut(void)
     {Serial2.write(0x1D);                                        // "GS" cut paper
     Serial2.write('V');                                          //"V"
     Serial2.write(0x42);                                         //decimal 66
     Serial2.write(0xB0);                                         //length to feed before cut (mm)
     }
//-----------------------------------------------------------
void set_text_size(unsigned int size)                           //set font size on printer
      {
      Serial2.write(0x1D);                                      // set text size to small size
      Serial2.write(0x21);
      Serial2.write(size);                                      // sizes - 00= 1,11 = 2x,22 = 3x,33 = 4x ,44= 5x
      }


#endif
