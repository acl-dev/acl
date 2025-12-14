#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"

namespace acl {

/**
 * Stream object IO registration callback class. Subclasses need to implement
 * virtual methods in this class. Subclass objects are registered through
 * setup_hook,
 * then the IO process in this subclass object is used as the underlying IO
 * process of stream/aio_stream class objects;
 * If stream/aio_stream's setup_hook registration process is not called, then
 * stream/aio_stream
 * class objects' underlying IO process is the default process
 * XXX: This pure virtual class is declared as a heap object class, it is
 * recommended that subclasses should also be declared as heap object classes
 */
class ACL_CPP_API stream_hook : public noncopyable {
public:
	stream_hook() {}

	/**
	 * Read data interface
	 * @param buf {void*} Read buffer address, read data will be stored in this
	 * buffer
	 * @param len {size_t} buf buffer size
	 * @return {int} Number of bytes read. When return value < 0, it indicates
	 * error
	 */
	virtual int read(void* buf, size_t len) = 0;

	/**
	 * Send data interface
	 * @param buf {const void*} Send buffer address
	 * @param len {size_t} Data length in buf buffer (must > 0)
	 * @return {int} Length of data written. Return value < 0 indicates error
	 */
	virtual int send(const void* buf, size_t len) = 0;

	/**
	 * stream/aio_stream's setup_hook internally will call stream_hook::open
	 * process, so that subclass objects can use it to initialize some data and
	 * sessions
	 * @param s {ACL_VSTREAM*} The stream object created in setup_hook internal
	 * call to this method
	 *  is passed as parameter
	 * @return {bool} If subclass instance returns false, setup_hook call fails and
	 * will restore to original state
	 */
	virtual bool open(ACL_VSTREAM* s) = 0;

	/**
	 * When stream/aio_stream stream object is about to close, this function will
	 * be called back so that subclass instances can do cleanup work
	 * @param alive {bool} Whether the connection is still normal
	 * @return {bool}
	 */
	virtual bool on_close(bool alive) { (void) alive; return true; }

	/**
	 * Called when stream/aio_stream object needs to release stream_hook subclass
	 * object
	 */
	virtual void destroy() {}

protected:
	virtual ~stream_hook() {}
};

} // namespace acl

