package com.ingres.demoapp.actions;

import org.eclipse.jface.action.Action;
import org.eclipse.ui.IPerspectiveDescriptor;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PlatformUI;

import com.ingres.demoapp.DemoApp;
import com.ingres.demoapp.model.RoutesCriteria;
import com.ingres.demoapp.persistence.UserProfileDAO;
import com.ingres.demoapp.ui.editors.RoutesCriteriaEditor;
import com.ingres.demoapp.ui.editors.RoutesCriteriaInput;
import com.ingres.demoapp.ui.perspectives.RoutesPerspective;

public class OpenRoutesPerspectiveAction extends Action {

    private final IWorkbenchWindow window;

    public OpenRoutesPerspectiveAction(IWorkbenchWindow window, String label, String actionDefinitionId) {
        this.window = window;
        setText(label);
        setId(actionDefinitionId);
        setActionDefinitionId(actionDefinitionId);
    }

    public void run() {
        if (window != null) {
            window.getActivePage().closeAllEditors(false);
            IPerspectiveDescriptor perspectiveDescriptor = PlatformUI.getWorkbench().getPerspectiveRegistry().findPerspectiveWithId(
                    RoutesPerspective.ID);
            window.getActivePage().setPerspective(perspectiveDescriptor);

            RoutesCriteria criteria = new RoutesCriteria();
            try {
                criteria.setDepartingAirport(UserProfileDAO.getDefaultUserProfile().getIataCode());
                RoutesCriteriaInput input = new RoutesCriteriaInput(criteria);
                window.getWorkbench().getActiveWorkbenchWindow().getActivePage().openEditor(input, RoutesCriteriaEditor.ID);
            } catch (Exception e) {
                DemoApp.error(e.getMessage(), e);
                window.getActivePage().closeAllEditors(false);
            }

        }
    }

}
