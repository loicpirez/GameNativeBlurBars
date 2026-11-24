package app.gamenative.utils

import android.app.Application
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config
import java.io.File
import java.util.concurrent.CountDownLatch
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicInteger

@RunWith(RobolectricTestRunner::class)
@Config(application = Application::class, sdk = [29])
class CustomArtworkMetadataTest {
    @get:Rule val temp = TemporaryFolder()

    @Test fun concurrentArtworkMetadataUpdatesNeverHideTheStoredGameId() {
        val folder = temp.newFolder("custom")
        GameMetadataManager.write(folder, GameMetadata(731))
        val start = CountDownLatch(1)
        val missing = AtomicInteger()
        val threads = Executors.newFixedThreadPool(2)
        try {
            val writer = threads.submit {
                start.await()
                repeat(400) { GameMetadataManager.update(folder, releaseDate = 1000L + it) }
            }
            val reader = threads.submit {
                start.await()
                repeat(1200) { if (GameMetadataManager.getAppId(folder) != 731) missing.incrementAndGet() }
            }
            start.countDown()
            writer.get(30, TimeUnit.SECONDS)
            reader.get(30, TimeUnit.SECONDS)
            assertEquals("Artwork readers observed an absent/wrong custom ID during metadata updates", 0, missing.get())
        } finally { threads.shutdownNow() }
    }

    @Test fun legacyIdMigrationDoesNotReenterTheWriterRecursively() {
        val folder = temp.newFolder("legacy")
        File(folder, ".gamenative").writeText("731")
        assertEquals(731, GameMetadataManager.getAppId(folder))
        assertTrue(File(folder, ".gamenative").readText().startsWith("{"))
    }

    @Test fun artworkUpdatesPreserveUnknownProfileFieldsAndCanResetTheFetchFlag() {
        val folder = temp.newFolder("preserved")
        val file = File(folder, ".gamenative")
        file.writeText("""{"appId":731,"steamgriddbFetched":true,"futureProfile":{"keep":42}}""")
        GameMetadataManager.update(folder, releaseDate = 12345, steamgriddbFetched = false)
        val result = org.json.JSONObject(file.readText())
        assertEquals(42, result.getJSONObject("futureProfile").getInt("keep"))
        assertEquals(GameMetadata(731, false, 12345), GameMetadataManager.read(folder))
    }
}
