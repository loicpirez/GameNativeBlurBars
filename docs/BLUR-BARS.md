# GameNative Blur Bars — Vulkan

Fork de [GameNative](https://github.com/utkarshdalal/GameNative), base
`344f706aea6533a3b25ec5b419bb6c2fe5c384b1`, licence GPL-3.0 inchangée.

## Utilisation

Dans un jeu utilisant le **rendu Vulkan de GameNative**, ouvrir le menu rapide,
puis Affichage → Effets → **Bandes de ratio floutées**. Activer le remplissage
et régler le rayon (0–120 px à 1080p), la largeur du bord prélevé (1–50 %) et la
luminosité (0–100 %). Valeurs initiales : 48 px, 12 %, 75 %. Les quatre réglages
sont sauvegardés par conteneur. L'ancien comportement est conservé tant que
l'option reste désactivée.

Utiliser un mode qui préserve le ratio : Adapter, Nearest/Linear ou FSR avec
ratio. Remplir/Étirer n'ont généralement plus de marges à décorer. La présence
de Vulkan dans DXVK ne suffit pas si la **sortie** est confiée à Gamescope : ce
fork cible le compositeur Vulkan de GameNative. OpenGL, SurfaceFlinger et le
rendu XR immersif ne reçoivent pas cet effet.

Le centre garde les pixels produits par le rendu existant. La scène entière,
sans le curseur, alimente un flou séparable à quart de résolution. Seules les
marges reçoivent les couleurs de leurs bords voisins ; le curseur est composé
ensuite. Les transformations souris/tactile et les autres effets restent ceux
de GameNative. L'activation quitte le scanout direct ; sa désactivation le rend
à nouveau possible si aucun autre effet ne demande le compositeur.

## Construction

Le dépôt suit le modèle amont : les bibliothèques natives sont fournies dans
les dossiers JNI de chaque variante. Les deux fichiers ARM64 de ce fork ont
été reconstruits depuis les sources modifiées. Pour les reconstruire après
une modification C++/GLSL, **exécuter cette étape avant Gradle** :

```powershell
git submodule update --init app/src/main/cpp/extras/adrenotools app/src/main/cpp/lsfg-vk-android snappy-java/upstream iq80-leveldb/upstream
git -C app/src/main/cpp/extras/adrenotools submodule update --init
git -C app/src/main/cpp/lsfg-vk-android submodule update --init thirdparty/dxbc
git -C snappy-java/upstream submodule update --init android/snappy
tools/build-blur-bars-native.ps1 -Sdk <Android-SDK> -Glslang <glslang.exe>
./gradlew.bat :app:assembleModernDebug :app:testModernDebugUnitTest --tests app.gamenative.ui.util.BlurBarsConfigTest --max-workers=2
```

Outils : JDK 17, SDK 36, build-tools 36.0.0, NDK 27.3.13750724, CMake 3.22.1,
glslang 16.6.0. `sdk.dir` dans `local.properties` peut remplacer `-Sdk`.
Le script natif construit uniquement le compositeur Vulkan et ses dépendances,
puis remplace les bibliothèques ARM64 modern et legacy. Aucune clé de signature
de production n'est requise. L'APK de test est signé par la clé debug locale ;
il ne constitue pas une mise à jour signée par l'éditeur officiel.

## Vérification

`tests/blur-bars` compile **le même** `BlurBarsVulkan.cpp` et les mêmes shaders
que l'APK. Le programme crée un périphérique Vulkan hors écran, active les
couches Khronos et leur validation de synchronisation, puis vérifie le centre
pixel par pixel et les marges. Il n'ouvre aucune fenêtre ni ne génère d'entrée.

```sh
cmake -S tests/blur-bars -B build/blur-bars-test -DVULKAN_HEADERS=<headers> -DGLSLANG_VALIDATOR=<glslang>
cmake --build build/blur-bars-test
# Installer/enregistrer VK_LAYER_KHRONOS_validation dans le loader de test.
./build/blur-bars-test/blur-bars-test
```

Critères : centre inchangé ; absence de noir provenant des marges dans les
prélèvements ; bandes horizontales/verticales et décentrage ; rayon, largeur et
luminosité indépendants ; tailles impaires, marges d'un pixel, redimensionnement
et alternance des deux images en vol ; zéro erreur de validation Vulkan.
Les tests Android ciblés couvrent valeurs par défaut, persistance des quatre
champs, autres réglages conservés, désactivation et données invalides.

Ce test de rendu de production sur ordinateur et une compilation ARM64 ne
constituent pas une mesure de fluidité, de consommation ou une validation des
entrées en jeu sur un appareil Android. Aucun appareil n'a été modifié pour
développer cette version. L'essai physique Android reste à réaliser.
