package app.gamenative.ui.screen.settings

import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.res.stringResource
import app.gamenative.PrefManager
import app.gamenative.R
import app.gamenative.ui.component.settings.SettingsSwitch
import app.gamenative.ui.theme.settingsTileColorsAlt

@Composable
fun QuietStartupOptions() {
    val settings by PrefManager.quietStartupSettings
    SettingsSwitch(
        colors = settingsTileColorsAlt(),
        title = { Text(stringResource(R.string.quiet_skip_login)) },
        subtitle = { Text(stringResource(R.string.quiet_skip_login_description)) },
        state = settings.skipLoginScreen,
        onCheckedChange = { PrefManager.skipLoginScreen = it },
    )
    SettingsSwitch(
        colors = settingsTileColorsAlt(),
        title = { Text(stringResource(R.string.quiet_disable_updates)) },
        subtitle = { Text(stringResource(R.string.quiet_disable_updates_description)) },
        state = settings.disableUpdateChecks,
        onCheckedChange = { PrefManager.disableUpdateChecks = it },
    )
    SettingsSwitch(
        colors = settingsTileColorsAlt(),
        title = { Text(stringResource(R.string.quiet_hide_banners)) },
        subtitle = { Text(stringResource(R.string.quiet_hide_banners_description)) },
        state = settings.hideConnectionBanners,
        onCheckedChange = { PrefManager.hideConnectionBanners = it },
    )
}
