package com.ingres.demoapp.persistence;

import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;

import org.eclipse.core.runtime.Assert;

import com.ingres.demoapp.DemoApp;
import com.ingres.demoapp.model.DatabaseConfiguration;
import com.ingres.demoapp.persistence.configuration.DatabaseConfigurationStore;

public class DatabaseConfigurationDAO {
    
    // //////////////////////////////////////////
    // singleton
    // //////////////////////////////////////////

    public static final DatabaseConfigurationDAO INSTANCE = new DatabaseConfigurationDAO();

    private DatabaseConfigurationDAO() {
        configStore = new DatabaseConfigurationStore(DemoApp.getDefault().getPluginPreferences());
    }

    // //////////////////////////////////////////
    // configuration stuff
    // //////////////////////////////////////////

    public DatabaseConfiguration getDatabaseConfiguration() {
        if (databaseConfiguration == null) {
            databaseConfiguration = getDefaultDatabaseConfiguration();
            notifyListeners();
        }
        return (DatabaseConfiguration) databaseConfiguration.clone();
    }

    public void setDatabaseConfiguration(DatabaseConfiguration config) {
        Assert.isNotNull(config);
        if (!config.equals(databaseConfiguration)) {
            databaseConfiguration = (DatabaseConfiguration) config.clone();
            if(databaseConfiguration.isDefaultConfiguration()) {
                configStore.store(databaseConfiguration);
            }
            notifyListeners();
        }
    }

    public void setDefaultDatabaseConfiguration() {
        setDatabaseConfiguration(getDefaultDatabaseConfiguration());
    }
    
    private DatabaseConfiguration databaseConfiguration;
    
    private DatabaseConfigurationStore configStore;
    
    private DatabaseConfiguration getDefaultDatabaseConfiguration() {
        DatabaseConfiguration config;

        config = configStore.load();
        if (config == null) {
            config = createDefaultDatabaseConfiguration();
            configStore.store(config);
        }

        return config;
    }

    private DatabaseConfiguration createDefaultDatabaseConfiguration() {
        final DatabaseConfiguration config = new DatabaseConfiguration();
        config.setDatabase("demodb");
        config.setDefaultConfiguration(true);
        config.setHost("127.0.0.1");
        config.setPassword("");
        config.setPort("II");
        config.setUser("");
        return config;
    }


    // //////////////////////////////////////////
    // configuration change listener
    // //////////////////////////////////////////

    public interface DatabaseConfigurationChangeListener {
        void update(DatabaseConfiguration newConfig);
    }

    public void registerChangeListener(DatabaseConfigurationChangeListener listener) {
        Assert.isNotNull(listener);
        listeners.add(listener);
    }

    public void removeChangeListener(DatabaseConfigurationChangeListener listener) {
        Assert.isNotNull(listener);
        listeners.remove(listener);
    }

    private Set listeners = new HashSet();

    private void notifyListeners() {
        Iterator it = listeners.iterator();
        while (it.hasNext()) {
            ((DatabaseConfigurationChangeListener) it.next()).update((DatabaseConfiguration) databaseConfiguration.clone());
        }
    }

}
