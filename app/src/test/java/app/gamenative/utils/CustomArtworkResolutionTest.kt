package app.gamenative.utils

import android.app.Application
import android.content.Context
import androidx.test.core.app.ApplicationProvider
import app.gamenative.PrefManager
import app.gamenative.data.GameSource
import app.gamenative.data.LibraryItem
import app.gamenative.ui.enums.PaneType
import app.gamenative.ui.screen.library.components.getGridImageUrl
import org.junit.Assert.*
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config
import java.io.File

@RunWith(RobolectricTestRunner::class)
@Config(application = Application::class, sdk = [29])
class CustomArtworkResolutionTest {
    @get:Rule val temp = TemporaryFolder()

    @Test fun cachedMissRecoversWhenMetadataAndArtworkArriveWithTheSameFolderSet() {
        installFakePrefManager(FakeDataStore()).use {
            val folder = temp.newFolder("game")
            PrefManager.customGameManualFolders = setOf(folder.absolutePath)
            awaitUntil { PrefManager.customGameManualFolders == setOf(folder.absolutePath) }
            CustomGameScanner.invalidateCache()
            assertNull(CustomGameScanner.getFolderPathFromAppId("CUSTOM_GAME_731"))
            GameMetadataManager.write(folder, GameMetadata(731))
            CustomGameArtwork.writeImage(File(folder, "CoVeRh.PNG"), byteArrayOf(1, 2, 3))
            CustomGameArtwork.writeImage(File(folder, "coverv.webp"), byteArrayOf(4, 5))
            val item = LibraryItem(appId = "CUSTOM_GAME_731", gameSource = GameSource.CUSTOM_GAME)
            val context = ApplicationProvider.getApplicationContext<Context>()
            assertTrue(getGridImageUrl(context, item, PaneType.GRID_HERO).primary.contains("CoVeRh.PNG"))
            assertTrue(getGridImageUrl(context, item, PaneType.GRID_CAPSULE).primary.contains("coverv.webp"))
            assertEquals(folder.absolutePath, CustomGameArtwork.resolve(item).folderPath)
        }
    }

    @Test fun scannedItemsCarryTheirFolderAndArtAndDoNotChangeStoredGameIdentity() {
        installFakePrefManager(FakeDataStore()).use {
            val folder = temp.newFolder("scan")
            GameMetadataManager.write(folder, GameMetadata(731))
            File(folder, "cover.png").writeBytes(byteArrayOf(1))
            File(folder, "coverh.png").writeBytes(byteArrayOf(2))
            File(folder, "steamgriddb_grid_hero_731.png").writeBytes(byteArrayOf(3))
            PrefManager.customGameManualFolders = setOf(folder.absolutePath)
            awaitUntil { PrefManager.customGameManualFolders == setOf(folder.absolutePath) }
            CustomGameScanner.invalidateCache()
            val item = CustomGameScanner.createLibraryItemFromFolder(folder.absolutePath)!!
            assertEquals("CUSTOM_GAME_731", item.appId)
            assertEquals(folder.absolutePath, item.customGameFolderPath)
            assertTrue(item.heroImageUrl.contains("coverh.png"))
            assertTrue(item.capsuleImageUrl.contains("cover.png"))
            assertEquals(731, GameMetadataManager.getAppId(folder))
        }
    }
}
