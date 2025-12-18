#ifndef	ACL_CFG_MACRO_INCLUDE_H
#define	ACL_CFG_MACRO_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "acl_define.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "acl_msg.h"
#include "acl_xinetd_cfg.h"

/**
 * Get a certain configuration item's string value from configuration object.
 * @param _xcp_ {ACL_XINETD_CFG_PARSER*} Configuration object
 * @param _name_ {const char*} Configuration item name
 * @param _obj_ {char*} Storage address for configuration item's string type value
 */
#define ACL_CFG_SET_ITEM_STR(_xcp_, _name_, _obj_) do {  \
	ACL_XINETD_CFG_PARSER *_tmp_xcp_ = _xcp_; \
	const char *_ptr_;  \
	_ptr_ = acl_xinetd_cfg_get(_tmp_xcp_, _name_);  \
	if (_ptr_ && *_ptr_) {  \
		_obj_ = acl_mystrdup(_ptr_);  \
		if (_obj_ == NULL)  \
			acl_msg_fatal("%s(%d): acl_mystrdup error=%s for %s",  \
					__FILE__, __LINE__, \
					strerror(errno), _ptr_);  \
	}  \
} while (0);

/**
 * Get a certain configuration item's integer value from configuration object.
 * @param _xcp_ {ACL_XINETD_CFG_PARSER*} Configuration object
 * @param _name_ {const char*} Configuration item name
 * @param _obj_ {int} Storage address for configuration item's integer type value
 * @param _def_ {int} If configuration item does not exist, use this default value
 */
#define ACL_CFG_SET_ITEM_INT(_xcp_, _name_, _obj_, _def_) do {  \
	ACL_XINETD_CFG_PARSER *_tmp_xcp_ = _xcp_; \
	const char *_ptr_;  \
	_ptr_ = acl_xinetd_cfg_get(_tmp_xcp_, _name_);  \
	if (_ptr_ && *_ptr_) {  \
		_obj_ = atoi(_ptr_);  \
		if (_obj_ <= 0)  \
			_obj_ = _def_;  \
	} else  \
		_obj_ = _def_;  \
} while (0);

#ifdef	__cplusplus
}
#endif

#endif
