idf_component_register(SRCS
                            "../../../../ext/olm/src/account.cpp"
                            "../../../../ext/olm/lib/crypto-algorithms/aes.c"
                            "../../../../ext/olm/src/base64.cpp"
                            "../../../../ext/olm/src/cipher.cpp"
                            "../../../../ext/olm/src/crypto.cpp"
                            "../../../../ext/olm/lib/curve25519-donna/curve25519-donna.c"
                            "../../../../ext/olm/src/ed25519.c"
                            "../../../../ext/olm/src/error.c"
                            "../../../../ext/olm/src/inbound_group_session.c"
                            "../../../../ext/olm/src/megolm.c"
                            "../../../../ext/olm/src/memory.cpp"
                            "../../../../ext/olm/src/message.cpp"
                            "../../../../ext/olm/src/olm.cpp"
                            "../../../../ext/olm/src/outbound_group_session.c"
                            "../../../../ext/olm/src/pickle.cpp"
                            "../../../../ext/olm/src/pickle_encoding.c"
                            "../../../../ext/olm/src/pk.cpp"
                            "../../../../ext/olm/src/ratchet.cpp"
                            "../../../../ext/olm/src/sas.c"
                            "../../../../ext/olm/src/session.cpp"
                            "../../../../ext/olm/lib/crypto-algorithms/sha256.c"
                            "../../../../ext/olm/src/utility.cpp"
                       INCLUDE_DIRS
                            "../../../../ext/olm/include"
                            "../../../../ext/olm/lib"
                            "../../../../src"
                       REQUIRES
                            esp_timer)

add_definitions(-DOLM_STATIC_DEFINE)
add_definitions(-DOLMLIB_VERSION_MAJOR=3)
add_definitions(-DOLMLIB_VERSION_MINOR=2)
add_definitions(-DOLMLIB_VERSION_PATCH=15)

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fpermissive -Wl,--allow-multiple-definition")

