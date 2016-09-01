/*
 *  sha1.cpp
 *
 *  Copyright (C) 1998, 2009
 *  Paul E. Jones <paulej@packetizer.com>
 *  All Rights Reserved.
 *
 *****************************************************************************
 *  $Id: sha1.cpp 12 2009-06-22 19:34:25Z paulej $
 *****************************************************************************
 *
 *  Description:
 *      This class implements the Secure Hashing Standard as defined
 *      in FIPS PUB 180-1 published April 17, 1995.
 *
 *      The Secure Hashing Standard, which uses the Secure Hashing
 *      Algorithm (SHA), produces a 160-bit message digest for a
 *      given data stream.  In theory, it is highly improbable that
 *      two messages will produce the same message digest.  Therefore,
 *      this algorithm can serve as a means of providing a "fingerprint"
 *      for a message.
 *
 *  Portability Issues:
 *      SHA-1 is defined in terms of 32-bit "words".  This code was
 *      written with the expectation that the processor has at least
 *      a 32-bit machine word size.  If the machine word size is larger,
 *      the code should still function properly.  One caveat to that
 *      is that the input functions taking characters and character arrays
 *      assume that only 8 bits of information are stored in each character.
 *
 *  Caveats:
 *      SHA-1 is designed to work with messages less than 2^64 bits long.
 *      Although SHA-1 allows a message digest to be generated for
 *      messages of any number of bits less than 2^64, this implementation
 *      only works with messages with a length that is a multiple of 8
 *      bits.
 *
 */

#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/sha1.hpp"
#endif

namespace acl
{

/*  
 *  sha1
 *
 *  Description:
 *      This is the constructor for the sha1 class.
 *
 *  Parameters:
 *      None.
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *
 */
sha1::sha1()
{
	reset();
}

/*  
 *  ~sha1
 *
 *  Description:
 *      This is the destructor for the sha1 class
 *
 *  Parameters:
 *      None.
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *
 */
sha1::~sha1()
{
	// The destructor does nothing
}

/*  
 *  reset
 *
 *  Description:
 *      This function will initialize the sha1 class member variables
 *      in preparation for computing a new message digest.
 *
 *  Parameters:
 *      None.
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *
 */
void sha1::reset()
{
	length_low_          = 0;
	length_high_         = 0;
	message_block_index_ = 0;

	h_[0]      = 0x67452301;
	h_[1]      = 0xEFCDAB89;
	h_[2]      = 0x98BADCFE;
	h_[3]      = 0x10325476;
	h_[4]      = 0xC3D2E1F0;

	computed_  = false;
	corrupted_ = false;
}

/*  
 *  result
 *
 *  Description:
 *      This function will return the 160-bit message digest into the
 *      array provided.
 *
 *  Parameters:
 *      message_digest_array: [out]
 *          This is an array of five unsigned integers which will be filled
 *          with the message digest that has been computed.
 *
 *  Returns:
 *      True if successful, false if it failed.
 *
 *  Comments:
 *
 */
bool sha1::result(unsigned *message_digest_array)
{
	int i; // Counter

	if (corrupted_)
		return false;

	if (!computed_)
	{
		pad_message();
		computed_ = true;
	}

	for(i = 0; i < 5; i++)
		message_digest_array[i] = h_[i];

	return true;
}

/*  
 *  input
 *
 *  Description:
 *      This function accepts an array of octets as the next portion of
 *      the message.
 *
 *  Parameters:
 *      message_array: [in]
 *          An array of characters representing the next portion of the
 *          message.
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *
 */
void sha1::input(const unsigned char *message_array, unsigned length)
{
	if (!length)
		return;

	if (computed_ || corrupted_)
	{
		corrupted_ = true;
		return;
	}

	while (length-- && !corrupted_)
	{
		message_block_[message_block_index_++] = (*message_array & 0xFF);

		length_low_ += 8;
		length_low_ &= 0xFFFFFFFF;              // Force it to 32 bits
		if (length_low_ == 0)
		{
			length_high_++;
			length_high_ &= 0xFFFFFFFF;     // Force it to 32 bits
			if (length_high_ == 0)
				corrupted_ = true;      // Message is too long
		}

		if (message_block_index_ == 64)
			process_message_block();

		message_array++;
	}
}

/*  
 *  input
 *
 *  Description:
 *      This function accepts an array of octets as the next portion of
 *      the message.
 *
 *  Parameters:
 *      message_array: [in]
 *          An array of characters representing the next portion of the
 *          message.
 *      length: [in]
 *          The length of the message_array
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *
 */
void sha1::input(const char *message_array, unsigned length)
{
	input((const unsigned char *) message_array, length);
}

/*  
 *  input
 *
 *  Description:
 *      This function accepts a single octets as the next message element.
 *
 *  Parameters:
 *      message_element: [in]
 *          The next octet in the message.
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *
 */
void sha1::input(unsigned char message_element)
{
	input(&message_element, 1);
}

/*  
 *  input
 *
 *  Description:
 *      This function accepts a single octet as the next message element.
 *
 *  Parameters:
 *      message_element: [in]
 *          The next octet in the message.
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *
 */
void sha1::input(char message_element)
{
	input((unsigned char *) &message_element, 1);
}

/*  
 *  operator<<
 *
 *  Description:
 *      This operator makes it convenient to provide character strings to
 *      the SHA1 object for processing.
 *
 *  Parameters:
 *      message_array: [in]
 *          The character array to take as input.
 *
 *  Returns:
 *      A reference to the SHA1 object.
 *
 *  Comments:
 *      Each character is assumed to hold 8 bits of information.
 *
 */
sha1& sha1::operator<<(const char *message_array)
{
	const char *p = message_array;

	while(*p)
	{
		input(*p);
		p++;
	}

	return *this;
}

/*  
 *  operator<<
 *
 *  Description:
 *      This operator makes it convenient to provide character strings to
 *      the SHA1 object for processing.
 *
 *  Parameters:
 *      message_array: [in]
 *          The character array to take as input.
 *
 *  Returns:
 *      A reference to the SHA1 object.
 *
 *  Comments:
 *      Each character is assumed to hold 8 bits of information.
 *
 */
sha1& sha1::operator<<(const unsigned char *message_array)
{
	const unsigned char *p = message_array;

	while(*p)
	{
		input(*p);
		p++;
	}

	return *this;
}

/*  
 *  operator<<
 *
 *  Description:
 *      This function provides the next octet in the message.
 *
 *  Parameters:
 *      message_element: [in]
 *          The next octet in the message
 *
 *  Returns:
 *      A reference to the SHA1 object.
 *
 *  Comments:
 *      The character is assumed to hold 8 bits of information.
 *
 */
sha1& sha1::operator<<(const char message_element)
{
	input((const unsigned char *) &message_element, 1);

	return *this;
}

/*  
 *  operator<<
 *
 *  Description:
 *      This function provides the next octet in the message.
 *
 *  Parameters:
 *      message_element: [in]
 *          The next octet in the message
 *
 *  Returns:
 *      A reference to the SHA1 object.
 *
 *  Comments:
 *      The character is assumed to hold 8 bits of information.
 *
 */
sha1& sha1::operator<<(const unsigned char message_element)
{
	input(&message_element, 1);

	return *this;
}

/*  
 *  process_message_block
 *
 *  Description:
 *      This function will process the next 512 bits of the message
 *      stored in the Message_Block array.
 *
 *  Parameters:
 *      None.
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *      Many of the variable names in this function, especially the single
 *      character names, were used because those were the names used
 *      in the publication.
 *
 */
void sha1::process_message_block()
{
	const unsigned K[] =    {               // Constants defined for SHA-1
		0x5A827999,
		0x6ED9EBA1,
		0x8F1BBCDC,
		0xCA62C1D6
	};
	int         t;                          // Loop counter
	unsigned    temp;                       // Temporary word value
	unsigned    W[80];                      // Word sequence
	unsigned    A, B, C, D, E;              // Word buffers

	/*
	 *  Initialize the first 16 words in the array W
	 */
	for (t = 0; t < 16; t++)
	{
		W[t] = ((unsigned) message_block_[t * 4]) << 24;
		W[t] |= ((unsigned) message_block_[t * 4 + 1]) << 16;
		W[t] |= ((unsigned) message_block_[t * 4 + 2]) << 8;
		W[t] |= ((unsigned) message_block_[t * 4 + 3]);
	}

	for (t = 16; t < 80; t++)
		W[t] = circular_shift(1, W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);

	A = h_[0];
	B = h_[1];
	C = h_[2];
	D = h_[3];
	E = h_[4];

	for (t = 0; t < 20; t++)
	{
		temp = circular_shift(5, A) + ((B & C) | ((~B) & D))
			+ E + W[t] + K[0];
		temp &= 0xFFFFFFFF;
		E = D;
		D = C;
		C = circular_shift(30, B);
		B = A;
		A = temp;
	}

	for (t = 20; t < 40; t++)
	{
		temp = circular_shift(5, A) + (B ^ C ^ D) + E + W[t] + K[1];
		temp &= 0xFFFFFFFF;
		E = D;
		D = C;
		C = circular_shift(30, B);
		B = A;
		A = temp;
	}

	for (t = 40; t < 60; t++)
	{
		temp = circular_shift(5, A) +
			((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
		temp &= 0xFFFFFFFF;
		E = D;
		D = C;
		C = circular_shift(30, B);
		B = A;
		A = temp;
	}

	for (t = 60; t < 80; t++)
	{
		temp = circular_shift(5, A) + (B ^ C ^ D) + E + W[t] + K[3];
		temp &= 0xFFFFFFFF;
		E = D;
		D = C;
		C = circular_shift(30, B);
		B = A;
		A = temp;
	}

	h_[0] = (h_[0] + A) & 0xFFFFFFFF;
	h_[1] = (h_[1] + B) & 0xFFFFFFFF;
	h_[2] = (h_[2] + C) & 0xFFFFFFFF;
	h_[3] = (h_[3] + D) & 0xFFFFFFFF;
	h_[4] = (h_[4] + E) & 0xFFFFFFFF;

	message_block_index_ = 0;
}

/*  
 *  pad_message
 *
 *  Description:
 *      According to the standard, the message must be padded to an even
 *      512 bits.  The first padding bit must be a '1'.  The last 64 bits
 *      represent the length of the original message.  All bits in between
 *      should be 0.  This function will pad the message according to those
 *      rules by filling the message_block array accordingly.  It will also
 *      call ProcessMessageBlock() appropriately.  When it returns, it
 *      can be assumed that the message digest has been computed.
 *
 *  Parameters:
 *      None.
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *
 */
void sha1::pad_message()
{
	/*
	 *  Check to see if the current message block is too small to hold
	 *  the initial padding bits and length.  If so, we will pad the
	 *  block, process it, and then continue padding into a second block.
	 */
	if (message_block_index_ > 55)
	{
		message_block_[message_block_index_++] = 0x80;
		while (message_block_index_ < 64)
			message_block_[message_block_index_++] = 0;

		process_message_block();

		while (message_block_index_ < 56)
			message_block_[message_block_index_++] = 0;
	}
	else
	{
		message_block_[message_block_index_++] = 0x80;
		while (message_block_index_ < 56)
			message_block_[message_block_index_++] = 0;
	}

	/*
	 *  Store the message length as the last 8 octets
	 */
	message_block_[56] = (length_high_ >> 24) & 0xFF;
	message_block_[57] = (length_high_ >> 16) & 0xFF;
	message_block_[58] = (length_high_ >> 8) & 0xFF;
	message_block_[59] = (length_high_) & 0xFF;
	message_block_[60] = (length_low_ >> 24) & 0xFF;
	message_block_[61] = (length_low_ >> 16) & 0xFF;
	message_block_[62] = (length_low_ >> 8) & 0xFF;
	message_block_[63] = (length_low_) & 0xFF;

	process_message_block();
}

/*  
 *  circular_shift
 *
 *  Description:
 *      This member function will perform a circular shifting operation.
 *
 *  Parameters:
 *      bits: [in]
 *          The number of bits to shift (1-31)
 *      word: [in]
 *          The value to shift (assumes a 32-bit integer)
 *
 *  Returns:
 *      The shifted value.
 *
 *  Comments:
 *
 */
unsigned sha1::circular_shift(int bits, unsigned word)
{
	return ((word << bits) & 0xFFFFFFFF)
		| ((word & 0xFFFFFFFF) >> (32 - bits));
}

} // namespace acl
