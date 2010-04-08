package com.ingres.demoapp.actions;

import org.eclipse.jface.action.Action;
import org.eclipse.ui.IPerspectiveDescriptor;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PlatformUI;

import com.ingres.demoapp.DemoApp;
import com.ingres.demoapp.model.UserProfile;
import com.ingres.demoapp.persistence.UserProfileDAO;
import com.ingres.demoapp.ui.editors.UserProfileInput;
import com.ingres.demoapp.ui.editors.UserProfileModifyEditor;
import com.ingres.demoapp.ui.perspectives.ProfilePerspective;

/**
 * This action is responsible for opening the Profile Perspective with the
 * appropriate editor.
 */
public class OpenProfilePerspectiveAction extends Action {

    private final transient IWorkbenchWindow window;

    /**
     * The constructor
     * 
     * @param window
     *            the window
     * @param label
     *            the label
     * @param actionDefinitionId
     *            the action definition id
     */
    public OpenProfilePerspectiveAction(IWorkbenchWindow window, String label, String actionDefinitionId) {
        this.window = window;
        setText(label);
        setId(actionDefinitionId);
        setActionDefinitionId(actionDefinitionId);
    }

    public void run() {
        if (window != null) {
            window.getActivePage().closeAllEditors(false);
            final IPerspectiveDescriptor perspectiveDescriptor = PlatformUI.getWorkbench().getPerspectiveRegistry().findPerspectiveWithId(
                    ProfilePerspective.ID);
            window.getActivePage().setPerspective(perspectiveDescriptor);

            try {
                UserProfile profile = UserProfileDAO.getDefaultUserProfile();
                UserProfileInput input = new UserProfileInput(profile);
                window.getWorkbench().getActiveWorkbenchWindow().getActivePage().openEditor(input, UserProfileModifyEditor.ID);
            } catch (Exception e) {
                DemoApp.error(e.getMessage(), e);
                window.getActivePage().closeAllEditors(false);
            }

        }
    }

}
