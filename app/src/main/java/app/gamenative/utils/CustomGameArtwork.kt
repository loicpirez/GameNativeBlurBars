package app.gamenative.utils

import android.net.Uri
import app.gamenative.PluviaApp
import app.gamenative.data.LibraryItem
import app.gamenative.events.AndroidEvent
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import java.io.File
import java.util.Locale
import java.nio.file.Files
import java.nio.file.StandardCopyOption
import java.nio.file.AtomicMoveNotSupportedException

data class CustomArtworkSnapshot(
    val folderPath: String? = null,
    val hero: String? = null,
    val capsule: String? = null,
    val header: String? = null,
    val logo: String? = null,
    val releaseDate: Long = 0L,
    val revision: Long = 0L,
)

/** One source of local artwork for library cards and the custom game page. */
object CustomGameArtwork {
    private val generations = MutableStateFlow<Map<String, Long>>(emptyMap())
    val revisions = generations.asStateFlow()

    init {
        // Registered before the first resolution starts. StateFlow retains a
        // refresh even when a card/page is between compositions or off-screen.
        val listener: (AndroidEvent.CustomGameImagesFetched) -> Unit = { invalidate(it.appId) }
        PluviaApp.events.on<AndroidEvent.CustomGameImagesFetched, Unit>(listener)
    }

    fun invalidate(appId: String) {
        CustomGameScanner.invalidateCache()
        generations.update { it + (appId to ((it[appId] ?: 0L) + 1L)) }
    }

    fun resolve(item: LibraryItem, revision: Long = revisions.value[item.appId] ?: 0L): CustomArtworkSnapshot {
        val direct = item.customGameFolderPath?.let(::File)?.takeIf {
            it.isDirectory && GameMetadataManager.getAppId(it)?.let { id -> id == item.gameId } != false
        }
        val folder = direct ?: CustomGameScanner.getFolderPathFromAppId(item.appId)?.let(::File)
            ?: return CustomArtworkSnapshot()
        return fromFolder(folder, revision)
    }

    fun fromFolder(folder: File, revision: Long = 0L): CustomArtworkSnapshot {
        val files = folder.listFiles().orEmpty().filter { it.isFile && it.length() > 0L }
            .sortedWith(compareBy({ it.name.lowercase(Locale.ROOT) }, { it.name }))
        fun downloaded(type: String): String? = files.firstOrNull {
            it.name.startsWith("steamgriddb_$type", ignoreCase = true) &&
                it.extension.lowercase(Locale.ROOT) in setOf("png", "jpg", "jpeg", "webp")
        }?.let { versionedUri(Uri.fromFile(it).toString(), revision) }
        fun complete(url: String?): String? = url?.let { versionedUri(it, revision) }
        val wide = complete(CustomGameScanner.findHeroCoverInFolder(folder))
        val tall = complete(CustomGameScanner.findCapsuleCoverInFolder(folder))
        val header = wide ?: downloaded("hero") ?: downloaded("grid_hero")
        return CustomArtworkSnapshot(
            folderPath = folder.absolutePath,
            hero = wide ?: downloaded("grid_hero") ?: header,
            capsule = tall ?: downloaded("grid_capsule"),
            header = header,
            logo = downloaded("logo"),
            releaseDate = GameMetadataManager.getReleaseDate(folder) ?: 0L,
            revision = revision,
        )
    }

    fun versionedUri(url: String, revision: Long): String {
        val uri = Uri.parse(url)
        if (uri.scheme != "file") return url
        val file = uri.path?.let(::File) ?: return url
        return uri.buildUpon().clearQuery()
            .appendQueryParameter("artwork", "$revision:${file.lastModified()}:${file.length()}")
            .build().toString()
    }

    /** Publish a complete download by same-directory rename, never a partial PNG. */
    fun writeImage(file: File, bytes: ByteArray) {
        val temporary = File.createTempFile(".gamenative-artwork-", ".tmp", file.parentFile)
        try {
            temporary.outputStream().use { it.write(bytes) }
            try {
                Files.move(temporary.toPath(), file.toPath(), StandardCopyOption.ATOMIC_MOVE, StandardCopyOption.REPLACE_EXISTING)
            } catch (_: AtomicMoveNotSupportedException) {
                // Same directory: replacement still publishes the completed file.
                Files.move(temporary.toPath(), file.toPath(), StandardCopyOption.REPLACE_EXISTING)
            }
        } finally {
            temporary.delete()
        }
    }
}
