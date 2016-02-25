#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/mime/mime_xxcode.hpp"
#endif

namespace acl {

static const unsigned char to_xx_tab[] =
	"+-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

static const unsigned char un_xx_tab[] = {
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0,   255, 1, 255, 255,
	2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  255, 255, 255, 255, 255, 255,
	255, 12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,
	27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  255, 255, 255, 255, 255,
	255, 38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,
	53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};

mime_xxcode::mime_xxcode(bool addCrlf /* = false */, bool addInvalid /* = false */)
	: mime_code(addCrlf, addInvalid, "xxcode")
{
	init(to_xx_tab, un_xx_tab, '~');
}

mime_xxcode::~mime_xxcode()
{
}

void mime_xxcode::encode(const char* in, int n, acl::string* out)
{
	mime_xxcode coder(false, false);
	coder.encode_update(in, n, out);
	coder.encode_finish(out);
}

void mime_xxcode::decode(const char* in, int n, acl::string* out)
{
	mime_xxcode decoder(false, false);
	decoder.decode_update(in, n, out);
	decoder.decode_finish(out);
}

} // namespace acl
