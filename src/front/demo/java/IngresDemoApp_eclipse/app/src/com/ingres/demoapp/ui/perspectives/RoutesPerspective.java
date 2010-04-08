package com.ingres.demoapp.ui.perspectives;

import org.eclipse.ui.IFolderLayout;
import org.eclipse.ui.IPageLayout;
import org.eclipse.ui.IPerspectiveFactory;
import org.eclipse.ui.IViewLayout;

import com.ingres.demoapp.ui.views.RoutesView;

/**
 * This class defines the initial layout for the routes window. The editor area
 * (for defining queries) is positioned in the upper part of the window. The
 * result and the help view are positioned in the lower part of the window, both
 * are unmovable and unclosable.
 */
public class RoutesPerspective implements IPerspectiveFactory {

    /**
     * The ID of this perspective. Needs to be whatever is mentioned in
     * plugin.xml.
     */
    public static final String ID = "com.ingres.demoapp.ui.perspectives.RoutesPerspective";

    /**
     * This method is responsible for creating the initial layout of the page.
     * The HelpView (<code>org.eclipse.help.ui.HelpView</code> defined in the
     * plugin <code>org.eclipse.help.ui</code>) is positioned in the lower
     * part of the window. To make this work the StickView definition of the
     * HelpView has to be removed in the defining plugin's plugin.xml.
     * 
     * @param layout
     *            the page layout
     * 
     * @see org.eclipse.ui.IPerspectiveFactory#createInitialLayout(org.eclipse.ui.IPageLayout)
     */
    public void createInitialLayout(final IPageLayout layout) {
        final String editorArea = layout.getEditorArea();
        layout.setEditorAreaVisible(true);

        // define a bottom folder
        final IFolderLayout bottomFolder = layout.createFolder("folder.bottom", IPageLayout.BOTTOM, 0.50f, editorArea);

        // add route and help view
        bottomFolder.addView(RoutesView.ID);
        bottomFolder.addView("org.eclipse.help.ui.HelpView");

        // set route and help view to unclosable and unmovable
        IViewLayout viewLayout = layout.getViewLayout(RoutesView.ID);
        viewLayout.setCloseable(false);
        viewLayout.setMoveable(false);

        viewLayout = layout.getViewLayout("org.eclipse.help.ui.HelpView");
        viewLayout.setCloseable(false);
        viewLayout.setMoveable(false);
    }
}
