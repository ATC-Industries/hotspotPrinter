void rebootEspWithReason(String reason);
void createDir(fs::FS &fs, const char * path);
void removeDir(fs::FS &fs, const char * path);
void readFile(fs::FS &fs, const char * path);
void writeFile(fs::FS &fs, const char * path, const char * message);
void appendFile(fs::FS &fs, const char * path, const char * message);
void renameFile(fs::FS &fs, const char * path1, const char * path2);
void deleteFile(fs::FS &fs, const char * path);
void testFileIO(fs::FS &fs, const char * path);
void performUpdate(Stream &updateSource, size_t updateSize);
void updateFirmware();



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

void createDir(fs::FS &fs, const char * path){
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void removeDir(fs::FS &fs, const char * path){
    Serial.printf("Removing Dir: %s\n", path);
    if(fs.rmdir(path)){
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}

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

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

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
void updateFromFS(fs::FS &fs, String updateFileName) {
        File updateBin = fs.open(updateFileName);
        if (updateBin) {
                if(updateBin.isDirectory()) {
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

void updateFirmware() {
        uint8_t cardType;
        Serial.println("Welcome to the SD-Update example!");

        // You can uncomment this and build again
        //Serial.println("Update successfull");

        //first init and check SD card
        if (!SD.begin()) {
                rebootEspWithReason("Card Mount Failed");
        }

        cardType = SD.cardType();

        if (cardType == CARD_NONE) {
                rebootEspWithReason("No SD card attached");
        }else{
                updateFromFS(SD, "/update.bin");
        }
}

void rebootEspWithReason(String reason){
        Serial.println(reason);
        delay(1000);
        ESP.restart();
}
