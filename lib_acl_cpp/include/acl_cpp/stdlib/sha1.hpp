/*
 *  sha1.h
 *
 *  Copyright (C) 1998, 2009
 *  Paul E. Jones <paulej@packetizer.com>
 *  All Rights Reserved.
 *
 *****************************************************************************
 *  $Id: sha1.h 12 2009-06-22 19:34:25Z paulej $
 *****************************************************************************
 *
 *  Description:
 *      This class implements the Secure Hashing Standard as defined
 *      in FIPS PUB 180-1 published April 17, 1995.
 *
 *      Many of the variable names in this class, especially the single
 *      character names, were used because those were the names used
 *      in the publication.
 *
 *      Please read the file sha1.cpp for more information.
 *
 */

#pragma once

namespace acl
{

class sha1
{
public:
	sha1();
	virtual ~sha1();

	/*
	 *  Re-initialize the class
	 */
	void reset();

	/*
	 *  Returns the message digest
	 */
	bool result(unsigned *message_digest_array);

	/*
	 *  Provide input to SHA1
	 */
	void input(const unsigned char *message_array, unsigned length);
	void input(const char *message_array, unsigned length);
	void input(unsigned char message_element);
	void input(char message_element);
	sha1& operator<<(const char *message_array);
	sha1& operator<<(const unsigned char *message_array);
	sha1& operator<<(const char message_element);
	sha1& operator<<(const unsigned char message_element);

private:
	/*
	 *  Process the next 512 bits of the message
	 */
	void process_message_block();

	/*
	 *  Pads the current message block to 512 bits
	 */
	void pad_message();

	/*
	 *  Performs a circular left shift operation
	 */
	inline unsigned circular_shift(int bits, unsigned word);

	unsigned h_[5];                     // Message digest buffers

	unsigned length_low_;               // Message length in bits
	unsigned length_high_;              // Message length in bits

	unsigned char message_block_[64];   // 512-bit message blocks
	int message_block_index_;           // Index into message block array

	bool computed_;                     // Is the digest computed?
	bool corrupted_;                    // Is the message digest corruped?
};

} // namespace acl
