package com.ingres.demoapp.actions;

import org.eclipse.jface.action.Action;
import org.eclipse.ui.IPerspectiveDescriptor;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;

import com.ingres.demoapp.DemoApp;
import com.ingres.demoapp.model.DatabaseConfiguration;
import com.ingres.demoapp.persistence.DatabaseConfigurationDAO;
import com.ingres.demoapp.ui.editors.DatabaseConfigurationEditor;
import com.ingres.demoapp.ui.editors.DatabaseConfigurationEditorInput;
import com.ingres.demoapp.ui.perspectives.DatabaseConfigurationPerspective;

/**
 * This action is responsible for opening the Database Configuration Perspective
 * with the appropriate editor.
 */
public class OpenConfigurationPerspectiveAction extends Action {

    private final transient IWorkbenchWindow window;

    /**
     * The constructor.
     * 
     * @param window
     *            the window
     * @param label
     *            the label
     * @param actionDefinitionId
     *            the action definition id
     */
    public OpenConfigurationPerspectiveAction(final IWorkbenchWindow window, final String label, final String actionDefinitionId) {
        this.window = window;
        setText(label);
        setId(actionDefinitionId);
        setActionDefinitionId(actionDefinitionId);
    }

    /**
     * This method executes the action. The following steps are done:<br>
     * <ol>
     * <li>Close all open editors</li>
     * <li>Open the database configuration perspective</li>
     * <li>Open an editor with the actual database configuration as input</li>
     * </ol>
     * 
     * @see org.eclipse.jface.action.Action#run()
     */
    public void run() {
        if (window != null) {
            window.getActivePage().closeAllEditors(false);
            final IPerspectiveDescriptor perspectiveDescriptor = PlatformUI.getWorkbench().getPerspectiveRegistry().findPerspectiveWithId(
                    DatabaseConfigurationPerspective.ID);
            window.getActivePage().setPerspective(perspectiveDescriptor);

            final DatabaseConfiguration configuration = DatabaseConfigurationDAO.INSTANCE.getDatabaseConfiguration();

            final DatabaseConfigurationEditorInput input = new DatabaseConfigurationEditorInput(configuration);
            try {
                window.getWorkbench().getActiveWorkbenchWindow().getActivePage().openEditor(input, DatabaseConfigurationEditor.ID);
            } catch (PartInitException e) {
                DemoApp.logError(e);
            }

        }
    }

}
