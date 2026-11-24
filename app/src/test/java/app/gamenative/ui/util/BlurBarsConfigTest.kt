package app.gamenative.ui.util

import com.winlator.container.Container
import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class BlurBarsConfigTest {
    @Test
    fun oldContainersKeepBlackBarsAndOtherSettings() {
        val container = Container("blur-bars-defaults")
        container.putExtra("unrelated-setting", "keep-me")
        val config = loadScreenEffectsConfig(container)
        assertFalse(config.enableBlurBars)
        assertEquals(48f, config.blurBarsRadius, 0f)
        assertEquals(12f, config.blurBarsWidth, 0f)
        assertEquals(75f, config.blurBarsBrightness, 0f)
        assertEquals("keep-me", container.getExtra("unrelated-setting"))
    }

    @Test
    fun everyParameterRoundTripsWithoutChangingExistingEffects() {
        val container = Container("blur-bars-round-trip")
        container.putExtra("unrelated-setting", "keep-me")
        val config = ScreenEffectsConfig(enableBlurBars = true, blurBarsRadius = 80f,
            blurBarsWidth = 25f, blurBarsBrightness = 40f, enableFXAA = true,
            scalingMode = ScreenEffectsConfig.SCALING_MODE_FSR_ASPECT, gamma = 1.4f)
        persistScreenEffectsConfig(container, config)
        assertEquals(config, loadScreenEffectsConfig(container))
        assertEquals("keep-me", container.getExtra("unrelated-setting"))
        persistScreenEffectsConfig(container, config.copy(enableBlurBars = false))
        assertFalse(loadScreenEffectsConfig(container).enableBlurBars)
        assertEquals(80f, loadScreenEffectsConfig(container).blurBarsRadius, 0f)
    }

    @Test
    fun malformedAndOutOfRangeSettingsAreSafe() {
        val container = Container("blur-bars-invalid")
        container.putExtra(ScreenEffectsConfig.KEY_BLUR_BARS_RADIUS, "NaN")
        container.putExtra(ScreenEffectsConfig.KEY_BLUR_BARS_WIDTH, "Infinity")
        container.putExtra(ScreenEffectsConfig.KEY_BLUR_BARS_BRIGHTNESS, "broken")
        assertEquals(48f, loadScreenEffectsConfig(container).blurBarsRadius, 0f)
        assertEquals(12f, loadScreenEffectsConfig(container).blurBarsWidth, 0f)
        assertEquals(75f, loadScreenEffectsConfig(container).blurBarsBrightness, 0f)
        persistScreenEffectsConfig(container, ScreenEffectsConfig(blurBarsRadius = 999f,
            blurBarsWidth = -3f, blurBarsBrightness = 150f))
        val result = loadScreenEffectsConfig(container)
        assertEquals(120f, result.blurBarsRadius, 0f)
        assertEquals(1f, result.blurBarsWidth, 0f)
        assertEquals(100f, result.blurBarsBrightness, 0f)
    }
}
