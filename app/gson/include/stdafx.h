// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���ǳ��õ��������ĵ���Ŀ�ض��İ����ļ�
//

#pragma once


//#include <iostream>
//#include <tchar.h>

// TODO: �ڴ˴����ó���Ҫ��ĸ���ͷ�ļ�
#include <assert.h>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <string>
#include "acl_cpp/lib_acl.hpp"
#include "lib_acl.h"
#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd 
#define chdir _chdir
#endif