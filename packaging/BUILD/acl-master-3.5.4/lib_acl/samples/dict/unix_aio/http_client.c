#include "lib_acl.h"
#include "lib_protocol.h"
#include "http_service.h"

HTTP_CLIENT *http_client_new(ACL_ASTREAM *stream)
{
	HTTP_CLIENT *client;

	client = (HTTP_CLIENT*) acl_mycalloc(1, sizeof(HTTP_CLIENT));
	client->hdr_req = http_hdr_req_new();
	client->stream = stream;
	client->http_req = NULL;

	return (client);
}

void http_client_free(HTTP_CLIENT *client)
{
	if (client->hdr_req)
		http_hdr_req_free(client->hdr_req);
	if (client->http_req) {
		client->http_req->hdr_req = NULL;
		http_req_free(client->http_req);
	}

	if (client->key)
		acl_vstring_free(client->key);
	if (client->sbuf)
		acl_vstring_free(client->sbuf);
	acl_myfree(client);
}

void http_client_reset(HTTP_CLIENT *client)
{
	if (client->hdr_req) {
		TRACE();
		http_hdr_req_reset(client->hdr_req);
	}

	if (client->http_req) {
		TRACE();
		client->http_req->hdr_req = NULL;
		TRACE();
		http_req_free(client->http_req);
		TRACE();
		client->http_req = NULL;
	}
	if (client->key)
		ACL_VSTRING_RESET(client->key);
	if (client->sbuf)
		ACL_VSTRING_RESET(client->sbuf);
}
