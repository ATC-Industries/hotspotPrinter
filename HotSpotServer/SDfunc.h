#ifndef __SDFUNC_H__
#define __SDFUNC_H__

String arrayOfUpdateFiles[20] = {};
extern bool diagnostic_flag;
void rebootEspWithReason(String reason);

/**
 * print out list of files and directoris in specified directory
 * @param fs      SD
 * @param dirname path to directory you want to list
 * @param levels  number of levels deep you want to list
 */
void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);
    if (diagnostic_flag)                                         //^^^if  diagnostic mode
      {
       Serial2.printf(">>Listing directory: %s\n", dirname);       //print directories to printer 
      }
    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        if (diagnostic_flag)                                         //^^^if  diagnostic mode
            {Serial2.printf(">>ListDir() - Failed to open directory");}       //print directories to printer 
      
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        if (diagnostic_flag)                                         //^^^if  diagnostic mode
            {Serial2.printf(">>listDir() - Not a directory");}
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
            if (diagnostic_flag){                                         //^^^if  diagnostic mode
                Serial2.print("  FILE: ");
                Serial2.print(file.name());
                Serial2.print("  SIZE: ");
                Serial2.println(file.size());
                }
        }
        file = root.openNextFile();
    }
}

void searchForUpdate(fs::FS &fs, const char * dirname, String arrayOfUpdateFiles[20]){
    Serial.printf("Looking for Update Files in: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        if (diagnostic_flag)                                         //^^^if  diagnostic mode
            {Serial2.printf(">>SearchForUpdate(): Failed to open directory");}
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        if (diagnostic_flag)                                         //^^^if  diagnostic mode
           {Serial2.printf(">>searchForUpdate() - Not a directory");}
        
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
                if (diagnostic_flag)                                         //^^^if  diagnostic mode
                  { Serial2.print(">>>");
                    Serial2.println(file.name());}
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
        if (diagnostic_flag)                                         //^^^if  diagnostic mode
                  {Serial2.printf("Dir created");}
    } else {
        Serial.println("mkdir failed");
        if (diagnostic_flag)                                         //^^^if  diagnostic mode
                  {Serial2.printf("mkdir failed");}
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
         if (diagnostic_flag)                                         //^^^if  diagnostic mode
                  {Serial2.println("removeDir() -Dir removed");}
    } else {
        Serial.println("rmdir failed");
        if (diagnostic_flag)                                         //^^^if  diagnostic mode
                  {Serial2.println("removeDir() - rmdir failed");}
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
        if (diagnostic_flag)                                         //^^^if  diagnostic mode
                  {Serial2.println("readFile()- Failed to open file for reading");}
        return;
    }

    Serial.print("Read from file: ");
     if (diagnostic_flag)                                         //^^^if  diagnostic mode
                  {Serial2.print("Read from file: ");}
    while(file.available()){
        Serial.write(file.read());
        if (diagnostic_flag)                                         //^^^if  diagnostic mode
                  {Serial2.write(file.read());}
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

    if (diagnostic_flag)                                         //^^^if  diagnostic mode
                  {Serial2.printf("writeFile() - Writing file: %s\n", path);}

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        if (diagnostic_flag)                                         //^^^if  diagnostic mode
                  {Serial2.println("writeFile() - Failed to open file for writing");}
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
        if (diagnostic_flag)                                         //^^^if  diagnostic mode
                  {Serial2.println("writeFile() - File written");}
    } else {
        Serial.println("Write failed");
        if (diagnostic_flag)                                         //^^^if  diagnostic mode
                  {Serial2.println("writeFile() - Write failed");}
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
        if (diagnostic_flag)                                         //^^^if  diagnostic mode
                  {Serial2.println("appendFile() - Failed to open file for appending");}
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
        if (diagnostic_flag)                                         //^^^if  diagnostic mode
                  {Serial2.println("appendFile() - Message appended");
    } else {
        Serial.println("Append failed");
        if (diagnostic_flag)                                         //^^^if  diagnostic mode
                  {Serial2.println("appendFile() - Append failed");}
         }
    file.close();
    }
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
        if (diagnostic_flag)                                         //^^^if  diagnostic mode
                  {Serial2.println("renameFile() - File renamed");}
    } else {
        Serial.println("Rename failed");
        if (diagnostic_flag)                                         //^^^if  diagnostic mode
                  {Serial2.println("renameFile() - Rename failed");}
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
        if (diagnostic_flag)                                         //^^^if  diagnostic mode
                  {Serial2.println("deleteFile() - File deleted");}
    } else {
        Serial.println("Delete failed");
        if (diagnostic_flag)                                         //^^^if  diagnostic mode
                  {Serial2.println("deleteFile() - Delete failed");}
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

//------------------------- UPDATE FIRMWARE FROM SD CARD -----------------------
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

 /**
  * perform the actual update from a given stream
  * @param updateSource file Stream
  * @param updateSize   size of the update
  */
void performUpdate(Stream &updateSource, size_t updateSize) {
        if (Update.begin(updateSize)) {
                size_t written = Update.writeStream(updateSource);
                if (written == updateSize) {
                        Serial.println("Written : " + String(written) + " successfully");
                        if (diagnostic_flag){
                                Serial2.println(">>performUpdate() - Written : " + String(written) + " successfully");
                                }
                }
                else {
                        Serial.println("Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
                        if (diagnostic_flag){
                               Serial2.println(">>performUpdate() -Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
                               }
                }
                if (Update.end()) {
                        Serial.println("OTA done!");
                        if (Update.isFinished()) {
                                Serial.println("Update successfully completed. Rebooting.");
                                if (diagnostic_flag){
                                    Serial2.println(">>performUpdate() - Update successfully completed. Rebooting.");}
                        }
                        else {
                                Serial.println("Update not finished? Something went wrong!");
                                 if (diagnostic_flag){
                                    Serial2.println(">>performUpdate() - Update not finished? Something went wrong!");}
                        }
                }
                else {
                        Serial.println("Error Occurred. Error #: " + String(Update.getError()));
                             if (diagnostic_flag){
                                    Serial2.println(">>performUpdate() - Error Occurred. Error #: " + String(Update.getError()));}
                }

        }
        else
        {
                Serial.println("Not enough space to begin OTA");
                if (diagnostic_flag){
                         Serial2.println(">>performUpdate() - Not enough space to begin OTA");}
        }
}

// check given FS for valid update.bin and perform update if available
void updateFromFS(fs::FS &fs, String updateFileName, String& updateMessage) {
        File updateBin = fs.open(updateFileName);
        if (updateBin) {
                if(updateBin.isDirectory()) {
                        Serial.println("Error, update.bin is not a file");
                        if (diagnostic_flag){
                             Serial2.println(">>updateBin() - Error, update.bin is not a file");}
                        updateMessage = "Error, update.bin is not a file";
                        updateBin.close();
                        return;
                }

                size_t updateSize = updateBin.size();

                if (updateSize > 0) {
                        Serial.println("Try to start update");
                        if (diagnostic_flag){
                             Serial2.println(">>updateBin() - Try to start update");}
                        performUpdate(updateBin, updateSize);
                }
                else {
                        Serial.println("Error, file is empty");
                        if (diagnostic_flag){
                             Serial2.println(">>updateBin() - Error, file is empty");}
                        updateMessage = "Error, file is empty";
                }

                updateBin.close();

                // whe finished remove the binary from sd card to indicate end of the process
                //fs.remove("/update.bin");
                ESP.restart();
        }
        else {
                Serial.println("Could not load update.bin from sd root");
                if (diagnostic_flag){
                             Serial2.println(">>updateBin() - Could not load update.bin from sd root");}
                updateMessage += "Could not load update.bin from sd root";
        }
}

/**
 * Run the Update.  you can attatch this to a button or something to initiate and update.
 */
void updateFirmware(String& updateMessage, String updateFile) {
        uint8_t cardType;
        Serial.println("Updating from SD Card!");

        // You can uncomment this and build again
        //Serial.println("Update successfull");

        //first init and check SD card
        if (!SD.begin()) {
                //rebootEspWithReason("Card Mount Failed");
                updateMessage = "Card Mount Failed";
        }

        cardType = SD.cardType();

        if (cardType == CARD_NONE) {
                //rebootEspWithReason("No SD card attached");
                updateMessage = "No SD card attached";
        }else{
                searchForUpdate(SD, "/", arrayOfUpdateFiles);
                updateFromFS(SD, updateFile, updateMessage);
        }
}

/**
 * Open card and check for update files.
 */
void checkForUpdateFirmware(String& updateMessage) {
        uint8_t cardType;
        Serial.println("checkForUpdateFirmware() - Searching for available updates");
        if (diagnostic_flag)
                   {
                    Serial2.println(">>checkForUpdateFirmware() - Searching for available updates");
                   }
        
        // You can uncomment this and build again
        //Serial.println("Update successfull");

        //first init and check SD card
        if (!SD.begin()) {
                //rebootEspWithReason("Card Mount Failed");
                updateMessage = "Card Mount Failed";
                if (diagnostic_flag)
                   {
                    Serial2.println(">>checkForUpdateFirmware() - Card Mount Failed");}
        }

        cardType = SD.cardType();

        if (cardType == CARD_NONE) {
                //rebootEspWithReason("No SD card attached");
                updateMessage = "No SD card attached";
        }else{

                searchForUpdate(SD, "/", arrayOfUpdateFiles);
        }
}

/**
 * Check if SD card is present
 */
bool isSDCard() {
        uint8_t cardType;
        Serial.println("Checking if SD card is installed");
        
         if (diagnostic_flag)
           {
            Serial2.println(">>isSDCard() - Checking if SD card is installed");
            if (diagnostic_flag)
                   {
                    Serial2.println(">>isSDCard() - Checking if SD card is installed");}
           }
        // You can uncomment this and build again
        //Serial.println("Update successfull");

        //first init and check SD card
        if (!SD.begin()) {
                //rebootEspWithReason("Card Mount Failed");
                Serial.println("Card Mount Failed");
                if (diagnostic_flag)
                   {
                    Serial2.println(">>isSDCard() - Card Mount Failed");
                   }
                return false;
        }

        cardType = SD.cardType();

        if (cardType == CARD_NONE) {
                //rebootEspWithReason("No SD card attached");
                Serial.println("No SD card attached");
                if (diagnostic_flag)
                   {
                    Serial2.println(">>isSDCard() - No SD card attached");
                   }
                return false;
        }else{
            Serial.println("SD Card found");
            if (diagnostic_flag)
                   {
                    Serial2.println(">>isSDCard() - SD Card found");
                   }
            return true;
        }
}

/**
 * Reboot the ESP processer and give a reason
 * @param reason Reason String
 */
void rebootEspWithReason(String reason){
        Serial.println(reason);
        if (diagnostic_flag)
                   {
                    Serial2.print(">>rebootEspWithReason() - ");
                    Serial2.println(reason);
                   }
        delay(1000);
        ESP.restart();
}

/**
 * Check for update files and print the result in an HTML table
 * @param client
 * @param arrayOfUpdateFiles
 */
void printTableOfUpdateFiles(WiFiClient& client, String arrayOfUpdateFiles[20]){
    // create a second array to hold the version numbers
    String arrayOfUpdateFilesVersionNums[20];
    for(int i = 0; i < 20; i++){
        arrayOfUpdateFilesVersionNums[i] = arrayOfUpdateFiles[i].substring(arrayOfUpdateFiles[i].indexOf("/update")+7,arrayOfUpdateFiles[i].indexOf(".bin"));
        arrayOfUpdateFilesVersionNums[i].replace("_", ".");
    }

    client.println("<table class=\"table\">");
    client.println(" <thead>");
    client.println("  <tr>");
    client.println("   <th scope=\"col\">Filename</th>");
    client.println("   <th scope=\"col\">Version</th>");
    client.println("   <th scope=\"col\">Update</th>");
    client.println("  </tr>");
    client.println(" </thead>");
    client.println("<tbody>");
    for(int i = 0; (i < 20 && arrayOfUpdateFiles[i] != ""); i++){
        client.println("<tr>");
          client.println("<td>" + arrayOfUpdateFiles[i] + "</th>");
          client.println("<td>" + arrayOfUpdateFilesVersionNums[i] + "</td>");
          client.println("<td><form action=\"/doUpdate" + String(i) + "\" method=\"GET\"><input type=\"submit\" value=\"Update\" class=\"btn btn-success\"></form></td>");
        client.println("</tr>");
    }
    client.println("</tbody>");
    client.println("</table>");
}

#endif
