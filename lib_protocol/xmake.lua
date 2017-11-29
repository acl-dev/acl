-- define target: protocol
target("protocol")

    -- set kind: static/shared
    set_kind("$(kind)")

    -- add deps: acl
    add_deps("acl")

    -- add source files
    add_files("src/**.c")

    -- add include directories
    add_includedirs("src", "include")

    -- add headers
    add_headers("include/(**.h)")
    set_headerdir("$(buildir)/include/protocol")



