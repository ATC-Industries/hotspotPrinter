// example of parsing a string for a substring a : location and triming spaces
// string dayUsers = inputLines[2].Substring(inputLines[2].IndexOf(':') + 1).Trim();



#ifndef __SQLITE_H__
#define __SQLITE_H__

#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <ArduinoJson.h>        // https://arduinojson.org/v5/example/generator/

const int COL = 9;
extern String results[75][COL];      //array the holds sql data
extern int rec;                     //must be declared here and in main program so values xfer
const char* data = "SQL reply";     //text to be printed when sql commands are processed

 DynamicJsonBuffer jsonBuffer;
// Build a JSON of query
 JsonObject& root = jsonBuffer.createObject();

/* This callback routine is where the return data from the query is processed
    argc = number of columns in query (integer value)
    argv = the value that is in the colum (string)
    azColName = the name of the column (string)
*/
static int callback(void *data, int argc, char **argv, char **azColName) {  //function to display column name and value
  int i;
 // rec = 0;                                                                 //reset pointer
 // Serial.printf("%s:\n", (const char*)data);                            //print 'SQL reply' to serial monitor
  if (rec == 0)
      {Serial.printf("%s\t%s\n\r",azColName[0],azColName[1]);}          //only print the column names once
  for (i = 0; i < argc; i++) {                                             //display all the columns selected in query
      Serial.printf("%s\t", argv[i] ? argv[i] : "NULL");                   //print the field values
      //Serial.printf("%s = %s\t", azColName[i], argv[i] ? argv[i] : "NULL");  //print column name and value then tab
      results[rec][i] = argv[i];
  }
//**** Adam I remmed this out while I was working  on my code,this echos a lot of extra text to my serial monitor that I didnt want to wade through
            JsonArray& jsonRecord = root.createNestedArray(String(rec));
            for (int i = 0; i < COL; i++) {
                jsonRecord.add(results[rec][i]);
              }
  //root[String(rec)] = results[rec][0];
  Serial.printf("\n");                                                     //print a space between records
  rec++;                                                                   //increment record counter

  return 0;
}

int openDb(const char *filename, sqlite3 **db) {
  int rc = sqlite3_open(filename, db);
  if (rc) {
    Serial.printf("OpenDb() - Can't open database: %s\n", sqlite3_errmsg(*db));
    return rc;
  } else {
    Serial.printf("OpenDb() - Opened database successfully\n");
  }
  return rc;
}

char *zErrMsg = 0;
int db_exec(sqlite3 *db, const char *sql) {                                //this routine is where SQL commands are executed
  rec=0;
  Serial.println(sql);
  long start = micros();                                                //record current count to calculate process time
  int rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);       //callback function prints the data to query
  if (rc != SQLITE_OK) {                                                 //if error
    Serial.printf("SQL error: %s\n", zErrMsg);                         //print the error message on error
    sqlite3_free(zErrMsg);
  } else {
      // Print out the JSON
   // root.prettyPrintTo(Serial);
    Serial.printf("\n");
    Serial.printf("Operation done successfully\n");
  }
  Serial.printf("%d records found\n", rec);

  Serial.print(F("Time taken:"));                                        //print in usec, time to perform Database task
  Serial.println(micros() - start);                                      //show how long it took to process this command
  return rc;
}


#endif
//create_member_table{
//int rc = db_exec(db1,"CREATE TABLE IF NOT EXIST Membership(`Weighin ID`  INTEGER,`Angler ID` INTEGER,`Boat ID` INTEGER,'Tournament ID` INTEGER,
//  `Date`  TEXT,
//  `Weight`  INTEGER,
//  `Number Fish` INTEGER,
//  `Short Fish`  INTEGER,
//  `Live Fish` INTEGER,
//  `Minutes Late`  INTEGER,
//  `IsRegistered?` INTEGER,
//  `HasBumped?`  INTEGER,
//  `Has Weighed` INTEGER)")
//;}

//rc = db_exec(db1, "CREATE TABLE test_table (name text)");
// code to go in loop\
//-----------------------

// Serial.begin(115200);
//   sqlite3 *db1;                                                    //declare varibles as sqlite3 databases
//   sqlite3 *db2;
//   char *zErrMsg = 0;
//   int rc;
//
//   SPI.begin();
//
// // Prob don't need this stuff for the SD card.
// if(!SD.begin()){                                                     //start sd card routine
//        Serial.println("Card Mount Failed");
//     }
//  else
//     { Serial.println("Card MOunted");}


// sqlite3_initialize();                                                               //start database program
//
//  // Open database 1
//  if (openDb("/sd/sdcard/census2000names.db", &db1))
//  {
//      Serial.println("test");
//      return;
//  }
//  if (openDb("/sd/sdcard/mdr512.db", &db2))
//     return;
//
//  rc = db_exec(db1, "Select * from surnames where name = 'MICHELLE'");
//  if (rc != SQLITE_OK) {
//      sqlite3_close(db1);
//      sqlite3_close(db2);
//      return;
//  }
// //   rc = db_exec(db2, "Select * from domain_rank where domain between 'google.com' and 'google.com.z'");
// //   if (rc != SQLITE_OK) {
// //       sqlite3_close(db1);
// //       sqlite3_close(db2);
// //       return;
// //   }
//  rc = db_exec(db1, "Select * from surnames where name = 'CLARKSON'");
//  if (rc != SQLITE_OK) {
//      sqlite3_close(db1);
//      sqlite3_close(db2);
//      return;
//  }
//  rc = db_exec(db2, "Select * from domain_rank where domain = 'agri-tronix.com'");
//  if (rc != SQLITE_OK) {
//      sqlite3_close(db1);
//      sqlite3_close(db2);
//      return;
//  }
//    rc = db_exec(db1, "Select name from sqlite_master WHERE type = 'table'");
//  if (rc != SQLITE_OK) {
//      sqlite3_close(db1);
//      sqlite3_close(db2);
//      return;
//  }
//
//   rc = db_exec(db1, "CREATE TABLE test_table (name text)");
//    if (rc != SQLITE_OK) {
//       sqlite3_close(db1);
//       sqlite3_close(db2);
//        return;
//   }
//
//       rc = db_exec(db2, "Select name from sqlite_master WHERE type = 'table'");  //list tables
//  if (rc != SQLITE_OK) {
//      sqlite3_close(db1);
//      sqlite3_close(db2);
//      return;
//  }
// //--------------------------
//
//
//  sqlite3_close(db1);
//  sqlite3_close(db2);
