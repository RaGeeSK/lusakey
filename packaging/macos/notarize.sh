#!/usr/bin/env bash
# Codesign, package, notarize, and staple the lusakey.app bundle produced by
# `macdeployqt`.
#
# Prerequisites:
#   - A "Developer ID Application" certificate installed in the local keychain.
#   - A notarytool keychain profile set up once via:
#       xcrun notarytool store-credentials "lusakey-notary" \
#         --apple-id <your-apple-id-email> --team-id <TEAMID> \
#         --password <app-specific-password>
#   - Edit SIGNING_IDENTITY below to match your actual certificate name
#     (`security find-identity -v -p codesigning` lists installed identities).
#
# Usage: ./notarize.sh /path/to/build/lusakey.app
set -euo pipefail

APP_PATH="${1:?Usage: $0 /path/to/lusakey.app}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SIGNING_IDENTITY="Developer ID Application: YOUR NAME (TEAMID)"
ENTITLEMENTS="$SCRIPT_DIR/entitlements.plist"
KEYCHAIN_PROFILE="lusakey-notary"

echo "Codesigning $APP_PATH ..."
codesign --force --deep --options runtime \
    --entitlements "$ENTITLEMENTS" \
    --sign "$SIGNING_IDENTITY" \
    "$APP_PATH"

DMG_PATH="${APP_PATH%.app}.dmg"
echo "Building $DMG_PATH ..."
hdiutil create -volname "lusakey" -srcfolder "$APP_PATH" -ov -format UDZO "$DMG_PATH"

echo "Submitting for notarization ..."
xcrun notarytool submit "$DMG_PATH" --keychain-profile "$KEYCHAIN_PROFILE" --wait

echo "Stapling ticket ..."
xcrun stapler staple "$DMG_PATH"

echo "Done: $DMG_PATH"
