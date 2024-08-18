//
//  FiberServer.hpp
//  fiber_server
//
//  Created by shuxin 　　zheng on 2020/9/26.
//  Copyright © 2020 acl. All rights reserved.
//

#ifndef FiberServer_hpp
#define FiberServer_hpp

#include <string>

class FiberServer : public acl::fiber {
public:
    FiberServer(const char* ip, int port);

protected:
    // @override
    void run(void);

    ~FiberServer(void) {}
    
private:
    std::string ip_;
    int port_;
    int lfd_;
    
    int BindAndrListen(const char *ip, int port);
};

#endif /* FiberServer_hpp */
