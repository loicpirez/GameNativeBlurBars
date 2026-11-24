package app.gamenative

import app.gamenative.data.QuietStartupSettings
import app.gamenative.data.StartupLibraryMode
import app.gamenative.utils.FakeDataStore
import app.gamenative.utils.awaitUntil
import app.gamenative.utils.installFakePrefManager
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class QuietStartupSettingsTest {
    @Test
    fun startupSkipsOnlyTheUnnecessaryLoginPrompt() {
        val normal = QuietStartupSettings(skipLoginScreen = false)
        val skip = normal.copy(skipLoginScreen = true)
        assertEquals(StartupLibraryMode.LOGIN, normal.initialLibraryMode(false, false))
        assertEquals(StartupLibraryMode.OFFLINE, skip.initialLibraryMode(false, false))
        assertEquals(StartupLibraryMode.OFFLINE, normal.initialLibraryMode(false, true))
        assertEquals(StartupLibraryMode.OFFLINE, skip.initialLibraryMode(false, true))
        assertEquals(StartupLibraryMode.ONLINE, normal.initialLibraryMode(true, false))
        assertEquals(StartupLibraryMode.ONLINE, skip.initialLibraryMode(true, false))
        assertEquals(StartupLibraryMode.ONLINE, normal.initialLibraryMode(true, true))
        assertEquals(StartupLibraryMode.ONLINE, skip.initialLibraryMode(true, true))
    }

    @Test
    fun bannersAreSuppressedForConnectingAndDisconnectedStates() {
        val visible = QuietStartupSettings(hideConnectionBanners = false)
        val hidden = visible.copy(hideConnectionBanners = true)
        assertTrue(visible.showConnectionWarning(true, false))
        assertTrue(visible.showConnectionWarning(false, true))
        assertFalse(hidden.showConnectionWarning(true, false))
        assertFalse(hidden.showConnectionWarning(false, true))
        assertFalse(visible.showConnectionWarning(false, false))
        assertFalse(hidden.showConnectionWarning(false, false))
    }

    @Test
    fun threeOptionsPersistReloadAndRemainIndependentOfExistingPreferences() {
        installFakePrefManager(FakeDataStore()).use {
            PrefManager.reloadQuietStartupSettings()
            assertEquals(QuietStartupSettings(), PrefManager.quietStartupSettings.value)
            assertTrue(PrefManager.skipLoginScreen)
            assertTrue(PrefManager.disableUpdateChecks)
            assertTrue(PrefManager.hideConnectionBanners)
            assertEquals(StartupLibraryMode.OFFLINE, PrefManager.quietStartupSettings.value.initialLibraryMode(false, false))
            assertFalse(PrefManager.quietStartupSettings.value.showConnectionWarning(false, true))
            PrefManager.showGamepadHints = false
            PrefManager.skipLoginScreen = true
            PrefManager.disableUpdateChecks = true
            PrefManager.hideConnectionBanners = true
            awaitUntil {
                PrefManager.getBoolean("blur_bars_skip_login_screen", false) &&
                    PrefManager.getBoolean("blur_bars_disable_update_checks", false) &&
                    PrefManager.getBoolean("blur_bars_hide_connection_banners", false) &&
                    !PrefManager.showGamepadHints
            }
            PrefManager.reloadQuietStartupSettings()
            assertEquals(QuietStartupSettings(true, true, true), PrefManager.quietStartupSettings.value)
            assertFalse(PrefManager.showGamepadHints)
            PrefManager.disableUpdateChecks = false
            assertTrue(PrefManager.skipLoginScreen)
            assertTrue(PrefManager.hideConnectionBanners)
            assertFalse(PrefManager.disableUpdateChecks)
            PrefManager.skipLoginScreen = false
            PrefManager.hideConnectionBanners = false
            awaitUntil {
                !PrefManager.getBoolean("blur_bars_skip_login_screen", true) &&
                    !PrefManager.getBoolean("blur_bars_disable_update_checks", true) &&
                    !PrefManager.getBoolean("blur_bars_hide_connection_banners", true)
            }
            PrefManager.reloadQuietStartupSettings()
            assertEquals(QuietStartupSettings(false, false, false), PrefManager.quietStartupSettings.value)
        }
    }
}
