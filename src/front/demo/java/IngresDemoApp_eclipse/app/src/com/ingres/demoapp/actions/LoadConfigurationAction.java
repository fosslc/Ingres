package com.ingres.demoapp.actions;

import java.beans.XMLDecoder;
import java.io.BufferedInputStream;
import java.io.FileInputStream;
import java.io.FileNotFoundException;

import org.eclipse.jface.action.Action;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.ui.IWorkbenchWindow;

import com.ingres.demoapp.DemoApp;
import com.ingres.demoapp.model.DatabaseConfiguration;
import com.ingres.demoapp.persistence.DatabaseConfigurationDAO;

/**
 * This action is responsible for loading a database configuration from xml. To
 * do so a Open Dialog is shown, so the user can pick a configuration file.
 * After loading the configuration, the actual database configuration is
 * replaced with the new one. Whoever the default configuration is not changed.
 */
public class LoadConfigurationAction extends Action {

    private final transient IWorkbenchWindow window;

    /**
     * The constructor
     * 
     * @param window
     *            the workbench window
     * @param label
     *            the label to show
     * @param actionDefinitionId
     *            the action definition id (same as in plugin.xml)
     */
    public LoadConfigurationAction(final IWorkbenchWindow window, final String label, final String actionDefinitionId) {
        this.window = window;
        setText(label);
        setId(actionDefinitionId);
    }

    /**
     * Executes the action.<br>
     * <ol>
     * <li>Shows an open dialog</li>
     * <li>Opens the choosen file</li>
     * <li>Sets the actual database configuration</li>
     * </ol>
     * If an error occures while execution, a error dialog is shown.
     * 
     * @see org.eclipse.jface.action.Action#run()
     * 
     */
    public void run() {
        final FileDialog fileDialog = new FileDialog(window.getShell(), SWT.OPEN);
        fileDialog.setFilterExtensions(new String[] { "*.xml" });
        fileDialog.setFileName("databaseConfiguration.xml");

        final String fileName = fileDialog.open();

        if (fileName != null) {

                XMLDecoder encoder;
                try {
                    encoder = new XMLDecoder(new BufferedInputStream(new FileInputStream(fileName)));
                    final DatabaseConfiguration config = (DatabaseConfiguration) encoder.readObject();
                    encoder.close();
                    config.setDefaultConfiguration(false);
                    DatabaseConfigurationDAO.INSTANCE.setDatabaseConfiguration(config);
                } catch (FileNotFoundException e) {
                    DemoApp.error(e.getMessage(), e);
                }

        }
    }

}
