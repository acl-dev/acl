-- define target: fiber
target("fiber")

    -- set kind: static/shared
    set_kind("$(kind)")

    -- add deps: acl
    -- add_deps("acl")

    -- add source files
    add_files("src/**.c")

    -- add include directories
    add_includedirs("src", "include")

    -- add headers
    add_headerfiles("include/(**.h)")
    -- set_headerdir("$(buildir)/include/fiber")

    -- add flags
    add_cxflags("-std=gnu99")
    add_defines("USE_JMP")

