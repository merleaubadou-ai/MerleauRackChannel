#!/bin/bash
# Cree "MerleauRack Channel" .dmg : installeur .pkg (AU + VST3 + app) + Lisez-moi
set -euo pipefail

VERSION="1.0.0"
NAME="MerleauRack Channel"
ART="build/MerleauRackChannel_artefacts/Release"
AU="$ART/AU/$NAME.component"
VST3="$ART/VST3/$NAME.vst3"
APP="$ART/Standalone/$NAME.app"

rm -rf dist && mkdir -p dist/pkgroot dist/stage

# Signature "ad hoc" (obligatoire sur Apple Silicon). Si DEVELOPER_ID est defini,
# signe avec ton certificat Apple Developer a la place.
SIGN_ID="${DEVELOPER_ID:--}"
for b in "$AU" "$VST3" "$APP"; do
  codesign --force --deep --timestamp=none -s "$SIGN_ID" "$b"
done

ROOT="dist/pkgroot"
mkdir -p "$ROOT/Library/Audio/Plug-Ins/Components" "$ROOT/Library/Audio/Plug-Ins/VST3" "$ROOT/Applications"
cp -R "$AU"   "$ROOT/Library/Audio/Plug-Ins/Components/"
cp -R "$VST3" "$ROOT/Library/Audio/Plug-Ins/VST3/"
cp -R "$APP"  "$ROOT/Applications/"

# Empeche l'installeur de "deplacer" les bundles vers une ancienne copie
pkgbuild --analyze --root "$ROOT" dist/components.plist
i=0
while /usr/libexec/PlistBuddy -c "Print :$i" dist/components.plist >/dev/null 2>&1; do
  /usr/libexec/PlistBuddy -c "Set :$i:BundleIsRelocatable false" dist/components.plist
  i=$((i+1))
done

pkgbuild --root "$ROOT" --component-plist dist/components.plist \
         --identifier com.merleauaudio.merleaurackchannel.pkg \
         --version "$VERSION" --install-location / \
         "dist/stage/Installer $NAME.pkg"

cp "scripts/Lisez-moi.txt" "dist/stage/Lisez-moi.txt"

hdiutil create -volname "$NAME" -srcfolder dist/stage -ov -format UDZO \
        "dist/MerleauRack-Channel-$VERSION-macOS.dmg"

echo "DMG pret : dist/MerleauRack-Channel-$VERSION-macOS.dmg"
