#!/bin/bash

if [ $# -eq 0 ]; then
    echo "The release version is required as an argument"
    exit 1
fi

PROJECT_DIR=$PWD

export RELEASE_VERSION="$1"
export PATH_TO_JUCE="$PROJECT_DIR/JUCE"

SIGN_ID="Developer ID Application: Uwyn, LLC (AGZT8GVS7G)"
BUILD_DIR="$PWD/Builds/MacOSX/build"
INSTALLERS_DIR="$PROJECT_DIR/Installers"
PKG_FILE="sendmidi-macos-${RELEASE_VERSION}.pkg"
ARCHIVE_FILE="$INSTALLERS_DIR/sendmidi-macOS-${RELEASE_VERSION}.zip"

rm -rfv "$BUILD_DIR"
mkdir -v "$INSTALLERS_DIR"

# build release artifacts
echo "Building all ProJucer artifacts"
xcodebuild -project ./Builds/MacOSX/sendmidi.xcodeproj -destination "generic/platform=macOS,name=Any Mac" -config Release SYMROOT=build -scheme "sendmidi - ConsoleApp"

# sign each individual artifact
echo "Codesigning all artifacts"
codesign -f -s "$SIGN_ID" "${BUILD_DIR}/Release/sendmidi" --deep --strict --timestamp --options=runtime

# build the installer package in the build directory
echo "Building installer package"
pkgbuild --root "${BUILD_DIR}/Release" --identifier com.uwyn.sendmidi --version ${RELEASE_VERSION} --install-location /usr/local/bin --sign "Developer ID Installer: Uwyn, LLC (AGZT8GVS7G)" "${BUILD_DIR}/${PKG_FILE}"

# Notarization
echo "Notarizating installer"
xcrun notarytool submit "${BUILD_DIR}/${PKG_FILE}" --keychain-profile "notary-uwyn.com" --wait
xcrun stapler staple "${BUILD_DIR}/${PKG_FILE}"
spctl --assess -vv --type install "${BUILD_DIR}/${PKG_FILE}"

echo "Creating zip archive of installer"
pushd "${BUILD_DIR}"
zip -r  "${ARCHIVE_FILE}" "${PKG_FILE}"
popd

echo "Finished building `realpath $ARCHIVE_FILE`"
