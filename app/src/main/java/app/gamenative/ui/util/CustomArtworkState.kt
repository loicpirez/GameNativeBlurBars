package app.gamenative.ui.util

import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.key
import androidx.compose.runtime.produceState
import app.gamenative.data.LibraryItem
import app.gamenative.utils.CustomArtworkSnapshot
import app.gamenative.utils.CustomGameArtwork
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/** Null is loading. Each request owns its state, so a late A cannot overwrite B. */
@Composable
internal fun rememberCustomArtwork(
    item: LibraryItem,
    refreshKey: Long = 0L,
    resolve: suspend (LibraryItem, Long) -> CustomArtworkSnapshot = { game, revision ->
        withContext(Dispatchers.IO) { CustomGameArtwork.resolve(game, revision) }
    },
): CustomArtworkSnapshot? {
    val revisions by CustomGameArtwork.revisions.collectAsState()
    val revision = revisions[item.appId] ?: 0L
    return key(item, revision, refreshKey) {
        produceState<CustomArtworkSnapshot?>(null) { value = resolve(item, revision) }.value
    }
}
