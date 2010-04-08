package com.ingres.demoapp.ui.perspectives;

import org.eclipse.ui.IFolderLayout;
import org.eclipse.ui.IPageLayout;
import org.eclipse.ui.IPerspectiveFactory;

/**
 * This class defines the initial layout for the database configuration window.
 * The editor area (for editing the database configuration) is positioned in the
 * upper part of window. The help view is positioned in the lower part of the
 * window, it is unmovable and unclosable.
 */
public class DatabaseConfigurationPerspective implements IPerspectiveFactory {

    /**
     * The ID of this perspective. Needs to be whatever is mentioned in
     * plugin.xml.
     */
    public static final String ID = "com.ingres.demoapp.ui.perspectives.DatabaseConfigurationPerspective";

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
        layout.setEditorAreaVisible(false);

        final IFolderLayout folder = layout.createFolder("folder.bottom", IPageLayout.BOTTOM, 0.50f, editorArea);

        folder.addView("org.eclipse.help.ui.HelpView");
        layout.getViewLayout("org.eclipse.help.ui.HelpView").setCloseable(false);
        layout.getViewLayout("org.eclipse.help.ui.HelpView").setMoveable(false);
    }
}
