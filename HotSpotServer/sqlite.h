#ifndef __SQLITE_H__
#define __SQLITE_H__

#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>            // sqlite3 data base Files

const char* data = "Callback function called";
static int callback(void *data, int argc, char **argv, char **azColName){
   int i;
   Serial.printf("%s: ", (const char*)data);
   for (i = 0; i<argc; i++){
       Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   Serial.printf("\n");
   return 0;
}

int openDb(const char *filename, sqlite3 **db) {
   int rc = sqlite3_open(filename, db);
   if (rc) {
       Serial.printf("Can't open database: %s\n", sqlite3_errmsg(*db));
       return rc;
   } else {
       Serial.printf("Opened database successfully\n,");
   }
   return rc;
}

char *zErrMsg = 0;
int db_exec(sqlite3 *db, const char *sql) {
   Serial.println(sql);
   long start = micros();
   int rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
   if (rc != SQLITE_OK) {
       Serial.printf("SQL error: %s\n", zErrMsg);
       sqlite3_free(zErrMsg);
   } else {
       Serial.printf("Operation done successfully\n");
   }
   Serial.print(F("Time taken:"));
   Serial.println(micros()-start);
   return rc;
}

#endif



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
// //   rc = db_exec(db1, "CREATE TABLE test_table (name text)");
// //   if (rc != SQLITE_OK) {
// //       sqlite3_close(db1);
// //       sqlite3_close(db2);
// //       return;
// //   }
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
