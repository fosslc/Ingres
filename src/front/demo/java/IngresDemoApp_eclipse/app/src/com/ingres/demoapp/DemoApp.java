package com.ingres.demoapp;

import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.dialogs.ErrorDialog;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.ui.plugin.AbstractUIPlugin;
import org.osgi.framework.BundleContext;

import com.ingres.demoapp.persistence.DatabaseConfigurationDAO;
import com.ingres.demoapp.ui.statusbar.StatusLineControl;

/**
 * The activator class controls the plug-in life cycle
 */
public class DemoApp extends AbstractUIPlugin {

    /**
     * The plug-in ID
     */
    public static final String PLUGIN_ID = "com.ingres.demoapp";

    /**
     * The shared instance
     */
    private static DemoApp plugin;

    /**
     * The constructor
     */
    public DemoApp() {
        plugin = this;
    }

    /*
     * (non-Javadoc)
     * 
     * @see org.eclipse.ui.plugin.AbstractUIPlugin#start(org.osgi.framework.BundleContext)
     */
    public void start(final BundleContext context) throws Exception {
        super.start(context);
    }

    /*
     * (non-Javadoc)
     * 
     * @see org.eclipse.ui.plugin.AbstractUIPlugin#stop(org.osgi.framework.BundleContext)
     */
    public void stop(final BundleContext context) throws Exception {
        plugin = null;
        super.stop(context);
    }

    /**
     * Returns the shared instance
     * 
     * @return the shared instance
     */
    public static DemoApp getDefault() {
        return plugin;
    }

    /**
     * Returns an image descriptor for the image file at the given plug-in
     * relative path
     * 
     * @param path
     *            the path
     * @return the image descriptor
     */
    public static ImageDescriptor getImageDescriptor(final String path) {
        return imageDescriptorFromPlugin(PLUGIN_ID, path);
    }

    /**
     * This method logs the message and <code>Throwable</code> at level ERROR
     * with the plugin's logger. After that a Error Dialog is shown.
     * 
     * @param message
     *            the message
     * @param t
     *            the <code>Throwable</code>
     * @return the <code>IStatus</code>
     */
    public static int error(final String message, final Throwable t) {
        final IStatus status = logError(t);
        return ErrorDialog.openError(getDefault().getWorkbench().getActiveWorkbenchWindow().getShell(), "Error", message, status);
    }

    /**
     * This method logs the message with given severity, code and
     * <code>Throwable</code> with the plugin's logger.
     * 
     * @param severity
     *            the severity
     * @param code
     *            status code
     * @param message
     *            the message
     * @param t
     *            the thowable
     * @return the <code>IStatus</code>
     * 
     * @see org.eclipse.core.runtime.ILog#log(IStatus)
     */
    public static IStatus log(final int severity, final int code, final String message, final Throwable t) {
        final String msg = (message == null) ? "" : message;
        final IStatus status = new Status(severity, "IngresDemoApp", code, msg, t);
        getDefault().getLog().log(status);
        return status;
    }

    /**
     * This method logs the message at level INFO with the plugin's logger.
     * 
     * @param message
     *            the message
     * @return the <code>IStatus</code>
     */
    public static IStatus logInfo(final String message) {
        return log(IStatus.INFO, 0, message, null);
    }

    /**
     * This method logs the message and <code>Throwable</code> at level INFO
     * with the plugin's logger.
     * 
     * @param message
     *            the message
     * @param t
     *            the <code>Throwable</code>
     * @return the <code>IStatus</code>
     */
    public static IStatus logInfo(final String message, final Throwable t) {
        return log(IStatus.INFO, 0, message, t);
    }

    /**
     * This method logs the message at level ERROR with the plugin's logger.
     * 
     * @param message
     *            the message
     * @return the <code>IStatus</code>
     */
    public static IStatus logError(final String message) {
        return log(IStatus.ERROR, 0, message, null);
    }

    /**
     * This method logs the message and <code>Throwable</code> at level ERROR
     * with the plugin's logger.
     * 
     * @param message
     *            the message
     * @param t
     *            the <code>Throwable</code>
     * @return the <code>IStatus</code>
     */
    public static IStatus logError(final String message, final Throwable t) {
        return log(IStatus.ERROR, 0, message, t);
    }

    /**
     * This method logs the <code>Throwable</code> at level ERROR with the
     * plugin's logger.
     * 
     * @param t
     *            the <code>Throwable</code>
     * @return the <code>IStatus</code>
     */
    public static IStatus logError(final Throwable t) {
        return log(IStatus.ERROR, 0, t.getMessage(), t);
    }

    // statusbar

    private StatusLineControl statusLineControl;

    public static void setStatusLineControl(StatusLineControl statusLineText) {
        if (getDefault().statusLineControl!=null) {
            DatabaseConfigurationDAO.INSTANCE.removeChangeListener(getDefault().statusLineControl);
        }
        getDefault().statusLineControl = statusLineText;
        DatabaseConfigurationDAO.INSTANCE.registerChangeListener(getDefault().statusLineControl);
    }

    public static void setStatusErrorText(String text) {
        if (getDefault().statusLineControl != null) {
            getDefault().statusLineControl.setErrorMessage(text);
            getDefault().statusLineControl.getParent().update(true);
        }
    }

}
