package app.gamenative.utils

import android.content.Context
import androidx.test.core.app.ApplicationProvider
import app.gamenative.PrefManager
import kotlinx.coroutines.runBlocking
import okhttp3.OkHttpClient
import okhttp3.mockwebserver.MockResponse
import okhttp3.mockwebserver.MockWebServer
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import java.util.concurrent.TimeUnit

@RunWith(RobolectricTestRunner::class)
class QuietStartupUpdateTest {
    @Test
    fun disablingUpdatesMakesZeroRequestsAndCanBeReenabled() = runBlocking {
        installFakePrefManager(FakeDataStore()).use {
            PrefManager.reloadQuietStartupSettings()
            MockWebServer().use { server ->
                server.start()
                val context = ApplicationProvider.getApplicationContext<Context>()
                val client = OkHttpClient.Builder().readTimeout(1, TimeUnit.SECONDS).build()
                val endpoint = server.url("/updates").toString()
                val response = """{"updateAvailable":true,"versionCode":999,"versionName":"test","downloadUrl":"https://example.com/test.apk"}"""
                PrefManager.disableUpdateChecks = true
                assertNull(UpdateChecker.checkForUpdate(context, endpoint, client))
                assertEquals(0, server.requestCount)
                PrefManager.disableUpdateChecks = false
                server.enqueue(MockResponse().setBody(response))
                assertNotNull(UpdateChecker.checkForUpdate(context, endpoint, client))
                assertEquals(1, server.requestCount)
                PrefManager.disableUpdateChecks = true
                assertNull(UpdateChecker.checkForUpdate(context, endpoint, client))
                assertEquals(1, server.requestCount)
                PrefManager.disableUpdateChecks = false
                awaitUntil { !PrefManager.getBoolean("blur_bars_disable_update_checks", true) }
            }
        }
    }
}
