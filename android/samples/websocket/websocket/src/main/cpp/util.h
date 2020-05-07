#pragma once

void JString2String(JNIEnv *env, jstring js, std::string &out);
jstring String2JString(JNIEnv *env, const char *s);
void log_info(const char* fmt, ...);
void log_error(const char* fmt, ...);
