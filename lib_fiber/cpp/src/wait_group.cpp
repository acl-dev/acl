#include "stdafx.hpp"
#include "fiber/fiber_tbox.hpp"
#include "fiber/wait_group.hpp"

namespace acl {

wait_group::wait_group(void)
{
	state_ = 0;
	box_   = new acl::fiber_tbox<unsigned long>;
}

wait_group::~wait_group(void)
{
	delete box_;
}

void wait_group::add(int n)
{
    state_ += (long long)n << 32;
    long long state = state_;
    int c = (int)(state >> 32);
    uint32_t w =  (uint32_t)state;
    if(c < 0){
        acl_msg_fatal("wait_group: negative wait_group counter");
    }
    if(w != 0 && n > 0 && c == n){
        acl_msg_fatal("wait_group: add called concurrently with wait");
    }
    if(c > 0 || w ==0){
        return;
    }
    if(state_ != state){
        acl_msg_fatal("wait_group: add called concurrently with wait");
    }
    state_ = 0;
    for (size_t i = 0; i < w; i++) {
#ifdef	_DEBUG
        unsigned long* tid = new unsigned long;
        *tid = acl::thread::self();
        box_->push(tid);
#else
        box_->push(NULL);
#endif
    }
}

void wait_group::done(void)
{
    add(-1);
}

void wait_group::wait(void)
{
    long long state = state_;
    int c = (int)(state >> 32);
    uint32_t w =  (uint32_t)state;
    if(c == 0) return;
    state_++;
    bool found;
#ifdef	_DEBUG
    unsigned long* tid = box_->pop(-1, &found);
    assert(found);
    delete tid;
#else
    (void) box_->pop(-1, &found);
    assert(found);
#endif
    if(state_ != 0){
        acl_msg_fatal("wait_group: wait_group is reused before previous wait has returned");
    }
}

} // namespace acl
