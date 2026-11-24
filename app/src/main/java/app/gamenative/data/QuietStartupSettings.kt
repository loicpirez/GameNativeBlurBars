package app.gamenative.data

enum class StartupLibraryMode { LOGIN, ONLINE, OFFLINE }

data class QuietStartupSettings(
    val skipLoginScreen: Boolean = true,
    val disableUpdateChecks: Boolean = true,
    val hideConnectionBanners: Boolean = true,
) {
    fun initialLibraryMode(isSteamLoggedIn: Boolean, hasStoredCredentials: Boolean): StartupLibraryMode = when {
        isSteamLoggedIn -> StartupLibraryMode.ONLINE
        skipLoginScreen || hasStoredCredentials -> StartupLibraryMode.OFFLINE
        else -> StartupLibraryMode.LOGIN
    }

    fun showConnectionWarning(isConnecting: Boolean, isDisconnected: Boolean): Boolean =
        !hideConnectionBanners && (isConnecting || isDisconnected)
}
