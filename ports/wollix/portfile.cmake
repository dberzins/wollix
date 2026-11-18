# Header-only: one build type is enough, and there is no debug tree to
# install.
set(VCPKG_BUILD_TYPE release)

# REF is the release tag; SHA512 is the hash of the GitHub source archive
# for that tag and is filled in when the tag exists (leave 0 until then;
# vcpkg prints the actual hash on the first failing fetch).
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO dberzins/wollix
    REF v0.10.0
    SHA512 63ef5b73fb9ad37c553bce2fc97237afa54fa19e002632f7ff87f1ce659dfaefd75145d04a9a4a7212a3ba5c60763e008ba783730f9c0e9abf7f6bc98df090d9
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
