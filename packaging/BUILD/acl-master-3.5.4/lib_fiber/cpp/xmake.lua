-- define target: fiber_cpp
target("fiber_cpp")

    -- set kind: static/shared
    set_kind("$(kind)")

    -- add deps: fiber, acl
    add_deps("fiber", "acl_cpp", "acl")

    -- add source files
    add_files("src/**.cpp")

    -- add include directories
    add_includedirs("src")
    add_includedirs("include", {public = true})

    -- add headers
    add_headerfiles("include/(**.h)", "include/(**.hpp)")



