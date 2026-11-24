package app.gamenative.utils;

import android.content.Context;

/** Resolve original-package private drives at use time; never rewrite imported configs. */
public final class ForkPaths {
    private ForkPaths() {}

    public static String originalPrivatePath(Context context, String path) {
        return originalPrivatePath(context.getDataDir().getAbsolutePath(), path);
    }

    public static String originalPrivatePath(String dataDir, String path) {
        for (String old : new String[]{"/data/data/app.gamenative", "/data/user/0/app.gamenative"}) {
            if (path.equals(old)) return dataDir;
            if (path.startsWith(old + "/")) return dataDir + path.substring(old.length());
        }
        return path;
    }

    public static String originalPrivateReferences(Context context, String values) {
        String prefix = context.getDataDir().getAbsolutePath() + "/";
        return values.replace("/data/data/app.gamenative/", prefix)
                .replace("/data/user/0/app.gamenative/", prefix);
    }
}
