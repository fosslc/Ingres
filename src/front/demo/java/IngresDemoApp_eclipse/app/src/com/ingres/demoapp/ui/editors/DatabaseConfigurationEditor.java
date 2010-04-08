package com.ingres.demoapp.ui.editors;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.IEditorInput;
import org.eclipse.ui.IEditorSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.part.EditorPart;

import com.ingres.demoapp.model.DatabaseConfiguration;
import com.ingres.demoapp.persistence.DatabaseConfigurationDAO;
import com.ingres.demoapp.ui.HelpConstants;

public class DatabaseConfigurationEditor extends EditorPart {

    public static final String ID = "com.ingres.demoapp.ui.perspectives.editors.DatabaseConfigurationEditor";

    private Composite top = null;

    private Button defaultSourceRadioButton = null;

    private Button definedSourceRadioButton = null;

    private Group group = null;

    private Group serverGroup = null;

    private Group credentialsGroup = null;

    private Group databaseGroup = null;

    private Button makeDefaultSourceCheckBox = null;

    private Label hostNameLabel = null;

    private Label instanceNameLabel = null;

    private Text hostNameText = null;

    private Label ingesLabel = null;

    private Text instanceNameText = null;

    private Label userIdLabel = null;

    private Label passwordLabel = null;

    private Text userIdText = null;

    private Text passwordText = null;

    private Text databaseText = null;

    private Composite composite = null;

    private Button changeButton = null;

    private Button resetButton = null;

    private DatabaseConfiguration databaseConfiguration; // @jve:decl-index=0:

    public void doSave(IProgressMonitor monitor) {
        // empty
    }

    public void doSaveAs() {
        // empty
    }

    public void init(IEditorSite site, IEditorInput input) throws PartInitException {
        Object adapater;
        if ((adapater = input.getAdapter(DatabaseConfiguration.class)) != null) {
            databaseConfiguration = (DatabaseConfiguration) adapater;
        } else {
            databaseConfiguration = null;
        }
        setSite(site);
        setInput(input);
    }

    public boolean isDirty() {
        return false;
    }

    public boolean isSaveAsAllowed() {
        return false;
    }

    public void setFocus() {
        // empty
    }

    public void createPartControl(Composite parent) {
        top = new Composite(parent, SWT.NONE);
        top.setLayout(new GridLayout());

        defaultSourceRadioButton = new Button(top, SWT.RADIO);
        defaultSourceRadioButton.setText("Default source");
        defaultSourceRadioButton.setSelection(true);
        defaultSourceRadioButton.addSelectionListener(new org.eclipse.swt.events.SelectionAdapter() {
            public void widgetSelected(org.eclipse.swt.events.SelectionEvent e) {
                configureDefault();
            }
        });
        defaultSourceRadioButton.addSelectionListener(new org.eclipse.swt.events.SelectionAdapter() {
            public void widgetSelected(org.eclipse.swt.events.SelectionEvent e) {
                if (((Button) e.getSource()).getSelection()) {
                    DatabaseConfigurationDAO.INSTANCE.setDefaultDatabaseConfiguration();
                }
            }
        });
        definedSourceRadioButton = new Button(top, SWT.RADIO);
        definedSourceRadioButton.setText("Defined source");
        definedSourceRadioButton.addSelectionListener(new org.eclipse.swt.events.SelectionAdapter() {
            public void widgetSelected(org.eclipse.swt.events.SelectionEvent e) {
                configureDefined();
            }
        });
        createGroup();
        createComposite();

        initializeToolTips();
        initializeDynamicHelp();
    }

    /**
     * This method initializes group
     * 
     */
    private void createGroup() {
        GridData gridData = new GridData();
        gridData.horizontalSpan = 3;
        GridLayout gridLayout = new GridLayout();
        gridLayout.numColumns = 3;
        gridLayout.makeColumnsEqualWidth = false;
        group = new Group(top, SWT.NONE);
        group.setEnabled(true);
        createServerGroup();
        group.setLayout(gridLayout);
        createCredentialsGroup();
        createDatabaseGroup();
        makeDefaultSourceCheckBox = new Button(group, SWT.CHECK);
        makeDefaultSourceCheckBox.setText("Make defined source default");
        makeDefaultSourceCheckBox.setEnabled(false);
        makeDefaultSourceCheckBox.setLayoutData(gridData);
    }

    /**
     * This method initializes serverGroup
     * 
     */
    private void createServerGroup() {
        GridData gridData3 = new GridData();
        gridData3.verticalAlignment = org.eclipse.swt.layout.GridData.BEGINNING;
        GridData gridData2 = new GridData();
        gridData2.widthHint = 30;
        GridData gridData1 = new GridData();
        gridData1.horizontalSpan = 2;
        gridData1.widthHint = 125;
        GridLayout gridLayout1 = new GridLayout();
        gridLayout1.numColumns = 3;
        serverGroup = new Group(group, SWT.NONE);
        serverGroup.setText("Server");
        serverGroup.setLayoutData(gridData3);
        serverGroup.setLayout(gridLayout1);
        hostNameLabel = new Label(serverGroup, SWT.NONE);
        hostNameLabel.setText("Host name");
        hostNameText = new Text(serverGroup, SWT.BORDER);
        hostNameText.setEditable(false);
        hostNameText.setLayoutData(gridData1);
        instanceNameLabel = new Label(serverGroup, SWT.NONE);
        instanceNameLabel.setText("Instance name");
        ingesLabel = new Label(serverGroup, SWT.NONE);
        ingesLabel.setText("Ingres");
        instanceNameText = new Text(serverGroup, SWT.BORDER);
        instanceNameText.setEnabled(true);
        instanceNameText.setEditable(false);
        instanceNameText.setTextLimit(2);
        instanceNameText.setLayoutData(gridData2);
    }

    /**
     * This method initializes credentialsGgroup
     * 
     */
    private void createCredentialsGroup() {
        GridData gridData8 = new GridData();
        gridData8.widthHint = 125;
        GridData gridData7 = new GridData();
        gridData7.horizontalAlignment = org.eclipse.swt.layout.GridData.FILL;
        gridData7.widthHint = 125;
        GridData gridData4 = new GridData();
        gridData4.verticalAlignment = org.eclipse.swt.layout.GridData.BEGINNING;
        GridLayout gridLayout2 = new GridLayout();
        gridLayout2.numColumns = 2;
        credentialsGroup = new Group(group, SWT.NONE);
        credentialsGroup.setText("Credentials");
        credentialsGroup.setLayoutData(gridData4);
        credentialsGroup.setLayout(gridLayout2);
        userIdLabel = new Label(credentialsGroup, SWT.NONE);
        userIdLabel.setText("User ID");
        userIdText = new Text(credentialsGroup, SWT.BORDER);
        userIdText.setEnabled(true);
        userIdText.setLayoutData(gridData7);
        userIdText.setEditable(false);
        passwordLabel = new Label(credentialsGroup, SWT.NONE);
        passwordLabel.setText("Password");
        passwordText = new Text(credentialsGroup, SWT.BORDER);
        passwordText.setEnabled(true);
        passwordText.setEchoChar('*');
        passwordText.setLayoutData(gridData8);
        passwordText.setEditable(false);
    }

    /**
     * This method initializes databaseGroup
     * 
     */
    private void createDatabaseGroup() {
        GridData gridData9 = new GridData();
        gridData9.widthHint = 125;
        GridData gridData5 = new GridData();
        gridData5.verticalAlignment = org.eclipse.swt.layout.GridData.FILL;
        databaseGroup = new Group(group, SWT.NONE);
        databaseGroup.setLayout(new GridLayout());
        databaseGroup.setLayoutData(gridData5);
        databaseGroup.setText("Database");
        databaseText = new Text(databaseGroup, SWT.BORDER);
        databaseText.setEnabled(true);
        databaseText.setLayoutData(gridData9);
        databaseText.setEditable(false);
    }

    /**
     * This method initializes composite
     * 
     */
    private void createComposite() {
        GridData gridData6 = new GridData();
        gridData6.horizontalAlignment = org.eclipse.swt.layout.GridData.END;
        GridLayout gridLayout3 = new GridLayout();
        gridLayout3.numColumns = 2;
        gridLayout3.marginWidth = 0;
        gridLayout3.marginHeight = 0;
        composite = new Composite(top, SWT.NONE);
        composite.setLayout(gridLayout3);
        composite.setLayoutData(gridData6);
        changeButton = new Button(composite, SWT.NONE);
        changeButton.setText("Change");
        changeButton.setEnabled(false);
        changeButton.addSelectionListener(new org.eclipse.swt.events.SelectionAdapter() {
            public void widgetSelected(org.eclipse.swt.events.SelectionEvent e) {
                databaseConfiguration.setDatabase(databaseText.getText());
                databaseConfiguration.setDefaultConfiguration(false);
                databaseConfiguration.setHost(hostNameText.getText());
                databaseConfiguration.setPassword(passwordText.getText());
                databaseConfiguration.setPort(instanceNameText.getText());
                databaseConfiguration.setUser(userIdText.getText());
                if (makeDefaultSourceCheckBox.getSelection()) {
                    databaseConfiguration.setDefaultConfiguration(true);
                }
                DatabaseConfigurationDAO.INSTANCE.setDatabaseConfiguration(databaseConfiguration);
            }
        });
        resetButton = new Button(composite, SWT.NONE);
        resetButton.setText("Reset");
        resetButton.setEnabled(false);
        resetButton.addSelectionListener(new org.eclipse.swt.events.SelectionAdapter() {
            public void widgetSelected(org.eclipse.swt.events.SelectionEvent e) {
                DatabaseConfiguration config = DatabaseConfigurationDAO.INSTANCE.getDatabaseConfiguration();
                DatabaseConfigurationEditorInput input = new DatabaseConfigurationEditorInput(config);
                setInput(input);
                populate();
            }
        });

        populate();
    }

    public void populate() {
        databaseText.setText(databaseConfiguration.getDatabase());
        hostNameText.setText(databaseConfiguration.getHost());
        passwordText.setText(databaseConfiguration.getPassword());
        instanceNameText.setText(databaseConfiguration.getPort());
        userIdText.setText(databaseConfiguration.getUser());

        if (databaseConfiguration.isDefaultConfiguration()) {
            defaultSourceRadioButton.setSelection(true);
            definedSourceRadioButton.setSelection(false);
            configureDefault();
        } else {
            defaultSourceRadioButton.setSelection(false);
            definedSourceRadioButton.setSelection(true);
            configureDefined();
        }
    }

    private void configureDefined() {
        hostNameText.setEnabled(true);
        instanceNameText.setEnabled(true);
        userIdText.setEnabled(true);
        passwordText.setEnabled(true);
        databaseText.setEnabled(true);
        hostNameText.setEditable(true);
        instanceNameText.setEditable(true);
        userIdText.setEditable(true);
        passwordText.setEditable(true);
        databaseText.setEditable(true);
        makeDefaultSourceCheckBox.setEnabled(true);
        changeButton.setEnabled(true);
        resetButton.setEnabled(true);
        serverGroup.setEnabled(true);
        hostNameLabel.setEnabled(true);
        instanceNameLabel.setEnabled(true);
        credentialsGroup.setEnabled(true);
        userIdLabel.setEnabled(true);
        passwordLabel.setEnabled(true);
        databaseGroup.setEnabled(true);
        ingesLabel.setEnabled(true);
    }

    private void configureDefault() {
        hostNameText.setEnabled(false);
        instanceNameText.setEnabled(false);
        userIdText.setEnabled(false);
        passwordText.setEnabled(false);
        databaseText.setEnabled(false);
        makeDefaultSourceCheckBox.setEnabled(false);
        changeButton.setEnabled(false);
        resetButton.setEnabled(false);
        serverGroup.setEnabled(false);
        hostNameLabel.setEnabled(false);
        instanceNameLabel.setEnabled(false);
        credentialsGroup.setEnabled(false);
        userIdLabel.setEnabled(false);
        passwordLabel.setEnabled(false);
        databaseGroup.setEnabled(false);
        ingesLabel.setEnabled(false);
    }

    public void initializeToolTips() {
        defaultSourceRadioButton.setToolTipText("Instance Data Source\nUse the default settings for connection");
        definedSourceRadioButton.setToolTipText("Instance Data Source\nChange the settings to define a data source");
        hostNameText.setToolTipText("Instance Data Source\nHost name of Ingres instance");
        instanceNameText.setToolTipText("Instance Data Source\nIngres instance identifier");
        userIdText.setToolTipText("Instance Data Source\nUser ID of the user connecting to Ingres");
        passwordText.setToolTipText("Instance Data Source\nPassword for the user ID");
        databaseText.setToolTipText("Instance Data Source\nDatabase name");
        makeDefaultSourceCheckBox.setToolTipText("Instance Data Source\nMake this data source the default");
        changeButton.setToolTipText("Instance Data Source\nChange the target instance data source and attempt a connection");
        resetButton.setToolTipText("Instance Data Source\nReset the form to the default data source");
    }

    public void initializeDynamicHelp() {
        PlatformUI.getWorkbench().getHelpSystem().setHelp(getSite().getShell(), HelpConstants.DATABASE_CONFIGURATION_EDITOR);
    }

}
