//routines to print data results to printer





#ifndef __OutputPrint_H__
#define __OutputPrint_H__



//define functions
void print_results(void);
void paper_cut(void);
extern String results[250][5];          //array the holds sql data





void print_results(){
    Serial2.write(0x1B);                         //upside down printing turn off
    Serial2.write('{');
    Serial2.write('0');
    
    Serial2.write(0x1D);                         // set text size to small size
    Serial2.write(0x21);
    Serial2.write(00);                           // sizes - 00= 1,11 = 2x,22 = 3x,33 = 4x ,44= 5x
    
    Serial2.write(0x1B);                         //turn off all text enhancements like bolding and italics
    Serial2.write(0x21);
    Serial2.write(0x00);
   int r = 0;
   Serial2.print("----------------");
   Serial2.print(rec);
   Serial2.println(" records----------------------");
   while ( r <= rec-1){                                                //Print all records found
        
        Serial2.print( results[r][0]+"  \t");                         // print the column names from array
        Serial2.println( results[r][1]);
        r++;
        }
   Serial2.println("");
   Serial2.println("-------------- End of results ------------------");     
   paper_cut();
   Serial2.write(0x1B);                         //upside down printing on
   Serial2.write('{');
   Serial2.write('1');
   }
        
  

 void paper_cut(void)
     {Serial2.write(0x1D);                                        // "GS" cut paper
     Serial2.write('V');                                          //"V"
     Serial2.write(0x42);                                         //decimal 66
     Serial2.write(0xB0);                                         //length to feed before cut (mm)
     }

#endif        
