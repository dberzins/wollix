# Header-only: one build type is enough, and there is no debug tree to
# install.
set(VCPKG_BUILD_TYPE release)

# REF is the release tag; SHA512 is the hash of the GitHub source archive
# for that tag and is filled in when the tag exists (leave 0 until then;
# vcpkg prints the actual hash on the first failing fetch).
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO dberzins/wollix
    REF v0.9.0
    SHA512 0
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DWOLLIX_BUILD_TESTS=OFF
        -DWOLLIX_BUILD_DEMOS=OFF
)

vcpkg_cmake_install()

# The project installs its package config under lib/cmake/wollix; vcpkg
# keeps configs under share/<port>.
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/wollix)

# Nothing but the config lived in lib/, and the project's own copy of the
# licence under share/wollix is replaced by vcpkg's copyright file.
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/lib")
file(REMOVE "${CURRENT_PACKAGES_DIR}/share/wollix/LICENSE")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
