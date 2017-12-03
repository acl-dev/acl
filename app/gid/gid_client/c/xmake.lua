target("gid")

    -- set kind: static
    set_kind("static")

    -- set default: disable
    set_default(false)

    -- add deps: acl_cpp, acl
    add_deps("acl_cpp", "acl")

    -- add source files
    add_files("src/**.c")

    -- add include directories
    add_includedirs(".", "include")

