/// @file crequests.h
#include <curl/curl.h>

#ifndef C_REQUESTS_H

#define C_REQUESTS_H
/** Success full operation*/
#define CREQ_ERROR_SUCCESS 0

/** Can not allocate memory using malloc() or realloc()*/
#define CREQ_ERROR_MEMORY_ALLOCATION 1

/** The error is related to cURL library */
#define CREQ_ERROR_CURL 2

/** Error happened in setting GET/POST parameter*/
#define CREQ_ERROR_SETTING_GET_PARAMS 3

/** GET/POST parameter in a form of name=value is too long.
 * it must be smaller than (8MB - len(url))
 */
#define CREQ_ERROR_PARAMS_TOO_LONG 4

typedef struct CREQ_GET_PARAMS CREQ_GET_PARAMS;

/**
 * @brief Structure to hold the GET/POST parameters.
 *
 * We use this structure to store all the user-provided
 * parameters as key-value. This structure is used internally
 * by CREQ_REQUEST_DATA structure. Whenever the function 
 * creq_add_param() is used, we add one node of this type
 * to ctx->request->params
 *
 */
struct CREQ_GET_PARAMS{
    char * key;         ///< key of the get parameter
    char * value;       ///< value of the get parameter
    CREQ_GET_PARAMS * next; ///< Pointer to the next node
};

typedef struct CREQ_HEADER CREQ_HEADER;


struct CREQ_HEADER{
    char * key;         ///< key of the header
    char * value;       ///< value of the hader
    CREQ_HEADER * next; ///< pointer to the next node
};


typedef struct CREQ_REDIRECTION_CHAIN CREQ_REDIRECTION_CHAIN;


struct CREQ_REDIRECTION_CHAIN{
    char * url;   ///< URL found in 'Location' header
    CREQ_REDIRECTION_CHAIN* next;  ///< pointer to the next node
};



typedef struct {
   int allow_redirects;   ///< Should we follow redirections?
   int verify;            ///< should we verify TLS/SSL connection?
   char * ua;             ///< User-Agent (will be freed by library)
   CREQ_GET_PARAMS* params;   ///< parameters of POST & GET requests
   char * url;            ///< URL to get (will be freed by library)
   long request_timeout;  ///< Maximum time (in seconds) before timeout
   struct curl_slist * headers; ///< curl will free this option for you
   char method[5];  ///< Method used (HEAD, POST, GET)
   char * proxy;    ///< String for proxy to use
} CREQ_REQUEST_DATA;

typedef struct {
    long status_code;  ///< returned HTTP status code
    char * url;   ///< final destination URL
    char * mem;   ///< pointer to memory we keep the body of the request
    size_t len;   ///< Length of the body of the request
    CREQ_HEADER * headers;  ///< linked-list of response headers
    CREQ_REDIRECTION_CHAIN * follow_chain; ///< Linked-list of redirections
} CREQ_RESPONSE_DATA;

typedef struct {
    int err_code;  ///< Any possible error code (0 means success)
    CURL * handle; ///< Internal curl handle
    CREQ_RESPONSE_DATA * response; ///< response structure
    CREQ_REQUEST_DATA * request;   ///< request structure
    CURLcode curl_code;            ///< possible curl error code
    short int _is_redirection;     ///< Internal member
} CREQ_CTX;



char * creq_error(CREQ_CTX *);
CREQ_CTX * creq_init(void);
void creq_get(CREQ_CTX * ctx, char * url);
void creq_close(CREQ_CTX * ctx);
char * creq_get_content(CREQ_CTX*);
void creq_set_allow_redirects(CREQ_CTX *, int);
void creq_set_useragent(CREQ_CTX * ctx, char * ua);
void creq_add_param(CREQ_CTX * ctx, char * key, char * value);
void creq_set_timeout(CREQ_CTX * ctx, long timesec);
void creq_add_header(CREQ_CTX * ctx, char * header);
void creq_post(CREQ_CTX * ctx, char * url);
void creq_head(CREQ_CTX * ctx, char * url);
void creq_set_proxy(CREQ_CTX *ctx, char * proxy);
void creq_set_verify(CREQ_CTX * ctx, int val);
char * creq_get_response_header(CREQ_CTX * ctx, char * header);
/*******************utility function*******************/


#endif
