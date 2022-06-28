#pragma once

struct FILE_FROM_TO 
{
	const char* from;
	const char* to;
};

bool file_copy(const char* from, const char* to);
bool files_copy(const char* name, const FILE_FROM_TO* tab,
	const char* from_path, const char* to_path);
