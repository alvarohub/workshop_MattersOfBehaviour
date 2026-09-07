#include "WebInterfaceClass.h"

bool WebInterface::bIsReceivingUpload_ = false;

void WebInterface::handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) 
{
    // change: don't use serial at any chunk to prevent charging processeur

    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    

    if (!index) {
        Serial.println(logmessage);

        Serial.printf( "SPIFFS: Total: %8u\n", SPIFFS.totalBytes() );
        Serial.printf( "SPIFFS: Used:  %8u\n", SPIFFS.usedBytes() );

        bIsReceivingUpload_ = true;
        logmessage = "Upload Start: " + String(filename);
        // open the file on first call and store the file handle in the request object
        request->_tempFile = SPIFFS.open("/" + filename, "w");
        Serial.println(logmessage);
    }

    if (len) {
        // stream the incoming chunk to the opened file
        request->_tempFile.write(data, len);
        //logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
        //Serial.println(logmessage);
        //Serial.print("FreeHeap: "); Serial.println(ESP.getFreeHeap());
    }

    if (final) {
        Serial.println(logmessage);
        size_t file_size_from_chunk = index + len;
        logmessage = "Upload Complete: " + String(filename) + ",size: " + String(file_size_from_chunk);
        // close the file handle as the upload is now done
        request->_tempFile.close();

        Serial.println(logmessage);

        bool bSuccess = true;

        // check file is ok
        File file = SPIFFS.open("/" + filename, "r");
        size_t fileSize = file.size();

        Serial.print( "File size on disk: " );
        Serial.println( fileSize );
        file.close();

        if( file_size_from_chunk != fileSize )
        {
            Serial.println( "ERR: Size mistmach" );
            bSuccess = false;
        }

        if(bSuccess)
        {
            request->send(303, "text/plain", "Upload successfull (1)"); // NB: ce texte n'est normalement pas visible.
            request->redirect("/success"); // ou directement: /
        }
        else
        {
            request->send(303, "text/plain", "ERROR: Upload Incomplete, Please Reload to resend!\n( "  + String( file_size_from_chunk ) + " differs from " + String( fileSize ) + " )");
        }
        bIsReceivingUpload_ = false;
    }
}