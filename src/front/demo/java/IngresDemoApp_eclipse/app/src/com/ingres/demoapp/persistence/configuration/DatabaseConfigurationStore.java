package com.ingres.demoapp.persistence.configuration;

import org.eclipse.core.runtime.Assert;
import org.eclipse.core.runtime.Preferences;

import com.ingres.demoapp.model.DatabaseConfiguration;

/**
 * This class is responsible for retrievig and storing database configurations
 * in the preferences store.
 * 
 */
public class DatabaseConfigurationStore {

    private static final String PREF_CONFIGURATION_AVAILABE = "com.ingres.demoapp.database.ConnectionConfiguration";

    private static final String PREF_HOST_NAME = "com.ingres.demoapp.database.ConnectionConfiguration.hostName";

    private static final String PREF_DATABASE = "com.ingres.demoapp.database.ConnectionConfiguration.database";

    private static final String PREF_PORT_NAME = "com.ingres.demoapp.database.ConnectionConfiguration.instanceName";

    private static final String PREF_USER = "com.ingres.demoapp.database.ConnectionConfiguration.user";

    private static final String PREF_PASSWORD = "com.ingres.demoapp.database.ConnectionConfiguration.password";

    private Preferences preferences;

    public DatabaseConfigurationStore(Preferences preferences) {
        Assert.isNotNull(preferences);

        this.preferences = preferences;
    }

    public void store(DatabaseConfiguration config) {
        Assert.isNotNull(config);

        synchronized (this) {
            preferences.setValue(PREF_HOST_NAME, config.getHost());
            preferences.setValue(PREF_DATABASE, config.getDatabase());
            preferences.setValue(PREF_PORT_NAME, config.getPort());
            preferences.setValue(PREF_USER, config.getUser());
            preferences.setValue(PREF_PASSWORD, config.getPassword());
            preferences.setValue(PREF_CONFIGURATION_AVAILABE, true);
        }
    }

    public DatabaseConfiguration load() {
        DatabaseConfiguration config = null;
        synchronized (this) {
            if (preferences.getBoolean(PREF_CONFIGURATION_AVAILABE)) {
                config = new DatabaseConfiguration();
                config.setDatabase(preferences.getString(PREF_DATABASE));
                config.setDefaultConfiguration(true);
                config.setHost(preferences.getString(PREF_HOST_NAME));
                config.setPassword(preferences.getString(PREF_PASSWORD));
                config.setPort(preferences.getString(PREF_PORT_NAME));
                config.setUser(preferences.getString(PREF_USER));
            }
        }
        return config;
    }

}
