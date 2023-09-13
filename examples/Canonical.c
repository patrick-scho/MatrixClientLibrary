#include <matrix.h>
#include <mjson.h>
#include <olm/sas.h>

#include <stdio.h>

int
main(void)
{
    const char json[] =
    "{"
    "\"method\":\"m.sas.v1\","
    "\"from_device\":\"ULZZOKJBYN\","
    "\"key_agreement_protocols\":[\"curve25519-hkdf-sha256\",\"curve25519\"],"
    "\"hashes\":[\"sha256\"],"
    "\"message_authentication_codes\":[\"hkdf-hmac-sha256.v2\",\"org.matrix.msc3783.hkdf-hmac-sha256\",\"hkdf-hmac-sha256\",\"hmac-sha256\"],"
    "\"short_authentication_string\":[\"decimal\",\"emoji\"],"
    "\"transaction_id\":\"CmMReoy5AK59qd7pa6EO7ocbFwX03isB\""
    "}";
    
    char canonical[1024];

    JsonCanonicalize(json, strlen(json), canonical, 1024);

    printf("%s\n", canonical);

    return 0;
}
