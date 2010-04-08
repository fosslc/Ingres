package com.ingres.demoapp.ui.statusbar;

import org.eclipse.jface.action.ContributionItem;
import org.eclipse.jface.preference.JFacePreferences;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;

import com.ingres.demoapp.model.DatabaseConfiguration;
import com.ingres.demoapp.persistence.DatabaseConfigurationDAO.DatabaseConfigurationChangeListener;

public class StatusLineControl extends ContributionItem implements DatabaseConfigurationChangeListener {

    String errorMessage;

    public void fill(Composite parent) {
        if (errorMessage != null) {
            CLabel label = new CLabel(parent, SWT.NONE);
            label.setForeground(JFaceResources.getColorRegistry().get(JFacePreferences.ERROR_COLOR));
            label.setFont(new Font(Display.getDefault(), "Tahoma", 8, SWT.BOLD));
            label.setText(errorMessage);
        } else {
            ConnectionStatus status = new ConnectionStatus(parent, SWT.NONE);
            if (databaseConfiguration != null) {
                status.setHostText(databaseConfiguration.getHost());
                status.setInstanceText("Ingres" + databaseConfiguration.getPort());
                status.setDatabaseText(databaseConfiguration.getDatabase());
                status.setUserText(databaseConfiguration.getUser());
            } else {
                status.setHostText(null);
                status.setInstanceText(null);
                status.setDatabaseText(null);
                status.setUserText(null);
            }
        }
    }

    /**
     * @return the message
     */
    public String getErrorMessage() {
        return errorMessage;
    }

    /**
     * @param message
     *            the message to set
     */
    public void setErrorMessage(String message) {
        this.errorMessage = message;
    }

    private DatabaseConfiguration databaseConfiguration;

    public void setStatus(DatabaseConfiguration databaseConfiguration) {
        this.databaseConfiguration = databaseConfiguration;
        errorMessage = null;
    }

    public void update(DatabaseConfiguration newConfig) {
        setStatus(newConfig);
        getParent().update(true);
    }

}
