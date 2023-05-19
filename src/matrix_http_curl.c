#include "matrix.h"

#include <curl/curl.h>



typedef struct {
    char * ptr;
    int cap;
    int len;
} WriteStr;

// typedef struct {
//     Str str;
//     size_t pos;
// } ReadStr;

size_t curlWriteString(char *ptr, size_t size, size_t nmemb, void *userdata) {
    WriteStr *writeStr = (WriteStr *)userdata;

    int toWrite = (int)size*nmemb;

    int writable = writeStr->cap - writeStr->len;
    int gonnaWrite = writable < (toWrite) ? writable : (toWrite);

    for (int i = 0; i < gonnaWrite; i++)
    {
        int offset = writeStr->len;
        writeStr->ptr[i+offset] = ptr[i];
    }

    writeStr->len += gonnaWrite;

    return gonnaWrite;
}
// size_t curlReadString(char *dst, size_t size, size_t nmemb, void *userdata) {
//     ReadStr *readStr = (ReadStr *)userdata;

//     size_t copyAmount = size*nmemb;
//     if (copyAmount > (readStr->str.len - readStr->pos)) {
//         copyAmount = (readStr->str.len - readStr->pos);
//     }

//     memcpy(dst, readStr->str.str + readStr->pos, copyAmount);
//     readStr->pos += copyAmount;
//     return copyAmount;
// }

CURLcode
curlPerform(CURL *curl) {
    // struct curl_slist *list = NULL;
    // list = curl_slist_append(list, uTokenHeaderStr);
    
    // curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    
    CURLcode result = curl_easy_perform(curl);

    // curl_slist_free_all(list);

    return result;
}



bool
MatrixHttpPost(
    MatrixClient * client,
    const char * url,
    char * requestBuffer, int requestLen,
    char * outResponseBuffer, int outResponseCap, int * outResponseLen
) {
    CURL *curl = (CURL *)client->httpUserData;

    CURLcode res;
    
    if(curl) {
        int urlLen = strlen(url);

        char fullUrl[MAX_URL_LEN];
        for (int i = 0; i < client->serverLen; i++)
            fullUrl[i] = client->server[i];
        for (int i = 0; i < urlLen; i++)
            fullUrl[client->serverLen+i] = url[i];
        fullUrl[client->serverLen+urlLen] = '\0';
        curl_easy_setopt(curl, CURLOPT_URL, fullUrl);
        
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBuffer);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, requestLen);
    
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        WriteStr writeStr = {
            outResponseBuffer,
            outResponseCap,
            0
        };
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &writeStr);

        res = curlPerform(curl);

        *outResponseLen = writeStr.len;

        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
    }
    

    return res == CURLE_OK;
}