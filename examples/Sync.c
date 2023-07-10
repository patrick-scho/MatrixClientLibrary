#include <matrix.h>
#include <stdio.h>

#define SERVER       "https://matrix.org"
#define ACCESS_TOKEN "syt_cHNjaG8_yBvTjVTquGCikvsAenOJ_49mBMO"

int
main(void)
{
    MatrixClient client;
    MatrixClientInit(&client,
        SERVER);
    
    MatrixHttpInit(&client);

    MatrixClientSetAccessToken(&client,
        ACCESS_TOKEN);

    static char syncBuffer[40000];
    MatrixClientSync(&client,
        syncBuffer, 40000);
    printf("%s", syncBuffer);
        
    MatrixHttpDeinit(&client);

    return 0;
}