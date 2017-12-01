-- project
set_project("acl")

-- version
set_version("3.3.1.rc1")
set_xmakever("2.1.6")

-- set warning all as error
set_warnings("all", "error")

-- set the object files directory
set_objectdir("$(buildir)/$(mode)/$(arch)/.objs")
set_targetdir("$(buildir)/$(mode)/$(arch)")

-- the debug or release mode
if is_mode("debug") then
    
    -- enable the debug symbols
    set_symbols("debug")

    -- disable optimization
    set_optimize("none")

    -- link libcmtd.lib
    if is_plat("windows") then 
        add_cxflags("-MTd") 
    end

elseif is_mode("release") then

    -- set the symbols visibility: hidden
    if not is_kind("shared") then
        set_symbols("hidden")
    end

    -- strip all symbols
    set_strip("all")

    -- enable fastest optimization
    set_optimize("fastest")

    -- link libcmt.lib
    if is_plat("windows") then 
        add_cxflags("-MT") 
    end
end

-- add common flags and macros
add_defines("ACL_WRITEABLE_CHECK", "ACL_PREPARE_COMPILE")

-- for the windows platform (msvc)
if is_plat("windows") then 
    add_cxxflags("-EHsc")
    add_ldflags("-nodefaultlib:\"msvcrt.lib\"")
	add_links("ws2_32", "IPHlpApi", "kernel32", "user32", "gdi32")
end

-- for the android platform
if is_plat("android") then
    add_defines("ANDROID")
end

-- for macosx
if is_plat("macosx") then
    add_defines("MACOSX")
    add_links("pthread", "z")
end

-- for linux
if is_plat("linux") then
    add_defines("LINUX2")
    add_links("pthread", "z", "dl")
end

-- for mingw
if is_plat("mingw") then
    add_defines("LINUX2", "MINGW")
end

-- for freebsd
if is_plat("freebsd") then
    add_defines("FREEBSD")
end

-- for Solaris (x86)
if is_plat("sunos5") then
    add_defines("SUNOS5")
end

-- for all non-windows platforms
if not is_plat("windows") then
    add_cflags("-Wshadow", "-Wpointer-arith", "-Waggregate-return", "-Wmissing-prototypes", "-Wno-long-long", "-Wuninitialized", "-Wstrict-prototypes")
    add_defines("_REENTRANT", "_USE_FAST_MACRO", "_POSIX_PTHREAD_SEMANTICS", "_GNU_SOURCE=1")
end

-- include project sources
includes("app/**/xmake.lua", "lib_acl", "lib_protocol", "lib_acl_cpp") 
if is_plat("linux") then
    includes("lib_fiber/c", "lib_fiber/cpp")
end

-- Build project (static library/release by default)
--
-- linux/macosx/windows:
-- $ xmake
--
-- iphoneos:
-- $ xmake f -p iphoneos
-- $ xmake
--
-- android:
-- $ xmake f -p android --ndk=/home/xxx/android-ndk-r10e
-- $ xmake
--
-- freebsd:
-- $ xmake f -p freebsd
-- $ xmake
--
-- mingw:
-- $ xmake f -p mingw
-- $ xmake
--
-- Build for share library with debug mode
--
-- $ xmake f -k shared -m debug; xmake
--
-- Build and run app example
--
-- $ xmake run [gson|iconv|wizard|master_ctl|master_daemon|...]
--
-- Configuration 
--
-- $ xmake f -p [windows|linux|iphoneos|android|macosx|freebsd|sunos5|cross] -m [debug|release] -a [x86|x64|x86_64|armv7|arm64|armv8-a] -k [static|shared]
--
-- Generate IDE project
--
-- $ xmake project -k vs2008
-- $ xmake project -k vs2017 -m "debug,release"
-- $ xmake project -k makefile
--
-- If you want to known more usage about xmake, please see: http://xmake.io/#/home?id=configuration
--
