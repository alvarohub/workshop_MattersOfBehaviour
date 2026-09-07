#ifndef MISBKIT_PERSISTANT_CONFIG_H
#define MISBKIT_PERSISTANT_CONFIG_H

#include <ArduinoLog.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

#define FORMAT_ON_FAIL true
#define CONFIG_FILE_NAME "/config.json"

class PersistantConfiguration{

    File config;
    JsonDocument doc;
    
    public:
    PersistantConfiguration(){};

    bool initialize(){
        if(!SPIFFS.begin(FORMAT_ON_FAIL)){
            Log.errorln("An Error has occurred while mounting SPIFFS");
            return false;
        }

        Log.infoln("File system info:\n    Total space: %d bytes\n    Space used: %d bytes", SPIFFS.totalBytes(), SPIFFS.usedBytes());
        return true;
    }

    bool load(){
        if(!SPIFFS.exists(CONFIG_FILE_NAME)){
            Log.errorln("File %s does not exist ine file system", CONFIG_FILE_NAME);
            return false;
        }
        config = SPIFFS.open(CONFIG_FILE_NAME);

    
    DeserializationError error = deserializeJson(doc,config);

    if(error){
        Log.errorln("Failed to read file, using default configuration");
        return false;
    }
    
    return true;
    }

    bool close(){
        if(!config){
            Log.errorln("File not available to close");
            return false;
        }
        doc.clear();
        config.close();
        return true;
    }

    bool save(){
        SPIFFS.remove(CONFIG_FILE_NAME);
        
        config = SPIFFS.open(CONFIG_FILE_NAME,"w");
        
        if(serializeJson(doc, config) == 0){
            Log.errorln("Failed to write config to file");
            config.close();
            return false;
        }

        config.close();
        return true;
    }

    File* get_file(){return &config;}
    JsonDocument& get_doc(){return doc;}
};

#endif //MISBKIT_PERSISTANT_CONFIG_H