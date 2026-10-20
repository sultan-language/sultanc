# Select one LLVM 19-23 development installation and expose it as
# SultanC::LLVM (shared library + matching C API include directory) for both
# bootstrap stages.
#
# Selection, in precedence order (an explicit selection that is invalid is a
# configuration error, never a reason to fall back to discovery):
#   1. SULTANC_LLVM_LIBRARY                                   (library pin)
#   2. LLVM_CONFIG (cache variable or environment): one llvm-config executable
#   3. LLVM_DIR: the LLVM CMake package directory (LLVM's documented contract)
#   4. Automatic discovery: the newest major found under CMake's prefixes,
#      llvm-config programs on PATH and the Homebrew prefix. Neither package
#      metadata nor llvm-config is required for discovery.
# SULTANC_LLVM_INCLUDE_DIR (one directory or a list) overrides only the
# include directories of whichever installation was selected above.
#
# The version is identified from library names, llvm-config or package
# version files. Include directories come from the selected installation's
# metadata (llvm-config --includedir/--cppflags, the package's
# LLVM_INCLUDE_DIRS) and are kept complete, including Debian's split
# <root>/include/llvm-N + <root>/include/llvm-c-N layout; individual headers
# are never probed and no compile or link test is run. A system prefix holding
# several majors gets the directories of the selected major, not an ambiguous
# ${prefix}/include. Include directories never decide selection: when the
# metadata reports none, the installation is still selected and SultanC::LLVM
# carries no include directory. An explicit SULTANC_LLVM_INCLUDE_DIR replaces
# only the include directories: library selection is unaffected by it. The linked
# library is always the monolithic shared libLLVM, never a component library.
set(_sultanc_llvm_versions 23 22 21 20 19)

function(_sultanc_llvm_supported major output)
    set(${output} FALSE PARENT_SCOPE)
    if(major IN_LIST _sultanc_llvm_versions)
        set(${output} TRUE PARENT_SCOPE)
    endif()
endfunction()

# Values written by an earlier automatic selection are not user overrides.
set(_sultanc_pin_include "")
set(_sultanc_pin_library "")
if(SULTANC_LLVM_INCLUDE_DIR AND
   NOT SULTANC_LLVM_INCLUDE_DIR STREQUAL _SULTANC_LLVM_AUTO_INCLUDE_DIR AND
   NOT SULTANC_LLVM_INCLUDE_DIR MATCHES "-NOTFOUND$")
    set(_sultanc_pin_include "${SULTANC_LLVM_INCLUDE_DIR}")
endif()
if(SULTANC_LLVM_LIBRARY AND
   NOT SULTANC_LLVM_LIBRARY STREQUAL _SULTANC_LLVM_AUTO_LIBRARY AND
   NOT SULTANC_LLVM_LIBRARY MATCHES "-NOTFOUND$")
    set(_sultanc_pin_library "${SULTANC_LLVM_LIBRARY}")
endif()

set(LLVM_CONFIG "" CACHE FILEPATH
    "Explicit llvm-config of the LLVM installation to use (also read from the environment)")
set(_sultanc_llvm_config "${LLVM_CONFIG}")
if(NOT _sultanc_llvm_config AND DEFINED ENV{LLVM_CONFIG} AND
   NOT "$ENV{LLVM_CONFIG}" STREQUAL "")
    set(_sultanc_llvm_config "$ENV{LLVM_CONFIG}")
endif()

# --- Installation facts ------------------------------------------------------

function(_sultanc_llvm_config_query tool output)
    execute_process(COMMAND "${tool}" ${ARGN}
        OUTPUT_VARIABLE _value OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET RESULT_VARIABLE _result)
    if(_result EQUAL 0)
        set(${output} "${_value}" PARENT_SCOPE)
    else()
        set(${output} "" PARENT_SCOPE)
    endif()
endfunction()

# Every major an installation prefix provides. Computed once per prefix per
# configure and memoized in a global property.
function(_sultanc_llvm_majors prefix output)
    string(MD5 _key "${prefix}")
    get_property(_known GLOBAL PROPERTY "_SULTANC_LLVM_MAJORS_${_key}" SET)
    if(_known)
        get_property(_majors GLOBAL PROPERTY "_SULTANC_LLVM_MAJORS_${_key}")
        set(${output} "${_majors}" PARENT_SCOPE)
        return()
    endif()
    set(_majors "")
    if(prefix MATCHES "/llvm[-@]([0-9]+)$")
        list(APPEND _majors "${CMAKE_MATCH_1}")
    elseif(prefix MATCHES "/llvm@([0-9]+)/")
        list(APPEND _majors "${CMAKE_MATCH_1}")
    elseif(prefix MATCHES "/llvm/([0-9]+)(\\.|$)")
        list(APPEND _majors "${CMAKE_MATCH_1}")
    endif()
    if(EXISTS "${prefix}/bin/llvm-config")
        _sultanc_llvm_config_query("${prefix}/bin/llvm-config" _reported --version)
        if(_reported MATCHES "^([0-9]+)\\.")
            list(APPEND _majors "${CMAKE_MATCH_1}")
        endif()
    endif()
    foreach(_libdir lib lib64)
        set(_metadata "${prefix}/${_libdir}/cmake/llvm/LLVMConfigVersion.cmake")
        if(EXISTS "${_metadata}")
            file(STRINGS "${_metadata}" _line
                REGEX "^[ \t]*set\\(PACKAGE_VERSION[ \t]+")
            if(_line AND "${_line}" MATCHES "PACKAGE_VERSION[ \t]+[^0-9]*([0-9]+)[.]")
                list(APPEND _majors "${CMAKE_MATCH_1}")
            endif()
        endif()
    endforeach()
    _sultanc_llvm_library_dirs("${prefix}" "" _libdirs)
    foreach(_libdir IN LISTS _libdirs)
        file(GLOB _libraries "${_libdir}/libLLVM-*.so*"
            "${_libdir}/libLLVM-*.dylib" "${_libdir}/libLLVM.so.*"
            "${_libdir}/libLLVM.*.dylib")
        foreach(_library IN LISTS _libraries)
            get_filename_component(_name "${_library}" NAME)
            if(_name MATCHES "^libLLVM-([0-9]+)[.]")
                list(APPEND _majors "${CMAKE_MATCH_1}")
            elseif(_name MATCHES "^libLLVM[.]so[.]([0-9]+)")
                if(CMAKE_MATCH_1 GREATER 1)
                    list(APPEND _majors "${CMAKE_MATCH_1}")
                endif()
            elseif(_name MATCHES "^libLLVM[.]([0-9]+)[.]dylib$")
                list(APPEND _majors "${CMAKE_MATCH_1}")
            endif()
        endforeach()
    endforeach()
    list(REMOVE_DUPLICATES _majors)
    set_property(GLOBAL PROPERTY "_SULTANC_LLVM_MAJORS_${_key}" "${_majors}")
    set(${output} "${_majors}" PARENT_SCOPE)
endfunction()

# Library directories of an installation. A version-labelled prefix such as
# Debian's <root>/lib/llvm-N also uses the multiarch shared library directory
# <root>/lib/<arch> of the same root; it is derived, not hardcoded.
function(_sultanc_llvm_library_dirs prefix version output)
    set(_dirs "${prefix}/lib" "${prefix}/lib64")
    if(CMAKE_LIBRARY_ARCHITECTURE)
        list(APPEND _dirs "${prefix}/lib/${CMAKE_LIBRARY_ARCHITECTURE}")
        if(version AND prefix MATCHES "^(.*)/lib/llvm-${version}$")
            list(APPEND _dirs "${CMAKE_MATCH_1}/lib/${CMAKE_LIBRARY_ARCHITECTURE}")
        endif()
    endif()
    set(${output} "${_dirs}" PARENT_SCOPE)
endfunction()

# Generic shared-library names have no inherent version. Accept them only when
# LLVM metadata from this installation identifies the same shared library or a
# single-version installed CMake package identifies the installation.
function(_sultanc_llvm_generic_identity prefix version library output)
    set(${output} FALSE PARENT_SCOPE)
    file(REAL_PATH "${prefix}" _prefix_real)
    file(REAL_PATH "${library}" _library_real)
    file(RELATIVE_PATH _relative "${_prefix_real}" "${_library_real}")
    if(IS_ABSOLUTE "${_relative}" OR _relative MATCHES "^\\.\\.(/|$)")
        return()
    endif()
    if(EXISTS "${prefix}/bin/llvm-config")
        _sultanc_llvm_config_query("${prefix}/bin/llvm-config" _reported --version)
        if(_reported MATCHES "^${version}([.]|$)")
            _sultanc_llvm_config_query("${prefix}/bin/llvm-config" _files
                --link-shared --libfiles)
            separate_arguments(_paths UNIX_COMMAND "${_files}")
            foreach(_path IN LISTS _paths)
                if(EXISTS "${_path}")
                    file(REAL_PATH "${_path}" _path_real)
                    if(_path_real STREQUAL _library_real)
                        set(${output} TRUE PARENT_SCOPE)
                        return()
                    endif()
                endif()
            endforeach()
        endif()
    endif()
    # An unversioned system prefix containing multiple LLVM majors remains
    # ambiguous even when one of them exported package-version metadata.
    _sultanc_llvm_majors("${prefix}" _installed_majors)
    list(LENGTH _installed_majors _major_count)
    if(NOT _major_count EQUAL 1)
        return()
    endif()
    foreach(_libdir lib lib64)
        set(_metadata "${prefix}/${_libdir}/cmake/llvm/LLVMConfigVersion.cmake")
        if(EXISTS "${_metadata}")
            file(STRINGS "${_metadata}" _line
                REGEX "^[ \\t]*set\\(PACKAGE_VERSION[ \\t]+")
            if(_line AND "${_line}" MATCHES
               "PACKAGE_VERSION[ \\t]+[^0-9]*${version}[.]")
                set(${output} TRUE PARENT_SCOPE)
                return()
            endif()
        endif()
    endforeach()
endfunction()

# The shared LLVM library of `version` in `dirs` (an installation's library
# directories). `prefix` identifies the installation for generic names.
function(_sultanc_llvm_library prefix version dirs output)
    set(_candidates "")
    if(_sultanc_pin_library)
        list(APPEND _candidates "${_sultanc_pin_library}")
    else()
        # A matching major always wins over an unversioned shared library.
        unset(_versioned_library CACHE)
        unset(_versioned_library)
        find_library(_versioned_library NAMES "LLVM-${version}"
            PATHS ${dirs} NO_DEFAULT_PATH NO_CACHE)
        if(_versioned_library)
            list(APPEND _candidates "${_versioned_library}")
        endif()
        foreach(_dir IN LISTS dirs)
            # Debian may install only libLLVM-19.so.1, without libLLVM-19.so.
            file(GLOB _versioned "${_dir}/libLLVM-${version}.so.*"
                "${_dir}/libLLVM.so.${version}"
                "${_dir}/libLLVM.so.${version}.*"
                "${_dir}/libLLVM.${version}.dylib"
                "${_dir}/libLLVM-${version}.*.dylib")
            list(SORT _versioned)
            list(APPEND _candidates ${_versioned})
        endforeach()
        # Only consider a generic name once all matching names are exhausted.
        unset(_generic_library CACHE)
        unset(_generic_library)
        find_library(_generic_library NAMES LLVM PATHS ${dirs}
            NO_DEFAULT_PATH NO_CACHE)
        if(_generic_library)
            list(APPEND _candidates "${_generic_library}")
        endif()
    endif()
    list(REMOVE_DUPLICATES _candidates)

    set(_library "")
    foreach(_candidate IN LISTS _candidates)
        if(NOT EXISTS "${_candidate}")
            continue()
        endif()
        get_filename_component(_name "${_candidate}" NAME)
        file(REAL_PATH "${_candidate}" _resolved)
        # A symlink may resolve within this installation's library
        # directories, but must not silently import a different installation.
        set(_same_installation FALSE)
        foreach(_dir IN LISTS dirs)
            if(IS_DIRECTORY "${_dir}")
                file(REAL_PATH "${_dir}" _real_dir)
                file(RELATIVE_PATH _relative "${_real_dir}" "${_resolved}")
                if(NOT IS_ABSOLUTE "${_relative}" AND
                   NOT _relative MATCHES "^\\.\\.(/|$)")
                    set(_same_installation TRUE)
                    break()
                endif()
            endif()
        endforeach()
        if(NOT _same_installation)
            continue()
        endif()
        get_filename_component(_resolved_name "${_resolved}" NAME)
        # Reject static archives, conflicting versioned names and symlinks.
        set(_named_majors "")
        set(_resolved_major "")
        set(_checking_resolved FALSE)
        foreach(_part IN ITEMS "${_name}" "${_resolved_name}")
            set(_named_major "")
            if(_part MATCHES "^libLLVM-([0-9]+)([.]so([.][0-9]+)*|([.][0-9]+)*[.]dylib)$")
                set(_named_major "${CMAKE_MATCH_1}")
            elseif(_part MATCHES "^libLLVM[.]so[.]([0-9]+)([.][0-9]+)*$")
                # Some LLVM builds use an ABI soname libLLVM.so.1.
                if(CMAKE_MATCH_1 GREATER 1)
                    set(_named_major "${CMAKE_MATCH_1}")
                endif()
            elseif(_part MATCHES "^libLLVM[.]([0-9]+)([.][0-9]+)*[.]dylib$")
                set(_named_major "${CMAKE_MATCH_1}")
            elseif(NOT _part MATCHES "^libLLVM([.]so([.][0-9]+)*|[.]dylib)$")
                set(_named_major "invalid")
            endif()
            if(_named_major)
                list(APPEND _named_majors "${_named_major}")
            endif()
            if(_checking_resolved)
                set(_resolved_major "${_named_major}")
            endif()
            set(_checking_resolved TRUE)
        endforeach()
        set(_conflicting FALSE)
        foreach(_major IN LISTS _named_majors)
            if(NOT _major STREQUAL version)
                set(_conflicting TRUE)
            endif()
        endforeach()
        if(_conflicting)
            continue()
        endif()
        # A version-labelled prefix cannot supply another LLVM major.
        if(prefix MATCHES "/llvm[-@]([0-9]+)(/|$)")
            if(NOT CMAKE_MATCH_1 STREQUAL version)
                continue()
            endif()
        endif()
        if(prefix MATCHES "/llvm/([0-9]+)([.]|/|$)")
            if(NOT CMAKE_MATCH_1 STREQUAL version)
                continue()
            endif()
        endif()
        # A generic destination needs real installation metadata.
        if(NOT _resolved_major)
            _sultanc_llvm_generic_identity("${prefix}" "${version}"
                "${_candidate}" _identified)
            if(NOT _identified)
                continue()
            endif()
        endif()
        set(_library "${_candidate}")
        break()
    endforeach()
    set(${output} "${_library}" PARENT_SCOPE)
endfunction()

# Monolithic shared LLVM library names (libLLVM, libLLVM-N, libLLVM.so.N,
# libLLVM.N.dylib and their versioned forms). Component libraries such as
# libLLVMCore.so are not the monolithic dependency.
function(_sultanc_llvm_monolithic_name name output)
    set(${output} FALSE PARENT_SCOPE)
    if(name MATCHES "^libLLVM(-[0-9]+)?([.]so([.][0-9]+)*|([.][0-9]+)*[.]dylib)$")
        set(${output} TRUE PARENT_SCOPE)
    endif()
endfunction()

# Include directories reported by one llvm-config: --includedir plus every -I
# of --cppflags, in order and without duplicates.
function(_sultanc_llvm_config_includes tool output)
    set(_dirs "")
    _sultanc_llvm_config_query("${tool}" _includedir --includedir)
    if(_includedir)
        list(APPEND _dirs "${_includedir}")
    endif()
    _sultanc_llvm_config_query("${tool}" _cppflags --cppflags)
    separate_arguments(_flags UNIX_COMMAND "${_cppflags}")
    foreach(_flag IN LISTS _flags)
        if(_flag MATCHES "^-I(.+)$")
            list(APPEND _dirs "${CMAKE_MATCH_1}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _dirs)
    set(${output} "${_dirs}" PARENT_SCOPE)
endfunction()

# The include directories of `version` for an installation `prefix`, from its
# metadata: its own llvm-config when it reports the same major; otherwise the
# installation's development layout (Debian's split per-major directories,
# a per-major prefix, or ${prefix}/include when the prefix holds one major).
# Directories are taken whole; their header files are not inspected.
function(_sultanc_llvm_includes prefix version output)
    set(${output} "" PARENT_SCOPE)
    if(EXISTS "${prefix}/bin/llvm-config")
        _sultanc_llvm_config_query("${prefix}/bin/llvm-config" _reported --version)
        if(_reported MATCHES "^${version}([.]|$)")
            _sultanc_llvm_config_includes("${prefix}/bin/llvm-config" _dirs)
            if(_dirs)
                set(${output} "${_dirs}" PARENT_SCOPE)
                return()
            endif()
        endif()
    endif()
    set(_dirs "")
    set(_root "${prefix}")
    if(prefix MATCHES "^(.*)/lib/llvm-${version}$")
        set(_root "${CMAKE_MATCH_1}")
        list(APPEND _dirs "${prefix}/include")
    endif()
    # Debian splits one major's headers: C++ under llvm-N, the C API under llvm-c-N.
    list(APPEND _dirs "${_root}/include/llvm-${version}" "${_root}/include/llvm-c-${version}")
    _sultanc_llvm_majors("${prefix}" _majors)
    list(LENGTH _majors _major_count)
    if(_major_count EQUAL 1 OR prefix MATCHES "/llvm[-@/]${version}([.]|/|$)")
        list(APPEND _dirs "${prefix}/include")
    endif()
    set(_existing "")
    foreach(_dir IN LISTS _dirs)
        if(IS_DIRECTORY "${_dir}")
            list(APPEND _existing "${_dir}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _existing)
    set(${output} "${_existing}" PARENT_SCOPE)
endfunction()

# --- Selection ---------------------------------------------------------------

set(_sultanc_selected_prefix "")
set(_sultanc_selected_version "")
set(_sultanc_selected_library "")
set(_sultanc_selected_include "")
set(_sultanc_selection_source "")

if(_sultanc_llvm_config AND NOT _sultanc_pin_library)
    # Explicit llvm-config: its installation is authoritative.
    if(NOT EXISTS "${_sultanc_llvm_config}")
        message(FATAL_ERROR "LLVM_CONFIG does not exist: ${_sultanc_llvm_config}")
    endif()
    _sultanc_llvm_config_query("${_sultanc_llvm_config}" _reported --version)
    if(NOT _reported MATCHES "^([0-9]+)\\.")
        message(FATAL_ERROR "LLVM_CONFIG did not report a version: ${_sultanc_llvm_config}")
    endif()
    set(_sultanc_selected_version "${CMAKE_MATCH_1}")
    _sultanc_llvm_supported("${_sultanc_selected_version}" _supported)
    if(NOT _supported)
        message(FATAL_ERROR "LLVM_CONFIG selects LLVM ${_reported}; SultanC supports LLVM 19-23.")
    endif()
    _sultanc_llvm_config_query("${_sultanc_llvm_config}" _sultanc_selected_prefix --prefix)
    _sultanc_llvm_config_query("${_sultanc_llvm_config}" _libdir --libdir)
    # SultanC links the single monolithic shared libLLVM. `--libfiles` lists
    # component libraries when LLVM was built with shared components (its
    # first entry is then one component, not LLVM); only a monolithic name is
    # accepted from it, otherwise the installation's library directories are
    # searched for the monolithic library of the reported major.
    _sultanc_llvm_config_query("${_sultanc_llvm_config}" _libfiles --link-shared --libfiles)
    separate_arguments(_libfiles UNIX_COMMAND "${_libfiles}")
    foreach(_file IN LISTS _libfiles)
        get_filename_component(_file_name "${_file}" NAME)
        _sultanc_llvm_monolithic_name("${_file_name}" _monolithic)
        if(_monolithic AND EXISTS "${_file}")
            set(_sultanc_selected_library "${_file}")
            break()
        endif()
    endforeach()
    if(NOT _sultanc_selected_library)
        _sultanc_llvm_library_dirs("${_sultanc_selected_prefix}"
            "${_sultanc_selected_version}" _dirs)
        list(PREPEND _dirs "${_libdir}")
        list(REMOVE_DUPLICATES _dirs)
        _sultanc_llvm_library("${_sultanc_selected_prefix}"
            "${_sultanc_selected_version}" "${_dirs}" _sultanc_selected_library)
    endif()
    _sultanc_llvm_config_includes("${_sultanc_llvm_config}" _sultanc_selected_include)
    set(_sultanc_selection_source "LLVM_CONFIG=${_sultanc_llvm_config}")
    if(NOT _sultanc_selected_library)
        message(FATAL_ERROR "LLVM_CONFIG (${_sultanc_llvm_config}) selects LLVM "
            "${_sultanc_selected_version}, but no monolithic shared libLLVM library was found "
            "(reported: ${_libfiles}).")
    endif()
elseif(LLVM_DIR AND NOT _sultanc_pin_library)
    # Explicit package directory, read directly (aliases resolved first).
    file(REAL_PATH "${LLVM_DIR}" _sultanc_llvm_dir)
    if(NOT EXISTS "${_sultanc_llvm_dir}/LLVMConfig.cmake")
        message(FATAL_ERROR "LLVM_DIR has no LLVMConfig.cmake: ${LLVM_DIR}")
    endif()
    find_package(LLVM CONFIG REQUIRED PATHS "${_sultanc_llvm_dir}" NO_DEFAULT_PATH)
    set(_sultanc_selected_version "${LLVM_VERSION_MAJOR}")
    _sultanc_llvm_supported("${_sultanc_selected_version}" _supported)
    if(NOT _supported)
        message(FATAL_ERROR "LLVM_DIR selects LLVM ${LLVM_PACKAGE_VERSION}; SultanC supports LLVM 19-23.")
    endif()
    if(LLVM_INSTALL_PREFIX)
        set(_sultanc_selected_prefix "${LLVM_INSTALL_PREFIX}")
    else()
        set(_sultanc_selected_prefix "${_sultanc_llvm_dir}")
    endif()
    set(_dirs ${LLVM_LIBRARY_DIRS} ${LLVM_LIBRARY_DIR})
    _sultanc_llvm_library_dirs("${_sultanc_selected_prefix}"
        "${_sultanc_selected_version}" _prefix_dirs)
    list(APPEND _dirs ${_prefix_dirs})
    list(REMOVE_DUPLICATES _dirs)
    _sultanc_llvm_library("${_sultanc_selected_prefix}"
        "${_sultanc_selected_version}" "${_dirs}" _sultanc_selected_library)
    # The package's include directories are kept complete, in order.
    set(_sultanc_selected_include ${LLVM_INCLUDE_DIRS})
    list(REMOVE_DUPLICATES _sultanc_selected_include)
    set(_sultanc_selection_source "LLVM_DIR=${LLVM_DIR}")
    if(NOT _sultanc_selected_library)
        message(FATAL_ERROR "LLVM_DIR (${LLVM_DIR}) has no shared LLVM ${_sultanc_selected_version} library.")
    endif()
else()
    # Automatic discovery or pinned files.
    set(_sultanc_roots ${CMAKE_PREFIX_PATH})
    if(DEFINED ENV{CMAKE_PREFIX_PATH} AND NOT "$ENV{CMAKE_PREFIX_PATH}" STREQUAL "")
        file(TO_CMAKE_PATH "$ENV{CMAKE_PREFIX_PATH}" _sultanc_env_prefixes)
        list(APPEND _sultanc_roots ${_sultanc_env_prefixes})
    endif()
    # Optional llvm-config programs on PATH identify nonstandard installations.
    foreach(_tool llvm-config-23 llvm-config-22 llvm-config-21 llvm-config-20
                  llvm-config-19 llvm-config)
        find_program(_sultanc_config NAMES "${_tool}" NO_CACHE)
        if(_sultanc_config)
            _sultanc_llvm_config_query("${_sultanc_config}" _reported_prefix --prefix)
            if(IS_DIRECTORY "${_reported_prefix}")
                list(APPEND _sultanc_roots "${_reported_prefix}")
            endif()
        endif()
        unset(_sultanc_config)
    endforeach()
    if(_sultanc_pin_library)
        get_filename_component(_sultanc_pin_libdir "${_sultanc_pin_library}" DIRECTORY)
        get_filename_component(_sultanc_pin_prefix "${_sultanc_pin_libdir}" DIRECTORY)
        if(CMAKE_LIBRARY_ARCHITECTURE AND
           _sultanc_pin_libdir MATCHES "/lib/${CMAKE_LIBRARY_ARCHITECTURE}$")
            get_filename_component(_sultanc_pin_prefix "${_sultanc_pin_prefix}" DIRECTORY)
        endif()
        set(_sultanc_roots "${_sultanc_pin_prefix}")
    endif()
    if(NOT _sultanc_pin_library)
        # CMake's platform prefixes plus the Homebrew prefix when Homebrew is
        # present; no machine-specific directory list is embedded here.
        list(APPEND _sultanc_roots ${CMAKE_SYSTEM_PREFIX_PATH})
        if(DEFINED ENV{HOMEBREW_PREFIX} AND NOT "$ENV{HOMEBREW_PREFIX}" STREQUAL "")
            list(PREPEND _sultanc_roots "$ENV{HOMEBREW_PREFIX}")
        endif()
        find_program(_sultanc_brew NAMES brew NO_CACHE)
        if(_sultanc_brew)
            _sultanc_llvm_config_query("${_sultanc_brew}" _brew_prefix --prefix)
            if(IS_DIRECTORY "${_brew_prefix}")
                list(APPEND _sultanc_roots "${_brew_prefix}")
            endif()
        endif()
    endif()

    set(_sultanc_prefixes "")
    foreach(_root IN LISTS _sultanc_roots)
        if(NOT _root)
            continue()
        endif()
        get_filename_component(_root "${_root}" ABSOLUTE)
        set(_options "${_root}" "${_root}/opt/llvm")
        foreach(_version IN LISTS _sultanc_llvm_versions)
            list(APPEND _options "${_root}/opt/llvm@${_version}"
                "${_root}/lib/llvm-${_version}" "${_root}/llvm-${_version}"
                "${_root}/llvm@${_version}")
        endforeach()
        foreach(_prefix IN LISTS _options)
            if(IS_DIRECTORY "${_prefix}")
                file(REAL_PATH "${_prefix}" _real_prefix)
                list(APPEND _sultanc_prefixes "${_real_prefix}")
            endif()
        endforeach()
    endforeach()
    list(REMOVE_DUPLICATES _sultanc_prefixes)

    set(_sultanc_unlinked "")
    foreach(_version IN LISTS _sultanc_llvm_versions)
        foreach(_prefix IN LISTS _sultanc_prefixes)
            _sultanc_llvm_majors("${_prefix}" _majors)
            if(NOT _version IN_LIST _majors)
                continue()
            endif()
            _sultanc_llvm_library_dirs("${_prefix}" "${_version}" _dirs)
            _sultanc_llvm_library("${_prefix}" "${_version}" "${_dirs}" _library)
            if(NOT _library)
                list(APPEND _sultanc_unlinked "LLVM ${_version} at ${_prefix}")
                continue()
            endif()
            set(_sultanc_selected_prefix "${_prefix}")
            set(_sultanc_selected_version "${_version}")
            set(_sultanc_selected_library "${_library}")
            break()
        endforeach()
        if(_sultanc_selected_prefix)
            break()
        endif()
    endforeach()
    if(NOT _sultanc_selected_prefix)
        if(_sultanc_unlinked)
            string(JOIN ", " _missing ${_sultanc_unlinked})
            message(FATAL_ERROR "LLVM found (${_missing}), but no linkable LLVM shared library was found.")
        endif()
        message(FATAL_ERROR "No installed LLVM 19-23 version was detected. "
            "Install LLVM, or select one with LLVM_CONFIG, LLVM_DIR or CMAKE_PREFIX_PATH.")
    endif()
    if(_sultanc_unlinked)
        message(STATUS "SultanC LLVM installations without linking libraries: ${_sultanc_unlinked}")
    endif()
    _sultanc_llvm_includes("${_sultanc_selected_prefix}" "${_sultanc_selected_version}"
        _sultanc_selected_include)
    set(_sultanc_selection_source "discovery")
endif()

# An explicit include override is exactly the given directory list.
if(_sultanc_pin_include)
    set(_sultanc_selected_include "${_sultanc_pin_include}")
endif()
# Include directories never qualify or disqualify an installation: selection
# rests on the version and the linkable library alone. When the metadata names
# no include directory, SultanC::LLVM carries none and the C compiler's own
# search paths apply.

set(SULTANC_LLVM_INCLUDE_DIR "${_sultanc_selected_include}" CACHE STRING
    "Selected LLVM include directories (a list)" FORCE)
set(SULTANC_LLVM_LIBRARY "${_sultanc_selected_library}" CACHE FILEPATH
    "Selected LLVM shared library" FORCE)
if(NOT _sultanc_pin_include)
    set(_SULTANC_LLVM_AUTO_INCLUDE_DIR "${SULTANC_LLVM_INCLUDE_DIR}"
        CACHE INTERNAL "Automatically selected LLVM include directory" FORCE)
endif()
if(NOT _sultanc_pin_library)
    set(_SULTANC_LLVM_AUTO_LIBRARY "${SULTANC_LLVM_LIBRARY}"
        CACHE INTERNAL "Automatically selected LLVM shared library" FORCE)
endif()
set(SULTANC_LLVM_VERSION_MAJOR "${_sultanc_selected_version}")
add_library(SultanC::LLVM SHARED IMPORTED GLOBAL)
set_target_properties(SultanC::LLVM PROPERTIES
    IMPORTED_LOCATION "${SULTANC_LLVM_LIBRARY}")
if(SULTANC_LLVM_INCLUDE_DIR)
    set_target_properties(SultanC::LLVM PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${SULTANC_LLVM_INCLUDE_DIR}")
endif()
message(STATUS "SultanC selected LLVM ${SULTANC_LLVM_VERSION_MAJOR} (${_sultanc_selection_source}): ${_sultanc_selected_prefix}")
message(STATUS "SultanC LLVM library: ${SULTANC_LLVM_LIBRARY}")
if(SULTANC_LLVM_INCLUDE_DIR)
    message(STATUS "SultanC LLVM include: ${SULTANC_LLVM_INCLUDE_DIR}")
else()
    message(STATUS "SultanC LLVM include: none reported by the installation metadata")
endif()
