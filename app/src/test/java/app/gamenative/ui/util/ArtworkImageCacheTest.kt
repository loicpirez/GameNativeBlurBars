package app.gamenative.ui.util

import android.app.Application
import android.content.Context
import android.graphics.Bitmap
import android.graphics.Color
import androidx.core.graphics.drawable.toBitmap
import androidx.test.core.app.ApplicationProvider
import app.gamenative.utils.CustomGameArtwork
import coil.ImageLoader
import coil.request.SuccessResult
import coil.size.Size
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.test.UnconfinedTestDispatcher
import kotlinx.coroutines.test.setMain
import kotlinx.coroutines.test.resetMain
import org.junit.Assert.*
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config
import org.robolectric.annotation.GraphicsMode
import java.io.File

@RunWith(RobolectricTestRunner::class)
@Config(application = Application::class, sdk = [29])
@GraphicsMode(GraphicsMode.Mode.NATIVE)
class ArtworkImageCacheTest {
    @get:Rule val temp = TemporaryFolder()

    @OptIn(ExperimentalCoroutinesApi::class)
    @Test fun replacementWithSamePathSizeAndTimestampDecodesNewPixelsAfterRefresh() = runBlocking {
        Dispatchers.setMain(UnconfinedTestDispatcher())
        val context = ApplicationProvider.getApplicationContext<Context>()
        val file = File(temp.newFolder(), "cover.bmp")
        // Fixed-size uncompressed BMP: different pixels, identical byte count.
        fun bmp(color: Int): ByteArray {
            val b = java.nio.ByteBuffer.allocate(58).order(java.nio.ByteOrder.LITTLE_ENDIAN)
            b.put('B'.code.toByte()).put('M'.code.toByte()).putInt(58).putInt(0).putInt(54)
            b.putInt(40).putInt(1).putInt(1).putShort(1).putShort(24).putInt(0).putInt(4)
            b.putInt(0).putInt(0).putInt(0).putInt(0)
            b.put(Color.blue(color).toByte()).put(Color.green(color).toByte()).put(Color.red(color).toByte()).put(0)
            return b.array()
        }
        CustomGameArtwork.writeImage(file, bmp(Color.RED))
        val stamp = file.lastModified()
        val url = android.net.Uri.fromFile(file).toString()
        val loader = ImageLoader.Builder(context).build()
        suspend fun pixel(revision: Long): Int {
            val versioned = CustomGameArtwork.versionedUri(url, revision)
            // Use File data for Windows-hosted Robolectric's URI mapping; retain
            // the exact production cache key and the real Coil decoder/cache.
            val request = artworkImageRequest(context, versioned).newBuilder().data(file)
                .size(Size.ORIGINAL).allowHardware(false).build()
            val result = loader.execute(request)
            assertTrue(result.toString(), result is SuccessResult)
            return (result as SuccessResult).drawable.toBitmap().getPixel(0, 0)
        }
        try {
            assertEquals(Color.RED, pixel(0))
            CustomGameArtwork.writeImage(file, bmp(Color.BLUE))
            assertEquals(58L, file.length())
            assertTrue(file.setLastModified(stamp))
            assertEquals(Color.BLUE, pixel(1))
            assertEquals(Color.RED, pixel(0)) // old memory entry proves cache was exercised.
        } finally {
            loader.shutdown()
            Dispatchers.resetMain()
        }
    }
}
