Easy to use library similar to Python request for HTTP(S) requests.

Only supports GET, POST and HEAD.

### Features

(All is just a wrapper over cURL library just made it easy for my daily jobs).

1. Get the body of the requests
2. Get response headers
3. Use proxy for sending request
4. Enable/Disable SSL verification.
5. Setting timeout for each request.
6. Set parameters for requests.
7. Sets User-agent
8. Verbose output
9. Gets the redirection chain


### Documentation

```bash
doxygen Doxyfile
```

#### Compile

```c
# in the root directory
gcc -O2 -fPIC -shared -o libcrequests.so  ./src/crequests.c ./src/lua_crequests.c -I./include -I/usr/include/lua5.4 -llua5.4 -lm -lcurl
```


### Example

Here is the full example of how to use the library

You can also check other examples in `test` directory.

```c
#include "crequests.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * This example shows almost all the functions and their
 * use cases.
 *
 * Make sure you compile it with -lcurl
 */

int main(int argc, char ** argv){
    // We must globally init curl somewhere
    curl_global_init(CURL_GLOBAL_ALL);
    
    // let's make a new context to use the library
    CREQ_CTX * ctx = creq_init();
    // We can not continue if the context is NULL
    if (!ctx){
        fprintf(stderr, "Can not create context for request\n");
        return 1;
    }

    // let's set a browser useragent (chrome - Ubuntu)
    creq_set_useragent(ctx, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
                             "(KHTML, like Gecko) Chrome/112.0.0.0 Safari/537.36");
    
    // let's add one parameter to our request
    // We don't need to URL-encode the parameter as the library will do it
    creq_add_param(ctx, "name", "John Doe");
    
    // let's see if all is good?
    if (ctx->err_code != CREQ_ERROR_SUCCESS){
        fprintf(stderr, "ERROR(%d): %s\n", ctx->err_code, creq_error(ctx));
        return 1;
    }
    // Let's add a token to our get request
    creq_add_header(ctx, "Authorization: Bearer this_is_the_token");

    // We also want to use our localhost socks5 proxy to connect to the server
    //creq_set_proxy(ctx, "socks5h://127.0.0.1:1031");
    
    // We want our request to take at most 10 seconds
    creq_set_timeout(ctx, 10);

    // And we don't care if our request is not safe
    creq_set_verify(ctx, 0);
    
    char url[] = "http://ipinfo.io/ip";

    // let's send our get request
    creq_get(ctx, url);
    if (ctx->err_code != CREQ_ERROR_SUCCESS){
        fprintf(stderr, "ERROR(%d): %s\n", ctx->err_code, creq_error(ctx));
        creq_close(ctx);
        curl_global_cleanup();
        return 1;
    }
    // let's print the HTTP status code for our GET request
    fprintf(stdout, "Status_code: %ld\n", ctx->response->status_code);
    
    // What is the destination URL? (perhaps we had some redirections)
    fprintf(stdout, "Final URL: %s\n", ctx->response->url);
    
    // Let's print the length of the content
    fprintf(stdout, "Content len: %ld\n",
            ctx->response->mem?strlen(ctx->response->mem):-1);
    
    // Print the content-length header value
    char * content_len = creq_get_response_header(ctx, "content-length");
    fprintf(stdout, "Content len(from header): %s\n", content_len);
    free(content_len);

    // here is the output.
    fprintf(stdout, "******Output********\n");
    fprintf(stdout, "%s\n", creq_get_content(ctx));
    fprintf(stdout, "********************\n");

    // List of the headers we received as response
    fprintf(stdout, "*********headers**********\n");
    CREQ_HEADER * tmp = ctx->response->headers;
    while (tmp){
        fprintf(stdout, "'%s' -> '%s'\n", tmp->key, tmp->value);
        tmp = tmp->next;
    }
    fprintf(stdout, "**************************\n");
    
    // Print the whole redirection chain
    // We will just see the original URL since there is no redirection header
    fprintf(stdout, "*********redirection**********\n");
    CREQ_REDIRECTION_CHAIN * r = ctx->response->follow_chain;
    while (r){
        fprintf(stdout, "'%s'\n", r->url);
        r = r->next;
    }
    fprintf(stdout, "**************************\n");
    
    // close the context and release the memory.
    creq_close(ctx);

    // this must be done exactly one time before exit
    curl_global_cleanup();
    return 0;
}
```

