package app.gamenative.utils

import org.junit.Assert.assertEquals
import org.junit.Test

class ForkPathsTest {
    @Test fun originalPrivateDrivesResolveIntoTheForkWithoutChangingPublicGamePaths() {
        val root = "/data/user/0/app.gamenative.blurbars"
        for (old in listOf("/data/data/app.gamenative", "/data/user/0/app.gamenative")) {
            assertEquals("$root/storage", ForkPaths.originalPrivatePath(root, "$old/storage"))
            assertEquals(root, ForkPaths.originalPrivatePath(root, old))
        }
        for (path in listOf("/storage/emulated/0/PC/FIGUMATE", "/storage/emulated/0/GameNative/CustomGames/BaldrForce",
            "/data/data/app.gamenative.gold/files", "$root/files")) {
            assertEquals(path, ForkPaths.originalPrivatePath(root, path))
        }
    }
}
