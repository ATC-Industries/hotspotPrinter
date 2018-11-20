/*
    This example opens Sqlite3 databases from SD Card and
    retrieves data from them.
    Before running please copy following files to SD Card:
    data/mdr512.db
    data/census2000names.db
*/
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>
//#include "SD_MMC.h"
bool isSDCardPresent = false;   // Flag checked on startup true if SD card is found
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




/**
 * print out list of files and directoris in specified directory
 * @param fs      SD
 * @param dirname path to directory you want to list
 * @param levels  number of levels deep you want to list
 */
void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void searchForUpdate(fs::FS &fs, const char * dirname, String arrayOfUpdateFiles[20]){
    Serial.printf("Looking for Update Files in: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    String fileName;
    int counter = 0;
    while(file){
        if(!file.isDirectory())
        {
            fileName = file.name();
            if((fileName.indexOf("update") >= 0) && (fileName.indexOf(".bin") >= 0))
            {
                Serial.println(file.name());
                arrayOfUpdateFiles[counter] = fileName;
                counter++;
            }
        }

        file = root.openNextFile();
    }
}

/**
 * Creates a Directory in the specified path on the open SD card
 * @param fs   SD
 * @param path Path you want to create directory in should include name of directory to create
 */
void createDir(fs::FS &fs, const char * path){
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

/**
 * Deletes a directory
 * @param fs   SD
 * @param path A Path to the dirctory you want to delete
 */
void removeDir(fs::FS &fs, const char * path){
    Serial.printf("Removing Dir: %s\n", path);
    if(fs.rmdir(path)){
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}

/**
 * Open file and read it's content to serial
 * @param fs   SD
 * @param path path to file you want to read
 */
void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

/**
 * Open and a file and write char* to it.  This will destroy any previous data in doc
 * @param fs      SD
 * @param path    path to file
 * @param message char* string of what you want to write to file
 */
void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

/**
 * append to the opened file like write but adds char* to end of file.
 * @param fs      SD
 * @param path    path to file
 * @param message char* string of what you want to append to file
 */
void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

/**
 * rename a file.  this would be used to move a file as well
 * @param fs    SD
 * @param path1 path to oridinal file
 * @param path2 new path
 */
void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}

/**
 * delete a file
 * @param fs   SD
 * @param path path to file
 */
void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

/**
 * test that you can open a file and print info to serial
 * @param fs   SD
 * @param path path to file
 */
void testFileIO(fs::FS &fs, const char * path){
    File file = fs.open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;
    if(file){
        len = file.size();
        size_t flen = len;
        start = millis();
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }
        end = millis() - start;
        Serial.printf("%u bytes read for %u ms\n", flen, end);
        file.close();
    } else {
        Serial.println("Failed to open file for reading");
    }


    file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }

    size_t i;
    start = millis();
    for(i=0; i<2048; i++){
        file.write(buf, 512);
    }
    end = millis() - start;
    Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
    file.close();
}

/**
 * run a test of all SD card funtions
 */
void runSDTest(){

    if(!SD.begin()){
        Serial.println("Card Mount Failed");
        return;
    }
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);

    listDir(SD, "/", 0);
    createDir(SD, "/mydir");
    listDir(SD, "/", 0);
    removeDir(SD, "/mydir");
    listDir(SD, "/", 2);
    writeFile(SD, "/hello.txt", "Hello ");
    appendFile(SD, "/hello.txt", "World!\n");
    readFile(SD, "/hello.txt");
    deleteFile(SD, "/foo.txt");
    renameFile(SD, "/hello.txt", "/foo.txt");
    readFile(SD, "/foo.txt");
    testFileIO(SD, "/test.txt");
    Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
    Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
}


//--------------------------------------------------------------------------------------------------------------

void setup() {

 Serial.begin(115200);
   sqlite3 *db1;                                                    //declare varibles as sqlite3 databases
   sqlite3 *db2;
   char *zErrMsg = 0;
   int rc;

   SPI.begin();
   

 if(!SD.begin()){                                                     //start sd card routine
        Serial.println("Card Mount Failed");
     }
  else  
     { Serial.println("Card MOunted");}

//------------------------------------//-----------------------------------

//runSDTest();

listDir(SD, "/", 2);                                    //list directory 2 levels deep


   sqlite3_initialize();                                                               //start database program

   // Open database 1
   if (openDb("/sd/sdcard/census2000names.db", &db1))
   {
       Serial.println("test");
       return;
   }
   if (openDb("/sd/sdcard/mdr512.db", &db2))
      return;

   rc = db_exec(db1, "Select * from surnames where name = 'MICHELLE'");
   if (rc != SQLITE_OK) {
       sqlite3_close(db1);
       sqlite3_close(db2);
       return;
   }
//   rc = db_exec(db2, "Select * from domain_rank where domain between 'google.com' and 'google.com.z'");
//   if (rc != SQLITE_OK) {
//       sqlite3_close(db1);
//       sqlite3_close(db2);
//       return;
//   }
   rc = db_exec(db1, "Select * from surnames where name = 'CLARKSON'");
   if (rc != SQLITE_OK) {
       sqlite3_close(db1);
       sqlite3_close(db2);
       return;
   }
   rc = db_exec(db2, "Select * from domain_rank where domain = 'agri-tronix.com'");
   if (rc != SQLITE_OK) {
       sqlite3_close(db1);
       sqlite3_close(db2);
       return;
   }
     rc = db_exec(db1, "Select name from sqlite_master WHERE type = 'table'");
   if (rc != SQLITE_OK) {
       sqlite3_close(db1);
       sqlite3_close(db2);
       return;
   }

//   rc = db_exec(db1, "CREATE TABLE test_table (name text)");
//   if (rc != SQLITE_OK) {
//       sqlite3_close(db1);
//       sqlite3_close(db2);
//       return;
//   }

        rc = db_exec(db2, "Select name from sqlite_master WHERE type = 'table'");  //list tables
   if (rc != SQLITE_OK) {
       sqlite3_close(db1);
       sqlite3_close(db2);
       return;
   }
//--------------------------
 

   sqlite3_close(db1);
   sqlite3_close(db2);

}

void loop() {
}
