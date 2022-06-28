#include <string>
#include "HttpTest.hpp"
#import <Foundation/Foundation.h>
#import "HttpRequest.h"

@implementation http_request

- (NSString*) test: (NSString*) url 
{
//	std::string result = http_test([url UTF8String]);
    http_test([url UTF8String]);
    std::string result("ok");
	NSString * aString = [NSString stringWithUTF8String:result.c_str()];
	return aString;
}

@end
