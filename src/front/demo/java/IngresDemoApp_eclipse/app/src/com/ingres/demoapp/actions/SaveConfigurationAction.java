package com.ingres.demoapp.actions;

import org.eclipse.jface.action.Action;

import com.ingres.demoapp.model.DatabaseConfiguration;
import com.ingres.demoapp.persistence.DatabaseConfigurationDAO;

public class SaveConfigurationAction extends Action {

    public SaveConfigurationAction(String label, String actionDefinitionId) {
        setText(label);
        setId(actionDefinitionId);
    }

    public void run() {
        DatabaseConfiguration configuration = DatabaseConfigurationDAO.INSTANCE.getDatabaseConfiguration();
        configuration.setDefaultConfiguration(true);
        DatabaseConfigurationDAO.INSTANCE.setDatabaseConfiguration(configuration);
    }

}
