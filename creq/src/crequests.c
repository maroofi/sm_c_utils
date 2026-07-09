/// @file crequests.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "crequests.h"


/*Some functions that are only used in this file*/
static void initialize_structure(CREQ_CTX *ctx);
static size_t _str_len(const char * str);
static char * creq_strip_line(char * line);
static void _creq_received_header(CREQ_CTX * ctx, char * header);
static void add_to_follow_chain(CREQ_CTX * ctx, char * url);
static void _creq_remove_response_header(CREQ_CTX * ctx);
static char * _creq_strip_line(char * line);
static void creq_construct_params(CREQ_CTX * ctx);
static size_t creq_header_callback(char *buff, size_t size,size_t nitems, void *userdata);
static size_t creq_callback_routine(char *, size_t, size_t, void*);
static void creq_free_follow_chain(CREQ_CTX * ctx);
static void creq_free_response_header(CREQ_CTX *ctx);
static void creq_free_params(CREQ_CTX* ctx);


/**
 * @brief Initialize the creq library for a new request
 * 
 * This must be the first function to call when you want to use this library.
 * This function will initialize all the necessary structures and values.
 *
 * You should always use a new context for each request. If you want to send
 * 10 requests, call creq_init() 10 times to get 10 context and then close
 * all of them by calling creq_close() for each context.
 *
 * @return A pointer to CREQ_CTX structure on success or NULL on failure.
 *
 * @code
 *   int main(){
 *       // initialize the library
 *       CREQ_CTX * ctx = creq_init();
 *       if (!ctx){
 *           fprintf(stdout, "Failed to init the library...\n");
 *           return 1;
 *       }
 *       // continue doing stuff here
 *       creq_close(ctx);
 *       return 0;
 *   }
 *  @endcode
 *
 */
CREQ_CTX * creq_init(void){
    CREQ_CTX * ctx = (CREQ_CTX*) malloc(sizeof(CREQ_CTX));
    if (!ctx)
        return NULL;
    ctx->err_code = CREQ_ERROR_SUCCESS;
    ctx->request = (CREQ_REQUEST_DATA *) malloc(sizeof(CREQ_REQUEST_DATA));
    ctx->response = (CREQ_RESPONSE_DATA *) malloc(sizeof(CREQ_RESPONSE_DATA));
    if (!ctx->response || !ctx->request){
        free(ctx);
        return NULL;
    }
    ctx->response->mem = NULL;
    ctx->response->len = 0;
    ctx->request->allow_redirects = 1;
    ctx->request->verify = 1;
    ctx->response->url = NULL;
    ctx->response->headers = NULL;
    ctx->request->proxy = NULL;
    ctx->request->ua = NULL;
    ctx->request->params = NULL;
    ctx->request->params = NULL;
    ctx->request->url = NULL;
    ctx->request->request_timeout = 0;
    ctx->request->headers = NULL;
    ctx->handle = curl_easy_init();
    ctx->response->follow_chain = NULL;
    ctx->_is_redirection = 0;
    ctx->request->method[0] = '\0';
    ctx->curl_code = CURLE_OK;
    return ctx;
}


/**
 * @brief Adds a header to the header list for sending
 * @param ctx The context returned by creq_init()
 * @param header The header you want to set in a form of "header-name: header-value"
 *
 * This function sets a new header for the request.
 * The **header** param will be copied by the library. So it's up to the user to free
 * the allocated memory for the passed parameter.
 *
 * This function has no return value.
 *
 * @code
 * int main(){
 *     CREQ_CTX * ctx = creq_init();
 *     if (!ctx){
 *         fprintf(stdout, "Fail to init the library....\n");
 *         return 1;
 *     }
 *     // Using stack for the new header. Will be copied to the internal structure.
 *     creq_add_header(ctx, "Authorization: Bearer thisismytoken");
 *     creq_get(ctx, "https://example.org");
 *     creq_close(ctx);
 *     return 0;
 * }
 * @endcode
 */
void creq_add_header(CREQ_CTX * ctx, char * header){
    if (!header)
        return;
    ctx->request->headers = curl_slist_append(ctx->request->headers, header);
}


/**
 * @brief Sets a proxy for sending requests.
 * @param ctx Context returned by creq_init()
 * @param proxy proxy string (as used in curl command line)
 *
 * This function sets a proxy for the library.
 *
 * Proxy is usually in a form of **protocol://IP:PORT**.
 *
 * **protocol** can be _http_, _https_, _socks5_, _socks5h_ and _socks4_.
 * 
 * For example: _socks5h://127.0.0.1:1031_
 *
 * The __proxy__ parameter will be coppied to the internal structure.
 * So it's up the user to free the passed __proxy__ param.
 *
 * This function has no return value.
 *
 * @code
 * int main(){
 *     CREQ_CTX *ctx = creq_init();
 *     if (!ctx){
 *         fprintf(stdout, "Fail to init the library....\n");
 *         return 1;
 *     }
 *     // Let's force it to use socks5
 *     creq_set_proxy(ctx, "socks5://1.2.3.4:1080");
 *     creq_get(ctx, "http://example.com");
 *     creq_close();
 *     return 0;
 * }
 * @endcode
 */
void creq_set_proxy(CREQ_CTX * ctx, char * proxy){
    if (!proxy)
        return;
    ctx->request->proxy = strdup(proxy);
}

/**
 * @brief Frees the allocated memory for the parameters.
 *
 * @param ctx Context returned by creq_init()
 *
 * Usually you don't need to even call this function. When you call creq_close(),
 * it will call this function internally if it's necessary.
 *
 * This function has no return value.
 */
static void creq_free_params(CREQ_CTX * ctx){
    CREQ_GET_PARAMS * params = ctx->request->params;
    // free all the memory regarding parameters
    if (params == NULL)
        return;
    CREQ_GET_PARAMS * tmp = params;
    while(tmp){
        free(tmp->key);
        free(tmp->value);
        params = tmp->next;
        free(tmp);
        tmp = params;
    }
    ctx->request->params = NULL;
}


/**
 * @brief Frees the redirection chain.
 *
 * You don't need to call this function. This will be called internally when
 * you call creq_close() function.
 *
 */
static void creq_free_follow_chain(CREQ_CTX * ctx){
    CREQ_REDIRECTION_CHAIN * chain = ctx->response->follow_chain;
    if (chain == NULL)
        return;
    CREQ_REDIRECTION_CHAIN * tmp = chain;
    while(tmp){
        free(tmp->url);
        chain = tmp->next;
        free(tmp);
        tmp = chain;
    }
    ctx->response->follow_chain = NULL;
}

/**
 * @brief Frees the response headers.
 *
 * You don't have to call this function. It will be called by creq_close().
 */
static void creq_free_response_header(CREQ_CTX * ctx){
    
    CREQ_HEADER * headers = ctx->response->headers;
    // free all the memory regarding response headers
    if (headers == NULL)
        return;
    CREQ_HEADER * tmp = headers;
    while(tmp){
        free(tmp->key);
        free(tmp->value);
        headers = tmp->next;
        free(tmp);
        tmp = headers;
    }
    ctx->response->headers = NULL;
}

/**
 * @brief Cleans up the library and memory after using it.
 * @param ctx Context returned by creq_init()
 *
 * Whenever you call creq_init(), you must call this method to clean up 
 * all the allocated memory and the internal structure for you.
 */
void creq_close(CREQ_CTX * ctx){
    free(ctx->response->mem);
    free(ctx->request->ua);
    free(ctx->request->url);
    free(ctx->request->proxy);
    creq_free_params(ctx);
    creq_free_response_header(ctx);
    curl_slist_free_all(ctx->request->headers);
    curl_easy_cleanup(ctx->handle);
    creq_free_follow_chain(ctx);
    free(ctx->request);
    free(ctx->response);
    free(ctx);
}

/**
 * @brief Tells the library if it should follow redirection.
 *
 * @param ctx Context returned by creq_init()
 * @param val 0 to disable redirection and 1 to enable it (default).
 *
 * By default, the library will follow redirection up to 10 times.
 * You can pass 0 to this function to disable this behavior.
 *
 * This function has no return value.
 *
 * @code
 * int main(){
 *     CREQ_CTX *ctx = creq_init();
 *     if (!ctx){
 *         fprintf(stdout, "Fail to init the library....\n");
 *         return 1;
 *     }
 *     // Let's not follow the redirection
 *     creq_set_allow_redirects(ctx, 0);
 *     creq_get(ctx, "http://example.com");
 *     creq_close();
 *     return 0;
 * }
 * @endcode
 */
void creq_set_allow_redirects(CREQ_CTX * ctx, int val){
    val = val?1:0;
    ctx->request->allow_redirects = val;
}

/**
 * @brief Enables/disables SSL/TLS verification
 * @param ctx Context returned by creq_init()
 * @param val either zero (disable verification) or one (enable verification).
 *
 * By default, the SSL verification is enabled internally by curl library.
 * It's not safe to disable it but if you know what you are doing, you can 
 * disable SSL verification by passing zero to this function.
 *
 * This function has no return value.
 */
void creq_set_verify(CREQ_CTX * ctx, int val){
    val = val?1:0;
    ctx->request->verify = val;
}

/*
 * @brief Get body of the data through this callback.
 * @param buff A pointer to the memory which will receive new data
 * @param size size of each unit of data
 * @param nmemb size of received data
 * @param userp user-specified data
 *
 * You don't need to call this function as the creq_get() and creq_post() will use this
 * function internally to receive data from curl.
 *
 * @return Always return the size of the data it received in buffer.
 *
 */
static size_t creq_callback_routine(char *buff, size_t size, size_t nmemb, void *userp){
    size_t real_size = size * nmemb;
    CREQ_CTX * pres = (CREQ_CTX*) userp;
    char * ptr = (char*) realloc(pres->response->mem, pres->response->len + real_size + 1);
    if (!ptr){
        pres->err_code = CREQ_ERROR_MEMORY_ALLOCATION;
        return real_size;
    }
    pres->response->mem = ptr;
    memcpy(pres->response->mem + pres->response->len, buff, real_size);
    pres->response->len = pres->response->len + real_size;
    pres->response->mem[pres->response->len] = '\0';
    return real_size;
}

/**
 * @brief Sets timeout for both connection and data transfer
 * @param ctx Context returned by creq_init()
 * @param timesec Maximum number of second to wait for connection and receiving data
 *
 * It's always a good practice to set a timeout for each request.
 * This function sets the timeout to **timesec** seconds. For example,
 * setting the timeout to 10 means that the request has at most 10 seconds to 
 * connect to the server and transfer the data. If it takes more than that, the 
 * library will return timeout error.
 *
 * This function has no return value.
 */
void creq_set_timeout (CREQ_CTX * ctx, long timesec){
    ctx->request->request_timeout = timesec;
}


/*
 * @brief Get header of the data through this callback.
 * @param buff A pointer to the memory which will receive new data
 * @param size size of each unit of data
 * @param nmemb size of received data
 * @param userp user-specified data
 *
 * You don't need to call this function as the creq_get() and creq_post() and creq_head() 
 * will use this function internally to receive headers from curl.
 *
 * @return Always return the size of the data it received in buffer.
 *
 */
static size_t creq_header_callback(char *buff, size_t size,size_t nitems, void *userdata){
    CREQ_CTX * ctx = (CREQ_CTX*)userdata;
    char * new_header = (char*) malloc(size* nitems + 1);
    if (!new_header)
        return nitems * size;
    int ch;
    int count_eol = 0;
    for (unsigned int i=0; i<nitems; ++i){
        ch = buff[i];
        if (ch == '\0'){        // null char in header is not valid
            fprintf(stderr, "NULL character found in the header\n");
            continue;
        }
        if (ch == '\n' || ch == '\r')
            count_eol++;
        new_header[i] = ch;
    }
    new_header[nitems] = '\0';
    char * eols[] = {"\r\n", "\r\n\r\n", "\n\n", NULL};
    int eol_found = 0;
    for (int i = 0; i< 3; ++i){
        if (strcmp(new_header, eols[i]) == 0){
            eol_found = 1;
            break;
        }
    } 
    if (eol_found){
        // we are at the end of header
        if (ctx->_is_redirection){
            _creq_remove_response_header(ctx);
            ctx->_is_redirection = 0;
            free(new_header);
            return nitems * size;
        }
    }
    _creq_received_header(ctx, new_header);
    free(new_header);
    return nitems * size;  //just nitems is enough
}

/**
 * @brief Internal function to add headers to header structure.
 *
 * You should not use this function.
 *
 */
static void _creq_received_header(CREQ_CTX * ctx, char * header){
    if (!header)
        return;
    size_t len = _str_len(header);
    if (len == 0)
        return;
    char * new_header = creq_strip_line(header);
    if (!new_header)
        return;
    len = _str_len(new_header);
    if (len == 0){
        free(new_header);
        return;
    }
    CREQ_HEADER * tmp = ctx->response->headers;
    char * colon_pos = strstr(new_header, ":");

    if (colon_pos == NULL){
        free(new_header);
        return;
    }
    char * key = (char*) malloc(colon_pos - new_header + 1);
    if(!key){
        free(new_header);
        return;
    }
    memcpy(key, new_header, colon_pos - new_header);
    key[colon_pos - new_header] = '\0';
    char * value = (char*) malloc(new_header + len - colon_pos);
    if (!value){
        free(new_header);
        free(key);
        return;
    }
    memcpy(value, colon_pos + 1, new_header + len - colon_pos -1);
    value[new_header+len-colon_pos -1] = '\0';
    free(new_header);

    char * new_value = creq_strip_line(value);
    if (!new_value){
        free(value);
        free(key);
    }

    free(value);
    char location[] = "location";
    if(strcasecmp(key, location) == 0 && _str_len(key) == _str_len(location)){
        // we have a location match
        ctx->_is_redirection = 1;
        CREQ_REDIRECTION_CHAIN * r = (CREQ_REDIRECTION_CHAIN*) malloc(sizeof(CREQ_REDIRECTION_CHAIN));
        if (r){
            r->url = strdup(new_value);
            r->next = NULL;
            CREQ_REDIRECTION_CHAIN * tmp = ctx->response->follow_chain;
            if (tmp == NULL){
                // this is the first one
                tmp = r;
            }else{
                while(tmp && tmp->next)
                    tmp = tmp->next;
                tmp->next = r;
            }
        }
#ifdef DEBUG
        fprintf(stdout, "****Location header detectecd\n");
#endif
    }
    

#ifdef DEBUG
    fprintf(stdout, "'%s' -> '%s'\n", key, new_value);
#endif
    CREQ_HEADER * h = (CREQ_HEADER*) malloc(sizeof(CREQ_HEADER));
    if (!h){
        free(key);
        free(new_value);
        return;
    }
    h->next = NULL;
    h->key = key;
    h->value = new_value;

    while (tmp && tmp->next)
        tmp = tmp->next;
    
    if (tmp == NULL){    // first header to add
        ctx->response->headers = h;
    }else{
        tmp->next = h;
    }
}

/**
 * @brief Frees the memory for response headers.
 *
 * This is an internal function not to be used by users.
 *
 */
static void _creq_remove_response_header(CREQ_CTX * ctx){
    CREQ_HEADER * tmp = ctx->response->headers;
    CREQ_HEADER * a_tmp = NULL;
    if (NULL == tmp)
        return;
    while (tmp){
        a_tmp = tmp;
        tmp = tmp->next;
        free(a_tmp->key);
        free(a_tmp->value);
        free(a_tmp);
    }
    ctx->response->headers = NULL;
}

/**
 * @brief Internal function to initialize the context before request.
 *
 * This is an internal function only should used by the library
 *
 */
static void initialize_structure(CREQ_CTX *ctx){
    ctx->response->status_code = 0;
    free(ctx->response->mem);
    free(ctx->request->url);
    ctx->response->len = 0;
    ctx->curl_code = CURLE_OK;
    ctx->response->url = NULL;
    curl_easy_setopt(ctx->handle, CURLOPT_TIMEOUT, ctx->request->request_timeout);
    curl_easy_setopt(ctx->handle, CURLOPT_CONNECTTIMEOUT,
                     ctx->request->request_timeout == 0?30:ctx->request->request_timeout);
    if (ctx->request->headers)
        curl_easy_setopt(ctx->handle, CURLOPT_HTTPHEADER, ctx->request->headers);
    // assuming it's compiled with gzip option
    curl_easy_setopt(ctx->handle, CURLOPT_ACCEPT_ENCODING, "gzip");
    if (ctx->request->proxy)
        curl_easy_setopt(ctx->handle, CURLOPT_PROXY, ctx->request->proxy);
    int option_result = 0;
    option_result = curl_easy_setopt(ctx->handle, CURLOPT_WRITEFUNCTION, creq_callback_routine);
    if (option_result != 0)
        fprintf(stdout, "%s\n", curl_easy_strerror(option_result));
    
    
    option_result = curl_easy_setopt(ctx->handle, CURLOPT_WRITEDATA, (void*)ctx);
    if (option_result != 0)
        fprintf(stdout, "%s\n", curl_easy_strerror(option_result));
    
    option_result = curl_easy_setopt(ctx->handle, CURLOPT_HEADERFUNCTION,
                                     creq_header_callback);

    if (option_result != 0)
        fprintf(stdout, "%s\n", curl_easy_strerror(option_result));
    curl_easy_setopt(ctx->handle, CURLOPT_HEADERDATA, (void*)ctx);

    curl_easy_setopt(ctx->handle, CURLOPT_FOLLOWLOCATION, ctx->request->allow_redirects);
    // it's unlimited by default
    curl_easy_setopt(ctx->handle, CURLOPT_MAXREDIRS, 10);
    if (ctx->request->verify){
        curl_easy_setopt(ctx->handle, CURLOPT_SSL_VERIFYHOST, 2);
        curl_easy_setopt(ctx->handle, CURLOPT_SSL_VERIFYPEER, 1);
    }else{
        curl_easy_setopt(ctx->handle, CURLOPT_SSL_VERIFYHOST, 0);
        curl_easy_setopt(ctx->handle, CURLOPT_SSL_VERIFYPEER, 0);
    }
    if (ctx->request->ua == NULL){
        curl_easy_setopt(ctx->handle, CURLOPT_USERAGENT, "creq version 0.1");
    }else{
        curl_easy_setopt(ctx->handle, CURLOPT_USERAGENT, ctx->request->ua);
    }
    
}

/**
 * @brief Add user-agent to the request header
 *
 * Using this function, you can set your user-agent as a header
 * in the request. You can also use creq_add_header() function to 
 * do the same thing. I created a separate function for it since it is
 * used a lot of time.
 *
 * The library copy the user-agent string internally. So the user
 * should free the __ua__ parameter after use.
 *
 * @param ctx Context returned by creq_init()
 * @param ua A pointer to the new user-agent.
 */
void creq_set_useragent(CREQ_CTX * ctx, char * ua){
    free(ctx->request->ua);
    size_t len = _str_len(ua);
    ctx->request->ua = (char*) malloc(len + 1);
    memcpy(ctx->request->ua, ua, len);
    ctx->request->ua[len] = '\0';
}

/**
 * @brief An internal function to add new URL to redirection chain
 *
 */
static void add_to_follow_chain(CREQ_CTX * ctx, char * url){
    if (!url)
        return;
    char * url_to_add = strdup(url);
    if (!url_to_add)
        return;
    CREQ_REDIRECTION_CHAIN * r = (CREQ_REDIRECTION_CHAIN*) malloc(sizeof(CREQ_REDIRECTION_CHAIN));
    if (!r){
        free(url_to_add);
        return;
    }
    r->next = NULL;
    r->url = url_to_add;
    ctx->response->follow_chain = r;
}

/**
 * @brief Sends a GET request to the specified URL.
 * @param ctx Context returned by creq_init()
 * @param url A pointer to the URL
 *
 * This function sends a GET request to the specified URL. The 
 * **url** param will be copied internally to the library so users
 * can free the memory for **url** after use.
 *
 * @return The function does not return any result. All the results will be 
 * stored in the context and cat be retrieved using **ctx** parameter.
 *
 * @code
 * // Send get request to get the IP address and print the status code
 * int main(){
 *     CREQ_CTX * ctx = creq_init();
 *     if (!ctx){
 *         fprintf(stderr, "Can not init the library...\n");
 *         return 1;
 *     }
 *     char url[] = "https://ipinfo.io/ip"
 *     creq_set_timeout(ctx, 10);
 *     creq_get(ctx, url);
 *     if (ctx->err_code != CREQ_ERROR_SUCCESS){
 *         fprintf(stderr, "Error(%d:%d): %s\n", ctx->err_code, ctx->curl_code, creq_error(ctx));
 *         creq_close(ctx);
 *         return 1;
 *     }
 *     // now that everything is good, we can print stuff
 *     // print http status code
 *     fprintf(stdout, "Status Code: %d\n", ctx->response->status_code);
 *     //  print my ip address
 *     fprintf(stdout, "My IP address is: %s\n", creq_get_content(ctx));
 *     creq_close(ctx);
 *     return 0;
 * }
 * @endcode
 */
void creq_get(CREQ_CTX * ctx, char * url){
    if (!url)
        return;
    // gets the data from the given URL
    strcpy(ctx->request->method, "GET");
    initialize_structure(ctx);
    ctx->request->url = strdup(url);
    add_to_follow_chain(ctx, url);
    creq_construct_params(ctx);
    curl_easy_setopt(ctx->handle, CURLOPT_URL, ctx->request->url);
    CURLcode res = curl_easy_perform(ctx->handle);
    
    ctx->err_code = CREQ_ERROR_SUCCESS;
    ctx->curl_code = res;
    if (res == 0)
        ctx->err_code = CREQ_ERROR_SUCCESS;
    else
        ctx->err_code = CREQ_ERROR_CURL;
    if (res == CURLE_OK){
        curl_easy_getinfo(ctx->handle, CURLINFO_RESPONSE_CODE, &(ctx->response->status_code));
        curl_easy_getinfo(ctx->handle, CURLINFO_EFFECTIVE_URL, &ctx->response->url);
    }
    
}

/**
 * @brief Sends a post request.
 * @param ctx Context returned by creq_init()
 * @param url A pointer to the URL
 *
 * This function sends a post request to the given URL.
 * The library copies the given **url** to its internal structure
 * so users can free the **url** after using this function.
 *
 * Post parameters are added using creq_add_param() method. 
 * Note that currently, this method does not support file upload.
 * This means all the params must be text and not binary data.
 *
 * The default encoding type of the data is application/x-www-form-urlencoded.
 *
 * as long as the binary data is base64-encoded or any other human readable characters,
 * it should be fine to send it with this method.
 *
 *This function has no return value.
 *
 * @code
 * 
 * int main(){
 *     CREQ_CTX * ctx = creq_init();
 *     if (!ctx){
 *         fprintf(stderr, "Can not init the library...\n");
 *         return 1;
 *     }
 *     char url[] = "https://reqbin.com/echo/post/json"
 *     creq_set_timeout(ctx, 10);
 *     creq_post(ctx, url);
 *     if (ctx->err_code != CREQ_ERROR_SUCCESS){
 *         fprintf(stderr, "Error(%d:%d): %s\n", ctx->err_code, ctx->curl_code, creq_error(ctx));
 *         creq_close(ctx);
 *         return 1;
 *     }
 *     // now that everything is good, we can print stuff
 *     // print http status code
 *     fprintf(stdout, "Status Code: %d\n", ctx->response->status_code);
 *     // print the output
 *     fprintf(stdout, "%s\n", creq_get_content(ctx));
 *     creq_close(ctx);
 *     return 0;
 * }
 * @endcode
 */
void creq_post(CREQ_CTX * ctx, char * url){
    if (!url)
        return;
    strcpy(ctx->request->method, "POST");
    initialize_structure(ctx);
    ctx->request->url = strdup(url);
    add_to_follow_chain(ctx, url);
    creq_construct_params(ctx);
    curl_easy_setopt(ctx->handle, CURLOPT_URL, ctx->request->url);
    //tell curl to send post request
    curl_easy_setopt(ctx->handle, CURLOPT_POST, 1);
    CURLcode res = curl_easy_perform(ctx->handle);
    ctx->err_code = CREQ_ERROR_SUCCESS;
    ctx->curl_code = res;
    if (res == 0)
        ctx->err_code = CREQ_ERROR_SUCCESS;
    else
        ctx->err_code = CREQ_ERROR_CURL;
    if (res == CURLE_OK){
        curl_easy_getinfo(ctx->handle, CURLINFO_RESPONSE_CODE, &(ctx->response->status_code));
        curl_easy_getinfo(ctx->handle, CURLINFO_EFFECTIVE_URL, &ctx->response->url);
    }
}

/**
 * @brief Sends a head request.
 * @param ctx Context returned by creq_init()
 * @param url A pointer to the URL to send HEAD request to.
 *
 * This function sends a HEAD request to the given URL.
 * It works exactly as creq_get() but there is no body in response just headers.
 *
 * Look at the creq_get() doc for the example.
 *
 * This function has no return value.
 */
void creq_head(CREQ_CTX * ctx, char * url){
    if (!url)
        return;
    strcpy(ctx->request->method, "HEAD");
    initialize_structure(ctx);
    ctx->request->url = strdup(url);
    add_to_follow_chain(ctx, url);
    creq_construct_params(ctx);
    curl_easy_setopt(ctx->handle, CURLOPT_URL, ctx->request->url);
    //tell curl to send post request
    curl_easy_setopt(ctx->handle, CURLOPT_NOBODY, 1);
    CURLcode res = curl_easy_perform(ctx->handle);
    ctx->err_code = CREQ_ERROR_SUCCESS;
    ctx->curl_code = res;
    if (res == 0)
        ctx->err_code = CREQ_ERROR_SUCCESS;
    else
        ctx->err_code = CREQ_ERROR_CURL;
    if (res == CURLE_OK){
        curl_easy_getinfo(ctx->handle, CURLINFO_RESPONSE_CODE, &(ctx->response->status_code));
        curl_easy_getinfo(ctx->handle, CURLINFO_EFFECTIVE_URL, &ctx->response->url);
    }
}
/**
 * @brief Internal function to construct parameters
 * @param ctx Context returned by creq_init()
 *
 * This is just an internal function to construct the GET/POST/HEAD parameters.
 * Users should not use this function.
 * This function has no return value.
 */
static void creq_construct_params(CREQ_CTX * ctx){

    if(ctx->request->params == NULL)
        return;
    CREQ_GET_PARAMS * tmp = ctx->request->params;
    char * param_string = NULL;
    char * param_tmp = NULL;
    char * key = NULL;
    char * val = NULL;
    size_t new_len = 0;
    size_t key_len, val_len, param_len;
    short int first_param = 1;
    while(tmp){
        param_len = (param_string == NULL?0:_str_len(param_string));
        if (strcmp(ctx->request->method, "GET") == 0){
            key = tmp->key != NULL?curl_easy_escape(ctx->handle, tmp->key, 0):NULL;
            val = curl_easy_escape(ctx->handle, tmp->value, 0);
        }else if (strcmp(ctx->request->method, "POST") == 0){
            key = tmp->key != NULL?strdup(tmp->key):NULL;
            val = strdup(tmp->value);
        }else if(strcmp(ctx->request->method, "HEAD") == 0){
            key = tmp->key!=NULL?curl_easy_escape(ctx->handle, tmp->key, 0):NULL;
            val = curl_easy_escape(ctx->handle, tmp->value, 0);
        }else{
            key = tmp->key!=NULL?curl_easy_escape(ctx->handle, tmp->key, 0):NULL;
            val = curl_easy_escape(ctx->handle, tmp->value, 0);
        }
        if (!val){
            ctx->err_code = CREQ_ERROR_CURL;
            return;
        }
        key_len = key==NULL?0:_str_len(key);
        val_len = _str_len(val);
        new_len = val_len + key_len + 3 + param_len;
        param_tmp = (char*) realloc(param_string,  new_len);
        if (!param_tmp){
            free(param_string);
            ctx->err_code = CREQ_ERROR_MEMORY_ALLOCATION;
            return;
        }
        param_string = param_tmp;
        // now concatenate params
        if (first_param){
            first_param = 0;
            strcpy(param_string, key==NULL?"":key);
            strcpy(param_string + key_len, key==NULL?"":"=");
            strcpy(param_string + key_len + (key==NULL?0:1), val);
            param_string[key_len + val_len + (key==NULL?0:1)] = '\0';
        }else{
            strcpy(param_string + param_len, "&");
            param_len += 1;
            strcpy(param_string + param_len, key==NULL?"":key);
            param_len += key_len;
            strcpy(param_string + param_len, key==NULL?"":"=");
            param_len += key==NULL?0:1;
            strcpy(param_string + param_len, val);
            param_len +=  val_len;
            param_string[param_len] = '\0';
        }
        free(key);
        free(val);
        tmp = tmp->next;
    }
    size_t final_len = param_string == NULL?0:_str_len(param_string);
    if (final_len == 0)
        return;
    if (final_len > (8 * 1024 * 102) - _str_len(ctx->request->url)){
        ctx->err_code = CREQ_ERROR_PARAMS_TOO_LONG;
        return;
    }
    if (strcmp(ctx->request->method, "GET") == 0 ||
        strcmp(ctx->request->method, "HEAD") == 0){
        char * new_url = (char*) malloc(_str_len(ctx->request->url) + _str_len(param_string) + 2);
        strcpy(new_url, ctx->request->url);
        new_url[_str_len(ctx->request->url)] = '?';
        strcpy(new_url + _str_len(ctx->request->url) + 1, param_string);
        new_url[_str_len(ctx->request->url) + _str_len(param_string) + 1] = '\0';
        free(param_string);
        free(ctx->request->url);
        ctx->request->url = new_url;
        return;
    }
    if (strcmp(ctx->request->method, "POST") == 0){
        if (final_len == 0){
            curl_easy_setopt(ctx->handle, CURLOPT_COPYPOSTFIELDS, "");
            return;
        }else{
            curl_easy_setopt(ctx->handle, CURLOPT_COPYPOSTFIELDS, param_string);
            free(param_string);
            return;
        }
    }
}

/**
 * @brief Returns the possible error message
 *
 * @param ctx Context returned by creq_init()
 *
 * Users should not free the returned pointer by this function.
 * The returned pointer is stack-based and should be consumed or copied
 * after calling this function.
 *
 * @return a pointer to the error message.
 */
char * creq_error(CREQ_CTX * ctx){
    if (ctx->err_code != CREQ_ERROR_SUCCESS){
        if (ctx->err_code == CREQ_ERROR_MEMORY_ALLOCATION)
            return "Can not allocate memory!";
        if (ctx->err_code == CREQ_ERROR_CURL)
            return (char *) curl_easy_strerror(ctx->curl_code);
    }
    return NULL;
}

/**
 * @brief Sets a GET/POST param.
 *
 * @param ctx Context returned by creq_init()
 * @param key Pointer to null-terminated string as the name of the parameter
 * @param value Pointer to the null-terminated string as the value of the parameter
 *
 * Sets a parameter for GET/POST request. The parameter must be text.
 * You CAN NOT use this method to set binary values. The function
 * calculates the length of the **key** and **value** parameters using
 * strlen() function. Binary data may have null characters which will result
 * in wrong calculation of the length.
 *
 * The key and value of each param are copied internally. So the caller must free() them (if it's necessary) after the call.
 *
 * In case your parameter does not have a key (you want to send just a value), set the key to NULL.
 *
 * 'value' can never be NULL.
 *
 * This function has no return value.
 *
 * @code
 * int main(){
 *     CREQ_CTX * ctx = creq_init();
 *     if (!ctx){
 *         fprintf(stderr, "Can not init the library...\n");
 *         return 1;
 *     }
 *     char url[] = "https://httpbin.org/get"
 *     // this will be the same as: https://httpbin.org/get?name=John+Doe
 *     creq_add_param(ctx, "name", "John Doe");
 *     creq_set_timeout(ctx, 10);
 *     creq_get(ctx, url);
 *     if (ctx->err_code != CREQ_ERROR_SUCCESS){
 *         fprintf(stderr, "Error(%d:%d): %s\n", ctx->err_code, ctx->curl_code, creq_error(ctx));
 *         creq_close(ctx);
 *         return 1;
 *     }
 *     // now that everything is good, we can print stuff
 *     // print http status code
 *     fprintf(stdout, "Status Code: %d\n", ctx->response->status_code);
 *     //  print my ip address
 *     fprintf(stdout, "My IP address is: %s\n", creq_get_content(ctx));
 *     creq_close(ctx);
 *     return 0;
 * }
 * 
 * @endcode
 */
void creq_add_param(CREQ_CTX * ctx, char * key, char * value){
    CREQ_GET_PARAMS * p = (CREQ_GET_PARAMS*) malloc(sizeof(CREQ_GET_PARAMS));
    if (!p){
        ctx->err_code = CREQ_ERROR_SETTING_GET_PARAMS;
        return;
    }
    if (NULL == key)
        p->key = NULL;
    else
        p->key = strdup(key);
    p->value = strdup(value);
    p->next = NULL;
    CREQ_GET_PARAMS * tmp = ctx->request->params;
    if (tmp == NULL){   // this is the first param
        ctx->request->params = p;
        return;
    }else{
        while (tmp->next)
            tmp = tmp->next;
        tmp->next = p;
        return;
    }
}

/**
 * @brief Returns the content of the request
 * @param ctx Context returned by creq_init()
 *
 * This function returns a pointer to the place where the response body is stored.
 *
 * You can use this method if you are sure the content does not have any null character
 * in it (e.g., the content is some HTML data or like that).
 *
 * If you know that the content is some binary data (e.g., a dowloaded binary file),
 * Then you should use ctx->response->mem and ctx->response->len and then iterate over the 
 * content. Therefore, I wouldn't use this method unless I am 100% sure the result is text.
 * Look at the example for more information.
 *
 * @code
 * int main(){
 *     CREQ_CTX * ctx = creq_init();
 *     if (!ctx){
 *         fprintf(stderr, "Can not init the context\n");
 *         return 1;
 *     }
 *     // let's download a binary file (Internet Download Manager :-D )
 *     char url[] = "https://mirror2.internetdownloadmanager.com/idman641build11.exe?v=lt&filename=idman641build11.exe";
 *     creq_get(ctx, url);
 *     if (ctx->err_code != CREQ_ERROR_SUCCESS){
 *         fprintf(stderr, "ERROR(%d:%d): %s\n", ctx->err_code, ctx->curl_code, creq_error(ctx));
 *         return 2;
 *     }
 *     // store the result in a file called idm.exe
 *     FILE * fp = fopen("idm.exe", "wb");
 *     if (!fp){
 *         fprintf(stderr, "Can not open a file to write.....\n");
 *         return 3;
 *     }
 *     // we should not use creq_get_content() method as we have binary content
 *     // instead, we use ctx->response->len and ctx->response->mem
 *     for (unsigned int i=0; i<ctx->response->len; ++i){
 *         fputc(ctx->response->mem[i], fp);
 *     }
 *     fclose(fp);
 *     return 0;
 * }
 * @endcode
 *
 *
 * @return A pointer to the memory related to the content of the request
 */
char * creq_get_content(CREQ_CTX * ctx){
    return ctx->response->mem;
}

/**
 * @brief Internal function for stripping the line
 *
 */
static char * _creq_strip_line(char * line){
    char seq[] = " \t\n\r\x0b\x0c";  // all whitespace
    unsigned int i = 0;
    const char * tmp = line;
    char ch;
    unsigned int found = 0;
    while (*tmp != '\0'){
        while((ch = seq[i++]) != '\0'){
            if (ch == *tmp){
                tmp += 1;
                i = 0;
                found = 1;
                break;
            }
            found=0;
        }
        if (!found)
            break;
    }
    //now tmp is the pointer to the new string
    char * new_str = (char *)malloc(sizeof(char) * _str_len(tmp) + 1);
    if (NULL == new_str){
        return NULL;
    }
    strcpy(new_str, tmp);
    return new_str;
}

/**
 * @brief Another internal function to strip the line
 *
 */
static char * creq_strip_line(char * line){
    //strips a line both from left and right side
    //first pass the string
    char * lstrip = _creq_strip_line(line);
    //reverse the string
    size_t len = _str_len(lstrip);
    char * reverse_str = (char *) malloc(len + 1);
    if (NULL == reverse_str){
        free(lstrip);
        return NULL;
    }
    for (unsigned int i=len;i>0; --i)
        reverse_str[len - i] = lstrip[i-1];
    reverse_str[len] = '\0';
    free(lstrip);
    char * rstrip = _creq_strip_line(reverse_str);
    free(reverse_str);
    if (NULL == rstrip)
        return NULL;
    // reverse it again
    len = _str_len(rstrip);
    reverse_str = (char *) malloc(len + 1);
    if (NULL == reverse_str){
        free(rstrip);
        return NULL;
    }
    for (unsigned int i=len;i>0; --i)
        reverse_str[len - i] = rstrip[i-1];
    reverse_str[len] = '\0';
    free(rstrip);
    return reverse_str;
}

/**
 * Internal function to be used instead of strlen()
 *
 */
static size_t _str_len(const char * str){
    char * tmp = (char*)str;
    for (;*tmp; ++tmp);
    return tmp - str;
}

/**
 * @brief Returns the value for a given header name
 * @param ctx Context returned by creq_init()
 * @param key A pointer to the null-terminated string specifying the header name.
 *
 * HTTP headers are case-insensitive. So no matter what you send as a key to this
 * function, it will use strcasecmp() function to compare it to the list of headers
 * and returns the value if there is a match. Otherwise, it returns NULL.
 *
 * @return A pointer to a null-terminated string as the value of a given header or NULL
 */
char * creq_get_response_header(CREQ_CTX * ctx, char * key){
    // returns the header given the key
    if (!key)
        return NULL;
    if (ctx->response->headers == NULL)
        return NULL;
    CREQ_HEADER * tmp = ctx->response->headers;
    while(tmp){
        if (strcasecmp(tmp->key, key) == 0){
            // we got it
            return strdup(tmp->value);
        }
        tmp = tmp->next;
    }
    return NULL;
}

