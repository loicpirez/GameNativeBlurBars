package app.gamenative.ui.util

import android.content.Context
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.platform.LocalContext
import androidx.lifecycle.compose.LocalLifecycleOwner
import coil.request.ImageRequest

/** A local refresh must change Coil's key even if a file keeps its size/mtime. */
internal fun artworkImageRequest(context: Context, data: Any?): ImageRequest =
    ImageRequest.Builder(context).data(data).apply {
        if (data is String && data.startsWith("file:") && data.contains("artwork=")) {
            memoryCacheKey(data)
            diskCacheKey(data)
        }
    }.build()

@Composable
internal fun rememberArtworkImageRequest(data: Any?): ImageRequest {
    val context = LocalContext.current
    val owner = LocalLifecycleOwner.current
    return remember(data, context, owner) {
        artworkImageRequest(context, data).newBuilder().lifecycle(owner).build()
    }
}
