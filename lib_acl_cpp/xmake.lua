-- define target: acl_cpp
target("acl_cpp")

    -- set kind: static/shared
    set_kind("$(kind)")

    -- add deps: protocol, acl
    add_deps("protocol", "acl")

    -- add source files
    add_files("src/**.cpp|aliyun/**.cpp")

    -- add include directories
    add_includedirs("$(projectdir)/include")
    add_includedirs("$(projectdir)/include/zlib")
    add_includedirs("$(projectdir)/include/mysql")
    add_includedirs("$(projectdir)/include/pgsql")
    add_includedirs("$(projectdir)/include/sqlite")
    add_includedirs("src", "include")

    -- add headers
    add_headers("include/(**.h)", "include/(**.hpp)", "include/(**.ipp)")
    set_headerdir("$(buildir)/include/acl_cpp")

    -- set precompile header
    set_pcxxheader("src/acl_stdafx.hpp")

    -- add defines and links
    add_defines("HAS_MYSQL_DLL", "HAS_PGSQL_DLL", "HAS_SQLITE_DLL", "HAS_POLARSSL_DLL")
    if is_plat("windows") then
        add_defines("HAS_ZLIB_DLL", "USE_WIN_ICONV")
    else
        add_links("iconv")
    end

