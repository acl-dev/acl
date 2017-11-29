-- define target: acl
target("acl")

    -- set kind: static/shared
    set_kind("$(kind)")

    -- add source files
    add_files("src/**.c|*/bak/*.c")

    -- add include directories
    add_includedirs(".", "include")

    -- add headers
    add_headers("include/(**.h)")
    set_headerdir("$(buildir)/include/acl")

    -- add defines
    add_defines("USE_REUSEPORT")
