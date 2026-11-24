package app.gamenative.data

enum class StartupLibraryMode { LOGIN, ONLINE, OFFLINE }

data class QuietStartupSettings(
    val skipLoginScreen: Boolean = false,
    val disableUpdateChecks: Boolean = false,
    val hideConnectionBanners: Boolean = false,
) {
    fun initialLibraryMode(isSteamLoggedIn: Boolean, hasStoredCredentials: Boolean): StartupLibraryMode = when {
        isSteamLoggedIn -> StartupLibraryMode.ONLINE
        skipLoginScreen || hasStoredCredentials -> StartupLibraryMode.OFFLINE
        else -> StartupLibraryMode.LOGIN
    }

    fun showConnectionWarning(isConnecting: Boolean, isDisconnected: Boolean): Boolean =
        !hideConnectionBanners && (isConnecting || isDisconnected)
}
