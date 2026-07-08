#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <crequests.h>

// compile with:
// gcc -fPIC -shared -o libcrequests.so  ./src/crequests.c ./src/lua_crequests.c -I./include -I/usr/include/lua5.4 -llua5.4 -lm -lcurl


typedef struct{
    char * url;
}req_param;

static void * malloc_or_abort(size_t n){
    void * p = malloc(n);
    if (NULL == p){
        fprintf(stderr, "Can not allocate %ld bytes of memory...aborting...\n", n);
        abort();
    }
    return p;
}

static char * mem_copy(char * data, unsigned long int len){
    if (data == NULL)
        return NULL;
    char * tmp = (char *) malloc_or_abort(len);
    memcpy(tmp, data, len);
    return tmp;
}


static char * safe_strdup(char * str){
    if (str == NULL)
        return NULL;
    return strdup(str);
}

int add_params(CREQ_CTX * ctx, char * err, lua_State * L){
    // add params table to the ctx one by one
    // returns 0 on success other values on failure and fill err message
    // we already removed "headers" key. header table is on top of the stack
    memset(err, 0x00, 255);
    char *key = NULL;
    char * value = NULL;
    // table is on top of the stack
    lua_pushnil(L);     // this is the first key
    while (lua_next(L, -2) != 0){
        // first check the key
        if (strcmp(luaL_typename(L, -2), "number") == 0){
            // we don't have key. the table's first element is an array
            key = NULL;
        }else{
            key = (char*)lua_tostring(L, -2);
        }
        value = (char*) lua_tostring(L, -1);
        if (value == NULL){
            strcpy(err, "the params value must be a string or a number");
            return 1;
        }
        creq_add_param(ctx, key, value);
        lua_pop(L, 1);
    }
    return 0;
}

int add_headers(CREQ_CTX * ctx, char * err, lua_State * L){
    // add params table to the ctx one by one
    // returns 0 on success other values on failure and fill err message
    // we already removed "headers" key. header table is on top of the stack
    memset(err, 0x00, 255);
    char *key = NULL;
    char * value = NULL;
    // table is on top of the stack
    lua_pushnil(L);     // this is the first key
    while (lua_next(L, -2) != 0){
        // first check the key
        if (lua_type(L, -2) != LUA_TSTRING){
            // all the keys must be string type
            strcpy(err, "all the headers key must be string");
            return 1;
        }
        // now check the value
        key = (char*)lua_tostring(L, -2);
        value = (char*) lua_tostring(L, -1);
        if (value == NULL){
            strcpy(err, "the header value must be a string or a number");
            return 1;
        }
        if (key == NULL){
            strcpy(err, "the header key must be a string");
            return 1;
        }
        size_t len = strlen(key) + 2 + strlen(value) + 1;
        char * h = malloc_or_abort(len);
        h[len-1] = '\0';
        memcpy(h, key, strlen(key));
        memcpy(h+strlen(key), ": ", 2);
        memcpy(h+strlen(key)+2, value, strlen(value));
        creq_add_header(ctx, h);
        free(h);
        lua_pop(L, 1);
    }
    return 0;
}

int config_context(CREQ_CTX * ctx, char * key, lua_State * L, char * err, req_param * rp){
    // we use L to fetch the appropriate value from the stack based on the key
    // we set all the necessary params of ctx based on the key-values provided
    // set URL
    if (strcmp(key, "url") == 0){
        if (lua_type(L, -1) != LUA_TSTRING){
            memset(err, 0x00, 255);
            strcpy(err, "The URL must be a string and valid");
            return 1;
        }
        const char * url = lua_tostring(L, -1);
        rp->url = safe_strdup((char*)url);
        if (rp->url == NULL){
            memset(err, 0x00, 255);
            strcpy(err, "Invalid URL");
            return 1;
        }
        return 0;
    }
    // set timeout
    if (strcmp(key, "timeout") == 0){
        if (lua_type(L, -1) != LUA_TNUMBER){
            memset(err, 0x00, 255);
            strcpy(err, "The timeout value must be a positive integer");
            return 1;
        }
        uint32_t timeout = (uint32_t)lua_tointeger(L, -1);
        if (timeout == 0){
            memset(err, 0x00, 255);
            strcpy(err, "The timeout value must be a positive integer > 0");
            return 1;
        }
        creq_set_timeout(ctx, timeout);
        return 0;
    }
    // verify
    if (strcmp(key, "verify") == 0){
        if (lua_type(L, -1) != LUA_TNUMBER){
            memset(err, 0x00, 255);
            strcpy(err, "The verify value must be zero(no ssl verification) or 1 (default)");
            return 1;
        }
        uint8_t verify = (uint8_t)lua_tointeger(L, -1);
        if (verify != 1 && verify != 0){
            memset(err, 0x00, 255);
            strcpy(err, "The verify value must be zero(no ssl verification) or 1 (default)");
            return 1;
        }
        creq_set_verify(ctx, verify);
        return 0;
    }
    // allow_redirection
    if (strcmp(key, "allow_redirection") == 0){
        if (lua_type(L, -1) != LUA_TNUMBER){
            memset(err, 0x00, 255);
            strcpy(err, "The 'allow_redirection' value must be zero(no redirection) or 1 (default)");
            return 1;
        }
        uint8_t ar = (uint8_t)lua_tointeger(L, -1);
        if (ar != 1 && ar != 0){
            memset(err, 0x00, 255);
            strcpy(err, "The 'allow_redirection' value must be zero(no redirection) or 1 (default)");
            return 1;
        }
        creq_set_allow_redirects(ctx, ar);
        return 0;
    }
    // proxy
    if (strcmp(key, "proxy") == 0){
        if (lua_type(L, -1) != LUA_TSTRING){
            memset(err, 0x00, 255);
            strcpy(err, "'proxy' must be in a form of <http|https|socks5|socks5h|socks4>://IP:PORT string");
            return 1;
        }
        const char * proxy = lua_tostring(L, -1);
        creq_set_proxy(ctx, (char*)proxy);
        return 0;
    }
    // headers
    if (strcmp(key, "headers") == 0){
        if (lua_type(L, -1) != LUA_TTABLE){
            memset(err, 0x00, 255);
            strcpy(err, "'headers' parameter must be a table with key-values");
            return 1;
        }
        // consume the header so that header table is on top of the stack
        const char * header_key = lua_tostring(L, -1);
        if (add_headers(ctx, err, L) != 0){
            // we got error but err is already set!
            return 1;
        }
        // we are good!
        return 0;
    }
    // params
    if (strcmp(key, "params") == 0){
        if (lua_type(L, -1) != LUA_TTABLE){
            memset(err, 0x00, 255);
            strcpy(err, "'params' parameter must be a table with key-values");
            return 1;
        }
        // consume the header so that header table is on top of the stack
        const char * params_key = lua_tostring(L, -1);
        if (add_params(ctx, err, L) != 0){
            // we got error but err is already set!
            return 1;
        }
        // we are good!
        return 0;
    }
    memset(err, 0x00, 255);
    strcpy(err, "Unknown parameter passed as input");
    return 2;
}


static void create_response_header_table(CREQ_CTX * ctx, lua_State *L){
    // we need to create a table on top of the stack
    // how many element we have?
    CREQ_HEADER * tmpheader = ctx->response->headers;
    int num_headers = 0;
    while(tmpheader){
        num_headers++;
        tmpheader = tmpheader->next;
    }
    lua_createtable(L, 0, num_headers);
    tmpheader = ctx->response->headers;
    while(tmpheader){
        lua_pushstring(L, tmpheader->key);
        lua_pushstring(L, tmpheader->value);
        lua_settable(L, -3);
        tmpheader = tmpheader->next;
    }
    return;
}


static int l_crequest_get(lua_State * L){
    req_param rp;
    rp.url = NULL;
    // the goal is to show how to get the key-value without knowing the key names
    // let's check if the first value on top of the stack is a table or not?
    luaL_checktype(L, -1, LUA_TTABLE);
    // create a context for request
    CREQ_CTX * ctx = creq_init();
    if (ctx == NULL){
        lua_pushnil(L);
        lua_pushstring(L, "Can not create a request context");
        return 2;
    }
    char err[255] = {0x00};
    char * key = NULL;
    // table is on top of the stack
    lua_pushnil(L);     // this is the first key
    while (lua_next(L, -2) != 0){
        // first check the key
        if (lua_type(L, -2) != LUA_TSTRING){
            // all the keys must be string type
            creq_close(ctx);
            lua_pushnil(L);
            lua_pushstring(L, "All the keys of the table must be string");
            return 2;
        }
        key = (char*)lua_tostring(L, -2);
        if (config_context(ctx, key, L, err, &rp) != 0){
            creq_close(ctx);
            lua_pushnil(L);
            lua_pushstring(L, err);
            return 2;
        }
        // remove value and keep the key
        lua_pop(L, 1);
    }
    creq_get(ctx, rp.url);
    if (ctx->err_code != CREQ_ERROR_SUCCESS){
        lua_pushnil(L);
        lua_pushinteger(L, ctx->err_code);
        lua_pushstring(L, creq_error(ctx));
        //lua_pushstring(L, safe_strdup(creq_error(ctx)));
        creq_close(ctx);
        free(rp.url);
        return 3;
    }
    // we need to create a table and pass everything in it
    lua_createtable(L, 0, 1);    // this is top to the stack (0 structure, 1 scalar)
    // status
    lua_pushstring(L, "status");
    lua_pushinteger(L, ctx->response->status_code);
    lua_settable(L, -3);
    // destination url
    lua_pushstring(L, "url");
    lua_pushstring(L, ctx->response->url);
    lua_settable(L, -3);
    // content
    lua_pushstring(L, "content");
    char * content = mem_copy(ctx->response->mem, ctx->response->len);
    if (NULL == content){
        lua_pushstring(L, "");
    }else{
        lua_pushlstring(L, content, ctx->response->len);
    }
    lua_settable(L, -3);
    free(content);
    // others?
    lua_pushstring(L, "headers");
    create_response_header_table(ctx, L);
    lua_settable(L, -3);
    creq_close(ctx);
    free(rp.url);
    return 1;
}

static int l_crequest_post(lua_State * L){
    req_param rp;
    rp.url = NULL;
    // the goal is to show how to get the key-value without knowing the key names
    // let's check if the first value on top of the stack is a table or not?
    luaL_checktype(L, -1, LUA_TTABLE);
    // create a context for request
    CREQ_CTX * ctx = creq_init();
    if (ctx == NULL){
        lua_pushnil(L);
        lua_pushstring(L, "Can not create a request context");
        return 2;
    }
    char err[255] = {0x00};
    char * key = NULL;
    // table is on top of the stack
    lua_pushnil(L);     // this is the first key
    while (lua_next(L, -2) != 0){
        // first check the key
        key = (char*)lua_tostring(L, -2);
        if (config_context(ctx, key, L, err, &rp) != 0){
            creq_close(ctx);
            lua_pushnil(L);
            lua_pushstring(L, err);
            return 2;
        }
        // remove value and keep the key
        lua_pop(L, 1);
    }
    creq_post(ctx, rp.url);
    if (ctx->err_code != CREQ_ERROR_SUCCESS){
        lua_pushnil(L);
        lua_pushinteger(L, ctx->err_code);
        lua_pushstring(L, creq_error(ctx));
        //lua_pushstring(L, safe_strdup(creq_error(ctx)));
        creq_close(ctx);
        free(rp.url);
        return 3;
    }
    // we need to create a table and pass everything in it
    lua_createtable(L, 0, 1);    // this is top to the stack (0 structure, 1 scalar)
    // status
    lua_pushstring(L, "status");
    lua_pushinteger(L, ctx->response->status_code);
    lua_settable(L, -3);
    // destination url
    lua_pushstring(L, "url");
    lua_pushstring(L, ctx->response->url);
    lua_settable(L, -3);
    // content
    lua_pushstring(L, "content");
    char * content = mem_copy(ctx->response->mem, ctx->response->len);
    if (NULL == content){
        lua_pushstring(L, "");
    }else{
        lua_pushlstring(L, content, ctx->response->len);
    }
    lua_settable(L, -3);
    free(content);
    // others?
    lua_pushstring(L, "headers");
    create_response_header_table(ctx, L);
    lua_settable(L, -3);
    creq_close(ctx);
    free(rp.url);
    return 1;
}
 


static const struct luaL_Reg crequests_lib_expose[] = {
    {"get", l_crequest_get},
    {"post", l_crequest_post},
    // TODO: ADD more function here
    {NULL, NULL}
};


int luaopen_libcrequests(lua_State * L){
    // initialize the metatable
    //lua_pop(L, 1);
    // This exists in lua version >= 5.2
    curl_global_init(CURL_GLOBAL_ALL);
    luaL_newlib(L, crequests_lib_expose);
    return 1;
}
