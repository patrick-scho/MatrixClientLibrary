idf_component_register(SRCS
                            "../../../../src/matrix.c"
                            "../../../../src/matrix_http_esp32.c"
                            "../../../../ext/mjson/src/mjson.c"
                       INCLUDE_DIRS
                            "../../../../ext/olm/include"
                            "../../../../ext/olm/lib"
                            "../../../../ext/mjson/src"
                            "../../../../src"
                       REQUIRES
                            esp-tls
                            esp_http_client
                            esp_netif
                            nvs_flash)

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fpermissive")
