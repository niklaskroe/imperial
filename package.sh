#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# package.sh — Build Imperial in Release and produce a macOS installer .pkg
#
# Usage:
#   ./package.sh [--sign]
#
# Without --sign: produces an unsigned pkg (fine for personal use / testers).
# With    --sign: signs binaries and pkg with your Developer ID certificates.
#                 Requires DEVELOPER_ID_APP and DEVELOPER_ID_INSTALLER to be
#                 set (see "Signing" section below), and an Apple Developer
#                 Program membership ($99/yr).
#
# Output: dist/Imperial-1.0.0.pkg
# -----------------------------------------------------------------------------
set -euo pipefail

# ── Configuration ─────────────────────────────────────────────────────────────
VERSION="1.0.0"
PRODUCT="Imperial"
BUILD_DIR="build"
ARTEFACTS="$BUILD_DIR/Imperial_artefacts/Release"
STAGING="$BUILD_DIR/pkg_staging"
DIST="dist"

# ── Signing (fill these in when you have a Developer ID) ──────────────────────
# DEVELOPER_ID_APP="Developer ID Application: Your Name (XXXXXXXXXX)"
# DEVELOPER_ID_INSTALLER="Developer ID Installer: Your Name (XXXXXXXXXX)"
# NOTARIZATION_PROFILE="notarytool-profile"   # keychain profile created with:
#   xcrun notarytool store-credentials notarytool-profile \
#         --apple-id "you@example.com" --team-id XXXXXXXXXX --password <app-specific-pw>

SIGN=false
if [[ "${1:-}" == "--sign" ]]; then
    SIGN=true
    if [[ -z "${DEVELOPER_ID_APP:-}" || -z "${DEVELOPER_ID_INSTALLER:-}" ]]; then
        echo "ERROR: Set DEVELOPER_ID_APP and DEVELOPER_ID_INSTALLER before using --sign."
        exit 1
    fi
fi

# ── Helpers ───────────────────────────────────────────────────────────────────
step() { echo; echo "▶ $*"; }

sign_bundle() {
    # Signs a .app / .vst3 / .component bundle with hardened runtime
    local bundle="$1"
    if $SIGN; then
        codesign --force --deep --strict \
                 --options runtime \
                 --sign "$DEVELOPER_ID_APP" \
                 "$bundle"
        echo "  signed: $bundle"
    fi
}

# ── 1. Build Release ───────────────────────────────────────────────────────────
step "Building $PRODUCT $VERSION (Release)"
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --config Release --parallel

# ── 2. Verify artefacts exist ─────────────────────────────────────────────────
step "Checking artefacts"
VST3="$ARTEFACTS/VST3/$PRODUCT.vst3"
AU="$ARTEFACTS/AU/$PRODUCT.component"
APP="$ARTEFACTS/Standalone/$PRODUCT.app"

for f in "$VST3" "$AU" "$APP"; do
    if [[ ! -e "$f" ]]; then
        echo "ERROR: expected artefact not found: $f"
        exit 1
    fi
done
echo "  VST3, AU, Standalone — all present"

# ── 3. Sign binaries ──────────────────────────────────────────────────────────
if $SIGN; then
    step "Signing binaries"
    sign_bundle "$VST3"
    sign_bundle "$AU"
    sign_bundle "$APP"
fi

# ── 4. Stage files ────────────────────────────────────────────────────────────
step "Staging files"
rm -rf "$STAGING"

# VST3 → /Library/Audio/Plug-Ins/VST3/
VST3_STAGE="$STAGING/vst3/Library/Audio/Plug-Ins/VST3"
mkdir -p "$VST3_STAGE"
cp -R "$VST3" "$VST3_STAGE/"

# AU → /Library/Audio/Plug-Ins/Components/
AU_STAGE="$STAGING/au/Library/Audio/Plug-Ins/Components"
mkdir -p "$AU_STAGE"
cp -R "$AU" "$AU_STAGE/"

# Standalone → /Applications/
APP_STAGE="$STAGING/app/Applications"
mkdir -p "$APP_STAGE"
cp -R "$APP" "$APP_STAGE/"

# ── 5. Build component packages ───────────────────────────────────────────────
step "Building component packages"
PKGS="$BUILD_DIR/pkgs"
rm -rf "$PKGS"
mkdir -p "$PKGS"

pkgbuild \
    --root "$STAGING/vst3" \
    --identifier "com.niklaskroe.imperial.vst3" \
    --version "$VERSION" \
    --install-location "/" \
    "$PKGS/Imperial-VST3.pkg"

pkgbuild \
    --root "$STAGING/au" \
    --identifier "com.niklaskroe.imperial.au" \
    --version "$VERSION" \
    --install-location "/" \
    "$PKGS/Imperial-AU.pkg"

pkgbuild \
    --root "$STAGING/app" \
    --identifier "com.niklaskroe.imperial.app" \
    --version "$VERSION" \
    --install-location "/" \
    "$PKGS/Imperial-Standalone.pkg"

# ── 6. Assemble distribution package ─────────────────────────────────────────
step "Assembling distribution package"
mkdir -p "$DIST"
OUTPUT="$DIST/$PRODUCT-$VERSION.pkg"

# distribution.xml describes the installer UI and component selection
DIST_XML="$BUILD_DIR/distribution.xml"
cat > "$DIST_XML" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">

    <title>Imperial $VERSION</title>
    <welcome    file="welcome.rtf"    mime-type="text/rtf" />
    <license    file="license.rtf"    mime-type="text/rtf" />
    <background file="background.png" mime-type="image/png"
                alignment="bottomleft" scaling="none" />

    <options require-scripts="false" customize="always" />

    <choices-outline>
        <line choice="vst3"/>
        <line choice="au"/>
        <line choice="app"/>
    </choices-outline>

    <choice id="vst3" title="VST3 Plugin"
            description="Installs Imperial.vst3 to /Library/Audio/Plug-Ins/VST3">
        <pkg-ref id="com.niklaskroe.imperial.vst3"/>
    </choice>

    <choice id="au" title="Audio Unit (AU) Plugin"
            description="Installs Imperial.component to /Library/Audio/Plug-Ins/Components">
        <pkg-ref id="com.niklaskroe.imperial.au"/>
    </choice>

    <choice id="app" title="Standalone App"
            description="Installs Imperial.app to /Applications">
        <pkg-ref id="com.niklaskroe.imperial.app"/>
    </choice>

    <pkg-ref id="com.niklaskroe.imperial.vst3"  version="$VERSION">Imperial-VST3.pkg</pkg-ref>
    <pkg-ref id="com.niklaskroe.imperial.au"    version="$VERSION">Imperial-AU.pkg</pkg-ref>
    <pkg-ref id="com.niklaskroe.imperial.app"   version="$VERSION">Imperial-Standalone.pkg</pkg-ref>

</installer-gui-script>
XML

# Locate installer resources (welcome, license, background) if present
RESOURCES_FLAG=""
if [[ -d "installer" ]]; then
    RESOURCES_FLAG="--resources installer"
fi

productbuild \
    --distribution "$DIST_XML" \
    --package-path "$PKGS" \
    $RESOURCES_FLAG \
    ${SIGN:+--sign "$DEVELOPER_ID_INSTALLER"} \
    "$OUTPUT"

# ── 7. Notarize (signing only) ────────────────────────────────────────────────
if $SIGN; then
    step "Notarizing"
    xcrun notarytool submit "$OUTPUT" \
          --keychain-profile "${NOTARIZATION_PROFILE:?set NOTARIZATION_PROFILE}" \
          --wait
    xcrun stapler staple "$OUTPUT"
    echo "  Notarization complete and stapled."
fi

# ── Done ──────────────────────────────────────────────────────────────────────
echo
echo "✓ Installer ready: $OUTPUT"
if ! $SIGN; then
    echo
    echo "  Note: this package is unsigned. Recipients who see a Gatekeeper"
    echo "  warning can bypass it with:"
    echo "    sudo xattr -rd com.apple.quarantine $OUTPUT"
fi
