package com.ingres.demoapp.util;

import com.ingres.demoapp.model.Version;

public class VersionHelper {
    
    public static boolean checkVersion(Version version) {
        if (version.getVerMajor().intValue() == 1 && version.getVerMinor().intValue() == 0 && version.getVerRelease().intValue() == 0) {
            return true;
        }
        return false;
    }

}
