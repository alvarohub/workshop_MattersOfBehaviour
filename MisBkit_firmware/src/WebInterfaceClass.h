
#ifndef MBK_WEBINTERFACE_H
#define MBK_WEBINTERFACE_H

#include <Arduino.h>
#include <ArduinoLog.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncHTTPUpdateServer.h>
#include <SPIFFS.h>

#include "WebInterfaceData.h"


class WebInterface
{
private:
    AsyncWebServer *server;
    ESPAsyncHTTPUpdateServer updateServer;
    static bool bIsReceivingUpload_; // prevent all other process to run

    static void handleNotFound(AsyncWebServerRequest *request) {
        String path = request->url();
        Serial.println("INF: handleNotFound: Requested: " + path); // TODO: changer en Log.infoln, mais il ne gere pas bien ma chaine.
        path = String("/") + getRelativeFilename(path);
        Serial.println("INF: handleNotFound: Converted to: " + path);
        
        if (SPIFFS.exists(path)) 
        {
            // response->addHeader("Content-Encoding", "gzip"); // TODO ?
            //request->send(SPIFFS, path, String(), true);

            String contentType;

            // TODO: functionnalize to getContentsType (mais il faudrait refactoriser ce fichier: il y a trop de corps de fonctions dans le .h)
            if (path.endsWith(".html"))      contentType = "text/html";
            else if (path.endsWith(".css"))  contentType = "text/css";
            else if (path.endsWith(".js"))   contentType = "application/javascript";
            else if (path.endsWith(".png"))  contentType = "image/png";
            else if (path.endsWith(".svg"))  contentType = "image/svg+xml";
            else if (path.endsWith(".svg.gz"))  contentType = "image/svg+xml";
            else                             contentType = "text/plain";

            AsyncWebServerResponse *response = request->beginResponse(SPIFFS, path, contentType);
            if( ! path.endsWith("script.js") ) // on ne cache pas le script car actuellement je le change souvent
            {
                response->addHeader("Cache-Control", "max-age=1200");  // 1 an: 31536000, 1j: 86400, 21600: 6h, 1200: 2h
            }
            request->send(response);
        } 
        else 
        {
            Serial.println("WRN: handleNotFound: ! Not found: " + path);
            request->send(404, "text/plain", "Not Found");
        }
    }

    
    static u8_t * readFromSPIFFs( const char * filename ) {
        static u8_t * readFromSPIFFs_buf = NULL; // owned by readFromSPIFFs
        File file = SPIFFS.open(filename);
        if(!file){
            Serial.println("Failed to open file for reading");
            return NULL;
        }
        int size = file.size();
        
        if(readFromSPIFFs_buf != NULL)
        {
            Serial.println("readFromSPIFFs: freeing buf");
            free(readFromSPIFFs_buf);
        }
        Serial.print("readFromSPIFFs: allocating size: "); Serial.println(size);

        readFromSPIFFs_buf = (u8_t * ) malloc( size + 1 );
        while(file.available())
        {
            file.read(readFromSPIFFs_buf,size); // in one read
        }
        readFromSPIFFs_buf[size] = 0; // mark the end
        file.close();
        return readFromSPIFFs_buf;
    }

    static String getRelativeFilename(const String &path) {
        // Convert an absolute filename to a relative one
            if (path.length() == 0) return "";

        int lastSlash = path.lastIndexOf('/');

        if (lastSlash < 0) return path;

        return path.substring(lastSlash + 1);
    }

    static void handleSpecific(AsyncWebServerRequest *request, const char* filename, const char * content_type, const u8_t * alternate_gz, int alternate_gz_size ) {
        Log.infoln("Serving ", filename);
        /*
        const u8_t * buf = readFromSPIFFs(filename);
        int size;
        bool is_gzip = false;
        if(buf == NULL) {
            buf = alternate_gz;
            size = alternate_gz_size;
            is_gzip = true;
        }
        else {
            size = strlen((const char*)buf);
        }
        AsyncWebServerResponse *response = request->beginResponse(200, content_type, buf, size);
        if( is_gzip ) response->addHeader("Content-Encoding", "gzip");
        request->send(response);
        */
        // le code précédent genere des problemes de parallelisme si les fichiers sont demandé en meme temps, car on a un seul buffer partagé et le browser demande plusieurs fichiers en meme temps.
        AsyncWebServerResponse *response;
        File file = SPIFFS.open(filename);
        if(!file){
            response = request->beginResponse(200, content_type, alternate_gz, alternate_gz_size);
            response->addHeader("Content-Encoding", "gzip");
        }
        else
        {
            file.close();
            response = request->beginResponse(SPIFFS, filename, content_type);
        }
        response->addHeader("Cache-Control", "max-age=864000");
        request->send(response);
       
    }

    static void handleRoot(AsyncWebServerRequest *request) {
        handleSpecific( request, "/index.html", "text/html", index_html_gz, index_html_gz_len );
    }

    static void handleStyles(AsyncWebServerRequest *request) {
        handleSpecific( request, "/style.css", "text/css", styles_css_gz, styles_css_gz_len );
    }

    static void handleScripts(AsyncWebServerRequest *request) {
        handleSpecific( request, "/scripts.js", "application/javascript", scripts_js_gz, scripts_js_gz_len );
    }

    static void handleEmergency(AsyncWebServerRequest *request) {
        const char * html_code = "<html><body><form method='POST' action='/upload' enctype='multipart/form-data'><input type='file' name='data' multiple /><input type='submit' name='upload' value='Upload' title='Upload File'></form></body></html>";
        AsyncWebServerResponse *response = request->beginResponse(200, "text/html", (const uint8_t*) html_code, strlen(html_code));
        request->send(response);
    }

    // handles uploads
    static void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);


public:

    WebInterface( AsyncWebServer *server): server(server) {};
    static bool isReceivingUpload() { return bIsReceivingUpload_; }
    ~WebInterface(){};
    void initialize() {
        Log.infoln("Initializing web interface");
        server->on("/", HTTP_GET, handleRoot);
        server->on("/style.css", HTTP_GET, handleStyles);
        server->on("/scripts.js", HTTP_GET, handleScripts);
        server->on("/emergency.html", HTTP_GET, handleEmergency);
        server->onNotFound(handleNotFound);

        // run handleUpload function when any file is uploaded
        server->on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
                //request->send(200); // Ne pas mettre cette ligne, sinon ca nous empeche de renvoyer un code 300 plus tard.
            }, handleUpload);

        server->on("/success", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send(200, "text/html", "<h2>Upload successfull !</h2><script>setTimeout(() => {window.location.href = '/'; }, 500);</script>");
        });
            
        updateServer.setup(server);
        updateServer.onUpdateBegin = [](const UpdateType type, int &result)
    {
        //you can force abort the update like this if you need to:
        //result = UpdateResult::UPDATE_ABORT;        
        Serial.println("Update started : " + String(type));
    };

    updateServer.onUpdateEnd = [](const UpdateType type, int &result)
    {
        Serial.println("Update finished : " + String(type) + " result: " + String(result));
    };

}
};

#endif // MBK_WEBINTERFACE_H
