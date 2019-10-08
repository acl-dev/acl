-- define target: fiber_cpp
target("fiber_cpp")

    -- set kind: static/shared
    set_kind("$(kind)")

    -- add deps: fiber, acl
    add_deps("fiber", "acl_cpp", "acl")

    -- add source files
    add_files("src/**.cpp")

    -- add include directories
    add_includedirs("src", "include")
    add_includedirs("$(projectdir)/lib_fiber/c/include")
    add_includedirs("$(projectdir)/lib_acl/include")
    add_includedirs("$(projectdir)/lib_acl_cpp/include")

    -- add headers
    add_headerfiles("include/(**.h)", "include/(**.hpp)")
    add_includedirs("src", "include", "../../lib_acl/src/master")
    -- set_headerdir("$(buildir)/include/fiber_cpp")



