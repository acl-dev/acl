#pragma once
#include <yyjson.h>
#include "dao_base.h"

namespace pkv {

class string_dao : public dao_base {
public:
    string_dao();
    ~string_dao() override;

    void set_string(const char* data);

    bool to_string(std::string& out);

private:
    const char* data_;
};

} // namespace pkv
