package app.gamenative.ui.util

import android.app.Application
import android.content.Context
import androidx.compose.runtime.saveable.SaverScope
import androidx.test.core.app.ApplicationProvider
import app.gamenative.PrefManager
import app.gamenative.utils.ContainerUtils
import app.gamenative.utils.FakeDataStore
import app.gamenative.utils.installFakePrefManager
import app.gamenative.utils.awaitUntil
import com.winlator.container.Container
import com.winlator.container.ContainerData
import org.json.JSONObject
import org.junit.Assert.*
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config

@RunWith(RobolectricTestRunner::class)
@Config(application = Application::class, sdk = [29])
class BlurBarsDialogTest {
    @get:Rule val temp = TemporaryFolder()

    @Test fun dialogDraftSurvivesRecreationAndPersistsIntoTheInGameSettings() {
        installFakePrefManager(FakeDataStore()).use {
            val container = Container("CUSTOM_GAME_731").apply { setRootDir(temp.newFolder("container")) }
            java.io.File(container.rootDir, ".wine").mkdirs()
            java.io.File(container.rootDir, ".wine/user.reg").writeText("WINE REGISTRY Version 2\n\n")
            container.putExtra(ScreenEffectsConfig.KEY_GAMMA, 1.4f)
            container.putExtra(ScreenEffectsConfig.KEY_ENABLE_FXAA, true)
            container.putExtra("keep", "unchanged")
            val initial = ContainerUtils.toContainerData(container)
            val draft = withBlurBars(initial, ScreenEffectsConfig(enableBlurBars = true, blurBarsRadius = 84f,
                blurBarsWidth = 21f, blurBarsBrightness = 55f))
            assertFalse(loadScreenEffectsConfig(container).enableBlurBars) // Editing/cancelling has no write side effect.
            val saved = with(ContainerData.Saver) { SaverScope { true }.save(draft) }
            val restored = ContainerData.Saver.restore(saved!!)
            assertEquals(draft, restored)
            val context = ApplicationProvider.getApplicationContext<Context>()
            ContainerUtils.applyToContainer(context, container, restored!!, false)
            assertTrue(container.saveDataChecked())
            val loaded = Container("CUSTOM_GAME_731").apply { loadData(JSONObject(fileFor(container).readText())) }
            val effects = loadScreenEffectsConfig(loaded)
            assertTrue(effects.enableBlurBars)
            assertEquals(84f, effects.blurBarsRadius, 0f)
            assertEquals(21f, effects.blurBarsWidth, 0f)
            assertEquals(55f, effects.blurBarsBrightness, 0f)
            assertEquals(1.4f, effects.gamma, 0f)
            assertTrue(effects.enableFXAA)
            assertEquals("unchanged", loaded.getExtra("keep"))
            assertEquals(84f, ContainerUtils.toContainerData(loaded).blurBarsRadius, 0f)
        }
    }

    @Test fun globalDefaultsAndOldSavedDialogsRemainCompatible() {
        installFakePrefManager(FakeDataStore()).use {
            val data = ContainerUtils.getDefaultContainerData()
            assertFalse(data.enableBlurBars)
            val configured = data.copy(enableBlurBars = true, blurBarsRadius = 64f, blurBarsWidth = 19f, blurBarsBrightness = 80f)
            ContainerUtils.setDefaultContainerData(configured)
            awaitUntil { PrefManager.defaultEnableBlurBars && PrefManager.defaultBlurBarsRadius == 64f }
            assertEquals(blurBarsScreenEffects(configured), blurBarsScreenEffects(ContainerUtils.getDefaultContainerData()))
            val saved = with(ContainerData.Saver) { SaverScope { true }.save(data) }
            // mapSaver encodes its map into an alternating key/value list.
            val old = (saved as List<*>).chunked(2).filterNot { (it[0] as String).contains("blurBars", true) }.flatten()
            val restored = ContainerData.Saver.restore(old)!!
            assertFalse(restored.enableBlurBars)
            assertEquals(48f, restored.blurBarsRadius, 0f)
        }
    }

    private fun fileFor(container: Container) = java.io.File(container.rootDir, ".container")
}
