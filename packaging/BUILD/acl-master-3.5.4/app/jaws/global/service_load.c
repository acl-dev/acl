#include "lib_acl.h"
#include "service.h"

typedef struct {
	ACL_DLL_HANDLE handle;
} DLL_HANDLE;

static ACL_FIFO *__handles = NULL;

void service_load(ACL_FIFO *service_modules, const char *dlname)
{
	const char *myname = "service_load";
	DLL_HANDLE *dll_handle;
	ACL_DLL_HANDLE handle;
	MODULE_SERVICE *module_ptr, module;

	if (dlname == NULL || *dlname == 0)
		return;

	acl_msg_info("%s(%d): begin load %s now ...", __FUNCTION__, __LINE__, dlname);
	handle = acl_dlopen(dlname);
	if (handle == NULL) {
		acl_msg_error("%s(%d): load %s error(%s)",
			myname, __LINE__, dlname, acl_last_serror());
		return;
	}

	acl_msg_info("%s(%d): load %s ok.", __FUNCTION__, __LINE__, dlname);

	/* Ìí¼ÓÇëÇó¹ýÂËÆ÷ */

	module.mod_init = (module_service_init_fn) (intptr_t)
		acl_dlsym(handle, "module_service_init");
	module.mod_create = (module_service_create_fn) (intptr_t)
		acl_dlsym(handle, "module_service_create");
	module.mod_main = (module_service_main_fn) (intptr_t)
		acl_dlsym(handle, "module_service_main");
	if (module.mod_create && module.mod_main) {
		module_ptr = (MODULE_SERVICE*) acl_mycalloc(1, sizeof(MODULE_SERVICE));
		module_ptr->mod_init = module.mod_init;
		module_ptr->mod_create = module.mod_create;
		module_ptr->mod_main = module.mod_main;
		acl_fifo_push(service_modules, module_ptr);
	}

	dll_handle = (DLL_HANDLE*) acl_mycalloc(1, sizeof(DLL_HANDLE));
	dll_handle->handle = handle;
	if (__handles == NULL)
		__handles = acl_fifo_new();
	acl_fifo_push(__handles, dll_handle);
}

void service_load_all(ACL_FIFO *service_modules, const char *dlnames)
{
	ACL_ARGV *argv;
	ACL_ITER iter;

	if (dlnames == NULL || *dlnames == 0)
		return;

	argv = acl_argv_split(dlnames, " \t;");
	acl_foreach(iter, argv) {
		const char *dlname = (const char*) iter.data;

		service_load(service_modules, dlname);
	}

	acl_argv_free(argv);
}

void service_unload_all()
{
	ACL_ITER iter;

	if (__handles == NULL)
		return;

	acl_foreach(iter, __handles) {
		DLL_HANDLE *handle = (DLL_HANDLE*) iter.data;
		acl_dlclose(handle->handle);
		acl_myfree(handle);
	}

	acl_fifo_free(__handles, NULL);
	__handles = NULL;
}
