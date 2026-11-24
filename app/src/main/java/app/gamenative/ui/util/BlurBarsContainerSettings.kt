package app.gamenative.ui.util

import com.winlator.container.Container
import com.winlator.container.ContainerData

/** The dialog edits a draft; the same extras are read by the in-game menu. */
internal fun blurBarsScreenEffects(data: ContainerData): ScreenEffectsConfig = ScreenEffectsConfig(
    enableBlurBars = data.enableBlurBars,
    blurBarsRadius = blurBarsValue(data.blurBarsRadius.toString(), 48f, 0f, 120f),
    blurBarsWidth = blurBarsValue(data.blurBarsWidth.toString(), 12f, 1f, 50f),
    blurBarsBrightness = blurBarsValue(data.blurBarsBrightness.toString(), 75f, 0f, 100f),
)

internal fun withBlurBars(data: ContainerData, effects: ScreenEffectsConfig): ContainerData = data.copy(
    enableBlurBars = effects.enableBlurBars,
    blurBarsRadius = effects.blurBarsRadius,
    blurBarsWidth = effects.blurBarsWidth,
    blurBarsBrightness = effects.blurBarsBrightness,
)

internal fun persistBlurBarsContainerSettings(container: Container, data: ContainerData) {
    val blur = blurBarsScreenEffects(data)
    container.putExtra(ScreenEffectsConfig.KEY_ENABLE_BLUR_BARS, blur.enableBlurBars)
    container.putExtra(ScreenEffectsConfig.KEY_BLUR_BARS_RADIUS, blur.blurBarsRadius)
    container.putExtra(ScreenEffectsConfig.KEY_BLUR_BARS_WIDTH, blur.blurBarsWidth)
    container.putExtra(ScreenEffectsConfig.KEY_BLUR_BARS_BRIGHTNESS, blur.blurBarsBrightness)
}
