package com.ingres.demoapp;

import org.eclipse.jface.action.IStatusLineManager;
import org.eclipse.jface.action.StatusLineManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IPerspectiveDescriptor;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.application.ActionBarAdvisor;
import org.eclipse.ui.application.IActionBarConfigurer;
import org.eclipse.ui.application.IWorkbenchWindowConfigurer;
import org.eclipse.ui.application.WorkbenchWindowAdvisor;

import com.ingres.demoapp.model.DatabaseConfiguration;
import com.ingres.demoapp.model.RoutesCriteria;
import com.ingres.demoapp.persistence.DatabaseConfigurationDAO;
import com.ingres.demoapp.persistence.UserProfileDAO;
import com.ingres.demoapp.persistence.VersionDAO;
import com.ingres.demoapp.ui.editors.DatabaseConfigurationEditor;
import com.ingres.demoapp.ui.editors.DatabaseConfigurationEditorInput;
import com.ingres.demoapp.ui.editors.RoutesCriteriaEditor;
import com.ingres.demoapp.ui.editors.RoutesCriteriaInput;
import com.ingres.demoapp.ui.perspectives.DatabaseConfigurationPerspective;
import com.ingres.demoapp.ui.perspectives.RoutesPerspective;
import com.ingres.demoapp.ui.statusbar.StatusLineControl;

public class ApplicationWorkbenchWindowAdvisor extends WorkbenchWindowAdvisor {

    public void createWindowContents(Shell shell) {
        IWorkbenchWindowConfigurer configurer = getWindowConfigurer();

        Menu menuBar = configurer.createMenuBar();
        shell.setMenuBar(menuBar);

        GridLayout shellLayout = new GridLayout();
        shellLayout.marginWidth = 0;
        shellLayout.marginHeight = 0;
        shellLayout.verticalSpacing = 2;
        shellLayout.horizontalSpacing = 2;
        shellLayout.numColumns = 2;
        shell.setLayout(shellLayout);

        GridData gridData;

        // banner
        // create image control due to redrwawing problems with linux/gtk
        // background image disappears after error dialog has been shown
        Image background = DemoApp.getImageDescriptor("/images/IngresBanner.bmp").createImage();
        Control banner = new Composite(shell, SWT.NONE);
        banner.setBackgroundImage(background);
        gridData = new GridData(GridData.FILL, GridData.CENTER, true, false);
        gridData.horizontalSpan = 2;
        gridData.heightHint = 70;
        banner.setLayoutData(gridData);

        // coolbar
        Control coolBar = configurer.createCoolBarControl(shell);
        gridData = new GridData(GridData.FILL, GridData.BEGINNING, false, false);
        gridData.horizontalIndent = -8;
        coolBar.setLayoutData(gridData);

        // workspace
        Control pageComposite = configurer.createPageComposite(shell);
        gridData = new GridData(GridData.FILL, GridData.FILL, true, true);
        pageComposite.setLayoutData(gridData);

        // statusline
        Control statusLine = configurer.createStatusLineControl(shell);
        gridData = new GridData(GridData.FILL, GridData.FILL, true, true);
        gridData.horizontalSpan = 2;
        statusLine.setLayoutData(gridData);
        shell.layout(true);
    }

    public ApplicationWorkbenchWindowAdvisor(IWorkbenchWindowConfigurer configurer) {
        super(configurer);
    }

    public ActionBarAdvisor createActionBarAdvisor(IActionBarConfigurer configurer) {
        return new ApplicationActionBarAdvisor(configurer);
    }

    public void postWindowCreate() {
        // setup status line
        IWorkbenchWindowConfigurer configurer = getWindowConfigurer();
        IStatusLineManager lineManager = configurer.getActionBarConfigurer().getStatusLineManager();

        StatusLineControl statusLineText = new StatusLineControl();
        statusLineText.setStatus(DatabaseConfigurationDAO.INSTANCE.getDatabaseConfiguration());
        DemoApp.setStatusLineControl(statusLineText);
        lineManager.appendToGroup(StatusLineManager.BEGIN_GROUP, statusLineText);
        lineManager.update(true);

        // check connection
        try {
            VersionDAO.getVersion();

            IWorkbenchWindow window = configurer.getWindow();
            window.getActivePage().closeAllEditors(false);
            IPerspectiveDescriptor perspectiveDescriptor = PlatformUI.getWorkbench().getPerspectiveRegistry().findPerspectiveWithId(
                    RoutesPerspective.ID);
            window.getActivePage().setPerspective(perspectiveDescriptor);

            RoutesCriteria criteria = new RoutesCriteria();
            criteria.setDepartingAirport(UserProfileDAO.getDefaultUserProfile().getIataCode());
            RoutesCriteriaInput input = new RoutesCriteriaInput(criteria);
            window.getWorkbench().getActiveWorkbenchWindow().getActivePage().openEditor(input, RoutesCriteriaEditor.ID);
        } catch (Exception e) {
            DemoApp.logError(e);
            
            IWorkbenchWindow window = configurer.getWindow();
            window.getActivePage().closeAllEditors(false);
            final IPerspectiveDescriptor perspectiveDescriptor = PlatformUI.getWorkbench().getPerspectiveRegistry().findPerspectiveWithId(
                    DatabaseConfigurationPerspective.ID);
            window.getActivePage().setPerspective(perspectiveDescriptor);

            final DatabaseConfiguration configuration = DatabaseConfigurationDAO.INSTANCE.getDatabaseConfiguration();

            final DatabaseConfigurationEditorInput input = new DatabaseConfigurationEditorInput(configuration);
            try {
                window.getWorkbench().getActiveWorkbenchWindow().getActivePage().openEditor(input, DatabaseConfigurationEditor.ID);
            } catch (PartInitException ex) {
                DemoApp.logError(ex);
            }
        }
    }

    public void preWindowOpen() {
        IWorkbenchWindowConfigurer configurer = getWindowConfigurer();
        configurer.setInitialSize(new Point(880, 700));
        configurer.setTitle("Ingres Frequent Flyer");
        configurer.setShellStyle(SWT.DIALOG_TRIM);
    }
}
