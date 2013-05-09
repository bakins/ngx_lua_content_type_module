#include <ngx_core.h>
#include <ngx_http.h>
#include "ngx_http_lua_api.h"

ngx_module_t ngx_lua_content_type_module;

static ngx_int_t ngx_lua_content_type_module_init(ngx_conf_t *cf);

static ngx_http_module_t ngx_lua_content_type_module_ctx = {
    NULL,                             /* preconfiguration */
    ngx_lua_content_type_module_init, /* postconfiguration */
    NULL,                             /* create main configuration */
    NULL,                             /* init main configuration */
    NULL,                             /* create server configuration */
    NULL,                             /* merge server configuration */
    NULL,                             /* create location configuration */
    NULL                              /* merge location configuration */
};


ngx_module_t ngx_lua_content_type_module = {
    NGX_MODULE_V1,
    &ngx_lua_content_type_module_ctx,  /* module context */
    NULL,                       /* module directives */
    NGX_HTTP_MODULE,            /* module type */
    NULL,                       /* init master */
    NULL,                       /* init module */
    NULL,                       /* init process */
    NULL,                       /* init thread */
    NULL,                       /* exit thread */
    NULL,                       /* exit process */
    NULL,                       /* exit master */
    NGX_MODULE_V1_PADDING
};

/* based on ngx_http_set_exten in ngx_http_core_module */
void set_exten(ngx_str_t *uri, ngx_str_t *exten)
{
    ngx_int_t  i;

    ngx_str_null(exten);

    for (i = uri->len - 1; i > 1; i--) {
        if (uri->data[i] == '.' && uri->data[i - 1] != '/') {
	    exten->len = uri->len - i - 1;
	    exten->data = uri->data[i + 1];
            return;

        } else if (uri->data[i] == '/') {
            return;
        }
    }

    return;
}

static int
ngx_lua_content_type(lua_State * L)
{
    ngx_http_request_t  *r;
    ngx_http_core_loc_conf_t  *clcf;
    ngx_str_t type, uri, exten;
    
    uri.data = luaL_checklstring(L, 1, &uri.len);

    if (NULL == uri.data) {
	return luaL_argerror(L, 1, "expecting string argument");
    }
    
    r = ngx_http_lua_get_request(L);
    
    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    set_exten(&uri, &exten);

    ngx_str_null(type);
    
    if(exten.len) {
	ngx_uint_t i, hash;
	u_char c;
	char *found = NULL;
	hash = 0;
	
	for (i = 0; i < exten.len; i++) {
	    c = exten.data[i];
		
	    if (c >= 'A' && c <= 'Z') {
		char *tmp;
		tmp = ngx_pnalloc(r->pool, exten_len);
		if (tmp == NULL) {
		    return luaL_error(L, "failed to allocate memory");
		}
		
		hash = ngx_hash_strlow(tmp, exten.data, exten.len);
		exten.data = tmp
		    break;
	    }
	    
	    hash = ngx_hash(hash, c);
	}
	    
	found = ngx_hash_find(&clcf->types_hash, hash,
			      exten.data, exten.len);
	
	if (found) {
	    type.len = found->len;
	    type.data = found->data;
	}
    }

    if(!type.len) {
	type.len = clcf->default_type.len;
	type.data = clcf->default_type.data;
    }
    
    lua_pushlstring(L, type.data, type.len);
    return 1;
}


static int luaopen_content_type(lua_State * L)
{
    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, ngx_lua_content_type);
    lua_setfield(L, -2, "get_content_type");
    return 1;
}


static ngx_int_t
ngx_lua_content_type_module_init(ngx_conf_t *cf)
{
    if (ngx_http_lua_add_package_preload(cf, "nginx.content_type",
                                         luaopen_content_type)
        != NGX_OK)
	{
	    return NGX_ERROR;
	}

    return NGX_OK;
}
