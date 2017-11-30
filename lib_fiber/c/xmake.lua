-- define target: fiber
target("fiber")

    -- set kind: static/shared
    set_kind("$(kind)")

    -- add deps: acl
    add_deps("acl")

    -- add source files
    add_files("src/**.c")

    -- add include directories
    add_includedirs("src", "include", "../../lib_acl/src/master")

    -- add headers
    add_headers("include/(**.h)")
    set_headerdir("$(buildir)/include/fiber")



