//
//  FiberClient.hpp
//  fiber_server
//
//  Created by shuxin 　　zheng on 2020/9/26.
//  Copyright © 2020 acl. All rights reserved.
//

#ifndef FiberClient_hpp
#define FiberClient_hpp

class FiberClient : public acl::fiber {
public:
    FiberClient(int fd) : fd_(fd) {}
    
protected:
    // @override
    void run(void);

    ~FiberClient(void) {}

private:
    int fd_;
};

#endif /* FiberClient_hpp */
