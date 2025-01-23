#include <b64/cencode.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
char *ws_createAcceptToken(char *key);
char *ws_generateHTTPResponse(char *key);
