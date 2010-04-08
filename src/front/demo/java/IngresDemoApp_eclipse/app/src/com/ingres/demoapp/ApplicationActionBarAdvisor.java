package com.ingres.demoapp;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.ICoolBarManager;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.action.ToolBarContributionItem;
import org.eclipse.jface.action.ToolBarManager;
import org.eclipse.swt.SWT;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.actions.ActionFactory;
import org.eclipse.ui.actions.ActionFactory.IWorkbenchAction;
import org.eclipse.ui.application.ActionBarAdvisor;
import org.eclipse.ui.application.IActionBarConfigurer;

import com.ingres.demoapp.actions.LoadConfigurationAction;
import com.ingres.demoapp.actions.OpenConfigurationPerspectiveAction;
import com.ingres.demoapp.actions.OpenProfilePerspectiveAction;
import com.ingres.demoapp.actions.OpenRoutesPerspectiveAction;
import com.ingres.demoapp.actions.SaveAsConfigurationAction;
import com.ingres.demoapp.actions.SaveConfigurationAction;

/**
 * Class for configuration of action bars of the workbench window. Basically the
 * following menu structure is created:<br>
 * <ul>
 * <li>File
 * <ul>
 * <li>Open</li>
 * <li>Save</li>
 * <li>Save as</li>
 * <li>Exit</li>
 * </ul>
 * </li>
 * <li>Tools
 * <ul>
 * <li>Route Search</li>
 * <li>My Profile</li>
 * <li>Options
 * <ul>
 * <li>Connect</li>
 * </ul>
 * </li>
 * </ul>
 * </li>
 * <li>Help
 * <ul>
 * <li>Help Contents</li>
 * <li>About</li>
 * </ul>
 * </li>
 * </ul>
 * The coolbar is filled with buttons for:<br>
 * <ul>
 * <li>opening the routes view</li>
 * <li>opening the profile view</li>
 * <li>opening the connection configuration</li>
 * <li>exiting the application</li>
 * </ul>
 */
public class ApplicationActionBarAdvisor extends ActionBarAdvisor {

    // TODO actions are duplicated -> how to disable icons in menubar?

    // ////////////
    // Menu actions
    // ////////////

    // File menu
    private Action openMenuAction;

    private Action saveMenuAction;

    private Action saveAsMenuAction;

    private IWorkbenchAction exitMenuAction;

    // Tools menu
    private Action routesMenuAction;

    private Action profileMenuAction;

    private Action configurationMenuAction;

    // Help menu
    private IWorkbenchAction aboutMenuAction;

    private IWorkbenchAction helpMenuAction;

    // ///////////////
    // Toolbar actions
    // ///////////////

    private Action routesToolbarAction;

    private Action profileToolbarAction;

    private Action configurationToolbarAction;

    private IWorkbenchAction exitToolbarAction;

    /**
     * Creates a new action bar advisor to configure a workbench window's action
     * bars via the given action bar configurer.
     * 
     * @param configurer
     *            the action bar configurer
     */
    public ApplicationActionBarAdvisor(IActionBarConfigurer configurer) {
        super(configurer);
    }

    /**
     * Instantiates the actions used in the fill methods.
     * 
     * @param window the window containing the action bars
     * 
     * @see org.eclipse.ui.application.ActionBarAdvisor#makeActions(org.eclipse.ui.IWorkbenchWindow)
     */
    protected void makeActions(final IWorkbenchWindow window) {
        // /////////////
        // Menu actions
        // /////////////

        // File menu
        openMenuAction = new LoadConfigurationAction(window, "Open", "menu.configuration.load");
        register(openMenuAction);

        saveMenuAction = new SaveConfigurationAction("Save", "menu.configuration.save");
        register(saveMenuAction);

        saveAsMenuAction = new SaveAsConfigurationAction(window, "Save as", "menu.configuration.saveas");
        register(saveAsMenuAction);

        exitMenuAction = ActionFactory.QUIT.create(window);
        register(exitMenuAction);

        // Tools menu
        routesMenuAction = new OpenRoutesPerspectiveAction(window, "Route search", "menu.perspective.open.routes");
        register(routesMenuAction);

        profileMenuAction = new OpenProfilePerspectiveAction(window, "My profile", "menu.perspective.open.profile");
        register(profileMenuAction);

        configurationMenuAction = new OpenConfigurationPerspectiveAction(window, "Connect", "menu.perspective.open.configuration");
        register(configurationMenuAction);

        // Help menu
        aboutMenuAction = ActionFactory.ABOUT.create(window);
        register(aboutMenuAction);

        helpMenuAction = ActionFactory.HELP_CONTENTS.create(window);
        register(helpMenuAction);

        // ////////////////
        // Toolbar actions
        // ////////////////

        routesToolbarAction = new OpenRoutesPerspectiveAction(window, "", "toolbar.perspective.open.routes");
        routesToolbarAction.setImageDescriptor(DemoApp.getImageDescriptor("/images/RoutesButton.bmp"));
        register(routesToolbarAction);

        profileToolbarAction = new OpenProfilePerspectiveAction(window, "", "toolbar.perspective.open.profile");
        profileToolbarAction.setImageDescriptor(DemoApp.getImageDescriptor("/images/ProfileButton.bmp"));
        register(profileToolbarAction);

        configurationToolbarAction = new OpenConfigurationPerspectiveAction(window, "", "toolbar.perspective.open.configuration");
        configurationToolbarAction.setImageDescriptor(DemoApp.getImageDescriptor("/images/ConnectButton.bmp"));
        register(configurationToolbarAction);

        exitToolbarAction = ActionFactory.QUIT.create(window);
        exitToolbarAction.setImageDescriptor(DemoApp.getImageDescriptor("/images/ExitButton.bmp"));
        register(exitToolbarAction);
    }

    /**
     * Fills the menubar with mentioned entries.
     * 
     * @param menuBar
     *            the menu manager for the menu bar
     * 
     * @see org.eclipse.ui.application.ActionBarAdvisor#fillMenuBar(org.eclipse.jface.action.IMenuManager)
     */
    protected void fillMenuBar(final IMenuManager menuBar) {
        final MenuManager fileMenu = new MenuManager("&File", IWorkbenchActionConstants.M_FILE);
        final MenuManager toolsMenu = new MenuManager("&Tools", "tools");
        final MenuManager optionsMenu = new MenuManager("&Options", "options");
        final MenuManager helpMenu = new MenuManager("&Help", IWorkbenchActionConstants.M_HELP);

        menuBar.add(fileMenu);
        menuBar.add(toolsMenu);
        menuBar.add(helpMenu);

        fileMenu.add(openMenuAction);
        fileMenu.add(saveMenuAction);
        fileMenu.add(saveAsMenuAction);
        fileMenu.add(new Separator());
        fileMenu.add(exitMenuAction);

        toolsMenu.add(routesMenuAction);
        toolsMenu.add(profileMenuAction);
        toolsMenu.add(new Separator());
        toolsMenu.add(optionsMenu);

        optionsMenu.add(configurationMenuAction);

        helpMenu.add(helpMenuAction);
        helpMenu.add(new Separator());
        helpMenu.add(aboutMenuAction);

    }

    /**
     * Fills coolbar with Route, Profile, Connection and Exit button.
     * 
     * @param coolBar
     *            the cool bar manager
     * 
     * @see org.eclipse.ui.application.ActionBarAdvisor#fillCoolBar(org.eclipse.jface.action.ICoolBarManager)
     */
    protected void fillCoolBar(final ICoolBarManager coolBar) {
        final IToolBarManager toolbar = new ToolBarManager(SWT.VERTICAL | SWT.RIGHT);
        coolBar.add(new ToolBarContributionItem(toolbar, "main"));

        toolbar.add(routesToolbarAction);
        toolbar.add(profileToolbarAction);
        toolbar.add(configurationToolbarAction);
        toolbar.add(exitToolbarAction);
    }

}
