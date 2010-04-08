package com.ingres.demoapp;

import org.eclipse.ui.application.IWorkbenchWindowConfigurer;
import org.eclipse.ui.application.WorkbenchAdvisor;
import org.eclipse.ui.application.WorkbenchWindowAdvisor;

import com.ingres.demoapp.ui.perspectives.RoutesPerspective;

/**
 * This class configures the workbench.
 */
public class ApplicationWorkbenchAdvisor extends WorkbenchAdvisor {

    /*
     * (non-Javadoc)
     * 
     * @see org.eclipse.ui.application.WorkbenchAdvisor#createWorkbenchWindowAdvisor(org.eclipse.ui.application.IWorkbenchWindowConfigurer)
     */
    public WorkbenchWindowAdvisor createWorkbenchWindowAdvisor(final IWorkbenchWindowConfigurer configurer) {
        return new ApplicationWorkbenchWindowAdvisor(configurer);
    }

    /**
     * Returns the id of the <code>RoutesPerspective</code> as initial
     * workbench window.
     * 
     * @return always <code>RoutesPerspective.ID</code>
     * 
     * @see org.eclipse.ui.application.WorkbenchAdvisor#getInitialWindowPerspectiveId()
     */
    public String getInitialWindowPerspectiveId() {
         return RoutesPerspective.ID;
    }
}
