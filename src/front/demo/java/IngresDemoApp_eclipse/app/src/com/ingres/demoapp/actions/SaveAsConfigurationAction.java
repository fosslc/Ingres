package com.ingres.demoapp.actions;

import java.beans.XMLEncoder;
import java.io.BufferedOutputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.ui.IWorkbenchWindow;

import com.ingres.demoapp.persistence.DatabaseConfigurationDAO;

public class SaveAsConfigurationAction extends Action {

    IWorkbenchWindow window;

    public SaveAsConfigurationAction(IWorkbenchWindow window, String label, String actionDefinitionId) {
        this.window = window;
        setText(label);
        setId(actionDefinitionId);
    }

    public void run() {

        FileDialog fileDialog = new FileDialog(window.getShell(), SWT.SAVE);
        fileDialog.setFilterExtensions(new String[] { "*.xml" });
        fileDialog.setFileName("databaseConfiguration.xml");

        String fileName = fileDialog.open();

        if (fileName != null) {
            try {
                XMLEncoder e = new XMLEncoder(new BufferedOutputStream(new FileOutputStream(fileName)));
                e.writeObject(DatabaseConfigurationDAO.INSTANCE.getDatabaseConfiguration());
                e.close();
            } catch (FileNotFoundException e) {
                MessageDialog.openError(window.getShell(), "Error", "Error while saving database configuration: " + e.getMessage());
            }

        }
    }

}
