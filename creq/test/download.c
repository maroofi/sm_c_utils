#include <stdio.h>
#include <string.h>
 
#include "crequests.h"
 

 int main(){
     CREQ_CTX * ctx = creq_init();
     if (!ctx){
         fprintf(stderr, "Can not init the context\n");
         return 1;
     }
     // let's download a binary file (Internet Download Manager :-D )
     char url[] = "https://mirror2.internetdownloadmanager.com/idman641build11.exe";
     creq_add_param(ctx, "v", "lt");
     creq_add_param(ctx, "filename", "idman641build11.exe");
     creq_get(ctx, url);
     if (ctx->err_code != CREQ_ERROR_SUCCESS){
         fprintf(stderr, "ERROR(%d:%d): %s\n", ctx->err_code, ctx->curl_code, creq_error(ctx));
         return 2;
     }
     // store the result in a file called idm.exe
     FILE * fp = fopen("idm.exe", "wb");
     if (!fp){
         fprintf(stderr, "Can not open a file to write.....\n");
         return 3;
     }
     // we should not use creq_get_content() method as we have binary content
     // instead, we use ctx->response->len and ctx->response->mem
     for (unsigned int i=0; i<ctx->response->len; ++i){
         fputc(ctx->response->mem[i], fp);
     }
     fclose(fp);
     return 0;
 }

