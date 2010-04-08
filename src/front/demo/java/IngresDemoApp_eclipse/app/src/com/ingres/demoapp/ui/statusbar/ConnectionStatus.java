package com.ingres.demoapp.ui.statusbar;

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;

public class ConnectionStatus extends Composite {

    private CLabel cLabel = null;
    private CLabel hostCLabel = null;
    private CLabel cLabel1 = null;
    private CLabel instanceCLabel = null;
    private CLabel cLabel2 = null;
    private CLabel databaseCLabel = null;
    private CLabel cLabel3 = null;
    private CLabel userCLabel = null;

    public ConnectionStatus(Composite parent, int style) {
        super(parent, style);
        initialize();
    }

    private void initialize() {
        cLabel = new CLabel(this, SWT.NONE);
        cLabel.setText("Host:");
        hostCLabel = new CLabel(this, SWT.NONE);
        hostCLabel.setText("CLabel");
        hostCLabel.setFont(new Font(Display.getDefault(), "Tahoma", 8, SWT.BOLD));
        cLabel1 = new CLabel(this, SWT.NONE);
        cLabel1.setText("Instance:");
        instanceCLabel = new CLabel(this, SWT.NONE);
        instanceCLabel.setText("CLabel");
        instanceCLabel.setFont(new Font(Display.getDefault(), "Tahoma", 8, SWT.BOLD));
        cLabel2 = new CLabel(this, SWT.NONE);
        cLabel2.setText("Database:");
        databaseCLabel = new CLabel(this, SWT.NONE);
        databaseCLabel.setText("CLabel");
        databaseCLabel.setFont(new Font(Display.getDefault(), "Tahoma", 8, SWT.BOLD));
        cLabel3 = new CLabel(this, SWT.NONE);
        cLabel3.setText("User:");
        userCLabel = new CLabel(this, SWT.NONE);
        userCLabel.setText("CLabel");
        userCLabel.setFont(new Font(Display.getDefault(), "Tahoma", 8, SWT.BOLD));
        GridLayout gridLayout = new GridLayout();
        gridLayout.numColumns = 8;
        this.setLayout(gridLayout);
        this.setSize(new Point(397, 57));
    }

    /**
     * @param text
     * @see org.eclipse.swt.custom.CLabel#setText(java.lang.String)
     */
    public void setHostText(String text) {
        hostCLabel.setText(text);
    }

    /**
     * @param text
     * @see org.eclipse.swt.custom.CLabel#setText(java.lang.String)
     */
    public void setInstanceText(String text) {
        instanceCLabel.setText(text);
    }

    /**
     * @param text
     * @see org.eclipse.swt.custom.CLabel#setText(java.lang.String)
     */
    public void setDatabaseText(String text) {
        databaseCLabel.setText(text);
    }

    /**
     * @param text
     * @see org.eclipse.swt.custom.CLabel#setText(java.lang.String)
     */
    public void setUserText(String text) {
        userCLabel.setText(text);
    }

}  //  @jve:decl-index=0:visual-constraint="10,10"
