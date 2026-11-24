# MerleauRack Channel

Channel strip modulaire au look de console analogique : **Preamp, Gate, EQ 4 bandes, Compresseur, De-esser** + section Master.
Les modules se déplacent par glisser-déposer (comme le Scheps Omni Channel) et l'ordre est sauvegardé avec le projet.
Formats Mac : **AU, VST3 et application autonome**, binaire universel (Apple Silicon + Intel), macOS 11 ou plus.

## Obtenir le .dmg (sans Mac de développement, gratuit)

1. Crée un compte sur https://github.com (gratuit).
2. Clique sur **New repository**, nomme-le `MerleauRackChannel`, choisis **Private**, puis **Create repository**.
3. Clique sur **uploading an existing file** et glisse tout le **contenu** de ce dossier
   (CMakeLists.txt, Source, scripts, tests, .github, README.md).
   Le dossier `.github` est caché dans le Finder : appuie sur **Cmd + Maj + .** pour l'afficher avant de le glisser.
   Clique sur **Commit changes**.
4. Va dans l'onglet **Actions** : le build « Build macOS DMG » démarre tout seul (environ 10 à 15 min).
   S'il ne démarre pas, clique sur « Build macOS DMG » puis **Run workflow**.
5. Quand la coche est verte, ouvre le build, descends à **Artifacts** et télécharge
   **MerleauRack-Channel-macOS**. Le zip contient le `.dmg`.

## Compiler soi-même sur un Mac (option)

Il faut Xcode (App Store) et CMake (`brew install cmake`).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --config Release --parallel
bash scripts/make_dmg.sh        # -> dist/MerleauRack-Channel-1.0.0-macOS.dmg
```

## Signature Apple

Le build est signé « ad hoc » : il fonctionne, mais macOS affiche un avertissement à l'ouverture du .pkg
(clic droit > Ouvrir). Pour une distribution publique sans avertissement, il faut un compte Apple Developer
(99 $/an), signer avec `DEVELOPER_ID="Developer ID Application: ..."` puis notariser.

## Structure

- `Source/DSP/MerleauDSP.h` : traitement audio (C++ pur, testé : `g++ -std=c++17 tests/dsp_test.cpp`)
- `Source/PluginProcessor.*` : paramètres, ordre des modules, sauvegarde
- `Source/PluginEditor.*`, `Source/UI/*` : interface rack (boutons, vu-mètres, glisser-déposer)
- `.github/workflows/build-macos.yml` : build automatique du .dmg sur un Mac de GitHub
