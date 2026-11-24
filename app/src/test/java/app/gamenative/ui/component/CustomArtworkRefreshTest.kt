package app.gamenative.ui.component

import android.app.Application
import android.content.Context
import android.graphics.Bitmap
import androidx.compose.material3.Text
import androidx.compose.ui.test.junit4.createComposeRule
import androidx.test.core.app.ApplicationProvider
import app.gamenative.PluviaApp
import app.gamenative.PrefManager
import app.gamenative.data.GameSource
import app.gamenative.data.LibraryItem
import app.gamenative.events.AndroidEvent
import app.gamenative.service.DownloadService
import app.gamenative.ui.screen.library.appscreen.CustomGameAppScreen
import app.gamenative.utils.CustomGameScanner
import app.gamenative.utils.FakeDataStore
import app.gamenative.utils.GameMetadata
import app.gamenative.utils.GameMetadataManager
import app.gamenative.utils.awaitUntil
import app.gamenative.utils.installFakePrefManager
import kotlinx.coroutines.runBlocking
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertEquals
import app.gamenative.utils.CustomGameArtwork
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config
import org.robolectric.annotation.GraphicsMode
import java.io.File
import java.util.concurrent.atomic.AtomicReference
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import kotlinx.coroutines.flow.MutableStateFlow
import app.gamenative.ui.util.rememberCustomArtwork
import app.gamenative.utils.CustomArtworkSnapshot
import app.gamenative.ui.screen.library.components.customGridImageUrls
import app.gamenative.ui.enums.PaneType
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.withContext

@RunWith(RobolectricTestRunner::class)
@Config(application = Application::class, sdk = [29])
@GraphicsMode(GraphicsMode.Mode.NATIVE)
class CustomArtworkRefreshTest {
    @get:Rule val compose = createComposeRule()

    @Test fun openCustomGamePagePicksUpArtworkThatArrivesAfterFirstComposition() {
        installFakePrefManager(FakeDataStore()).use {
            val context = ApplicationProvider.getApplicationContext<Context>()
            DownloadService.populateDownloadService(context)
            val folder = File(context.filesDir, "artwork-late").apply { mkdirs() }
            GameMetadataManager.write(folder, GameMetadata(731))
            PrefManager.customGameManualFolders = setOf(folder.absolutePath)
            awaitUntil { PrefManager.customGameManualFolders == setOf(folder.absolutePath) }
            CustomGameScanner.invalidateCache()
            assertEquals(folder.absolutePath, CustomGameScanner.getFolderPathFromAppId("CUSTOM_GAME_731"))
            val hero = AtomicReference<String?>()
            val location = AtomicReference<String?>()
            val gridHero = AtomicReference<String?>()
            val item = LibraryItem(appId = "CUSTOM_GAME_731", name = "Fixture", gameSource = GameSource.CUSTOM_GAME)
            val screen = CustomGameAppScreen()
            compose.setContent {
                val info = screen.getGameDisplayInfo(context, item)
                hero.set(info.heroImageUrl)
                location.set(info.installLocation)
                val art = rememberCustomArtwork(item)
                gridHero.set(art?.let { customGridImageUrls(it, item, PaneType.GRID_HERO).primary })
                Text(info.name)
            }
            compose.waitForIdle()
            compose.waitUntil(5_000) { location.get() == folder.absolutePath }
            val bitmap = Bitmap.createBitmap(32, 16, Bitmap.Config.ARGB_8888)
            bitmap.eraseColor(android.graphics.Color.RED)
            File(folder, "coverh.png").outputStream().use { bitmap.compress(Bitmap.CompressFormat.PNG, 100, it) }
            val oldRevision = CustomGameArtwork.revisions.value[item.appId] ?: 0L
            runBlocking { PluviaApp.events.emit(AndroidEvent.CustomGameImagesFetched(item.appId)) }
            assertEquals(oldRevision + 1, CustomGameArtwork.revisions.value[item.appId])
            assertNotNull(CustomGameArtwork.resolve(item).hero)
            compose.waitUntil(5_000) { !hero.get().isNullOrBlank() && !gridHero.get().isNullOrBlank() }
            assertNotNull(hero.get())
            assertEquals(hero.get(), gridHero.get())
        }
    }

    @Test fun lateArtworkFromThePreviousGameCannotPaintTheNewGame() {
        val selected = MutableStateFlow(LibraryItem(appId = "CUSTOM_GAME_1", gameSource = GameSource.CUSTOM_GAME))
        val started = CompletableDeferred<Unit>()
        val finishA = CompletableDeferred<Unit>()
        val finishedA = CompletableDeferred<Unit>()
        val startedB = CompletableDeferred<Unit>()
        val hero = AtomicReference<String?>()
        compose.setContent {
            val current by selected.collectAsState()
            val artwork = rememberCustomArtwork(current, resolve = { item, _ ->
                if (item.gameId == 1) withContext(NonCancellable) {
                    started.complete(Unit)
                    finishA.await()
                    finishedA.complete(Unit)
                    CustomArtworkSnapshot(hero = "A")
                } else {
                    startedB.complete(Unit)
                    CustomArtworkSnapshot(hero = "B")
                }
            })
            hero.set(artwork?.hero)
            Text(artwork?.hero.orEmpty())
        }
        try {
            compose.waitUntil(5_000) { started.isCompleted }
            compose.runOnIdle { selected.value = selected.value.copy(appId = "CUSTOM_GAME_2") }
            assertEquals("CUSTOM_GAME_2", selected.value.appId)
            compose.waitUntil(5_000) { startedB.isCompleted }
            compose.waitUntil(5_000) { hero.get() == "B" }
        } finally { finishA.complete(Unit) }
        compose.waitUntil(5_000) { finishedA.isCompleted }
        compose.waitForIdle()
        assertEquals("B", hero.get())
    }
}
