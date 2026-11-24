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

## Réglages avant lancement — version 1.4.1-blur-bars.3

Dans la configuration d’un jeu → **Graphismes**, sélectionner le serveur X11
et le moteur d’affichage Vulkan. La section **Bandes de ratio floutées** propose
l’activation et les trois paramètres. Enregistrer pour les appliquer au
conteneur ; annuler conserve les valeurs précédentes. Les paramètres par
défaut utilisent la même section pour les nouveaux conteneurs. Les jeux déjà
configurés gardent leurs valeurs. Le menu rapide en jeu lit les mêmes clés.
La sauvegarde/recréation du dialogue conserve ces réglages sans écraser les
autres effets. Les exports incluent les quatre clés dans extraData ; l’import
d’une configuration compatible les reprend également.

## Artworks des Custom Games — version 1.4.1-blur-bars.3

Les écritures de métadonnées sont synchronisées et atomiques : les cartes ne
peuvent plus lire un identifiant absent au milieu d’une mise à jour SteamGridDB.
La migration du format entier ancien ne réentre plus récursivement dans son
lecteur. Les champs de profil inconnus sont conservés. Les éléments scannés
portent leur dossier et leurs images ; un échec de recherche d’identifiant ne
reste pas mémorisé indéfiniment après l’arrivée des métadonnées.

Les cartes, la liste et la fiche observent la même révision des images. Une
image arrivée après l’ouverture provoque une nouvelle résolution ; une réponse
du jeu précédent possède son propre état et ne peut pas peindre le jeu courant.
Les téléchargements publient un fichier complet par remplacement et les clés
Coil changent même si le nom, la taille et la date du fichier restent identiques.
Les images locales coverh/coverv/cover restent prioritaires sur SteamGridDB.

## Démarrage et connexion — version 1.4.1-blur-bars.2

Dans **Paramètres → Interface**, trois interrupteurs globaux indépendants :

- **Passer l’écran de connexion au démarrage** : mémorise l’équivalent de
  « Skip login » et ouvre la bibliothèque. Une session Steam déjà connectée
  reste en ligne ; les connexions manuelles restent accessibles depuis Comptes.
- **Désactiver la recherche de mises à jour** : bloque la requête HTTP de
  vérification de l’application et ses invitations de mise à jour.
- **Masquer les bannières de connexion** : cache les états connexion/déconnexion
  Steam et le texte de déconnexion du formulaire ; les commandes manuelles de
  connexion restent disponibles.

Depuis la version **1.4.1-blur-bars.4**, ces trois options sont **activées par
défaut** dès le premier démarrage, ou lors d’une mise à jour si leurs préférences
sont absentes. Sans compte, la bibliothèque s’ouvre directement. Un choix déjà
enregistré, y compris désactivé, reste respecté. Les trois interrupteurs dans
Paramètres → Interface permettent toujours de revenir au comportement souhaité.
Les comptes, conteneurs et sauvegardes sont conservés.

## Installation à côté de l’original — version .5

Le package est désormais **app.gamenative.blurbars**, affiché **GameNative Blur
Bars**. Il cohabite avec l’application originale ; aucune désinstallation de
celle-ci n’est nécessaire. La racine publique propre au fork est
**GameNativeBlurBars**, afin de ne pas importer automatiquement la bibliothèque
originale. Les dossiers choisis explicitement restent ajoutables.

La variante **legacy** conserve glibc et Proton bionic. La variante modern est
réservée aux conteneurs bionic. Pour importer un conteneur existant, copier son
préfixe réel, son .container et ses profils de commandes référencés, puis fournir
les runtimes correspondants. Un simple export JSON ne copie pas Wine, le registre,
les DLL, les polices et les sauvegardes privées.

Les références privées de l’ancien package dans les lecteurs et variables sont
résolues vers le fork au lancement, sans réécrire le JSON importé. Les chemins
de jeux publics restent identiques. Les chemins IPC et cache utilisent le
répertoire du package courant. Une copie ne constitue pas une validation en jeu.

## Reconstruire

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
./gradlew.bat :app:assembleModernDebug :app:testModernDebugUnitTest --tests app.gamenative.ui.util.BlurBarsConfigTest --tests app.gamenative.ui.util.BlurBarsDialogTest --tests app.gamenative.ui.util.ArtworkImageCacheTest --tests app.gamenative.utils.CustomArtworkMetadataTest --tests app.gamenative.utils.CustomArtworkResolutionTest --tests app.gamenative.ui.component.CustomArtworkRefreshTest --tests app.gamenative.QuietStartupSettingsTest --tests app.gamenative.utils.QuietStartupUpdateTest --max-workers=2
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
Les tests de démarrage couvrent aussi le choix de la bibliothèque avec/sans
comptes, les trois préférences persistantes et réversibles, les états des
bannières et **zéro requête HTTP** quand la recherche de mise à jour est coupée.

Ce test de rendu de production sur ordinateur et une compilation ARM64 ne
constituent pas une mesure de fluidité, de consommation ou une validation des
entrées en jeu sur un appareil Android. Aucun appareil n'a été modifié pour
développer cette version. L'essai physique Android reste à réaliser.

### Régressions Custom Games et dialogue

Dix tests supplémentaires couvrent les lectures concurrentes pendant 400 mises
à jour, la migration héritée, la conservation des champs inconnus, un échec de
cache suivi de métadonnées tardives, les images des éléments scannés, le
rafraîchissement d’une fiche ouverte et du modèle de carte, les réponses A/B
retardées, le vrai décodage Coil après remplacement à nom/taille/date identiques,
ainsi que la sauvegarde réelle d’un conteneur, les valeurs par défaut et la
recréation des anciens dialogues. Avec les sept tests précédents, 17 tests
ciblés passent. Ces essais Robolectric sur ordinateur ne constituent pas une
acceptation en jeu sur un appareil Android.
