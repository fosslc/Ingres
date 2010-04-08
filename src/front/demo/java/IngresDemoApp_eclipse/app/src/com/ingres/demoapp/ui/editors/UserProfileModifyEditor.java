package com.ingres.demoapp.ui.editors;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.sql.SQLException;
import java.util.List;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ComboViewer;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.IEditorInput;
import org.eclipse.ui.IEditorSite;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.part.EditorPart;

import com.ingres.demoapp.DemoApp;
import com.ingres.demoapp.model.Airport;
import com.ingres.demoapp.model.Country;
import com.ingres.demoapp.model.Region;
import com.ingres.demoapp.model.UserProfile;
import com.ingres.demoapp.persistence.AirportDAO;
import com.ingres.demoapp.persistence.CountryDAO;
import com.ingres.demoapp.persistence.UserProfileDAO;
import com.ingres.demoapp.ui.HelpConstants;
import com.ingres.demoapp.ui.widget.AirportWidget;

public class UserProfileModifyEditor extends EditorPart {

    public static final String ID = "com.ingres.demoapp.ui.perspectives.editors.UserProfileModifyEditor";

    private Group personalDetailsGroup = null;

    private Group preferencesGroup = null;

    private Button checkBox = null;

    private Composite composite = null;

    private Button addButton = null;

    private Button removeButton = null;

    private Button changeButton = null;

    private Group homeAirportGroup = null;

    private AirportWidget homeAirport = null;

    private Composite composite1 = null;

    private Button newButton = null;

    private Button modifyButton = null;

    private Label emailAddressLabel = null;

    private Label lastNameLabel = null;

    private Label firstNameLabel = null;

    private Label photographLabel = null;

    private Text lastNameText = null;

    private Text firstNameText = null;

    private Text photographText = null;

    private Button browseButton = null;

    private Composite top = null;

    private UserProfile userProfile; // @jve:decl-index=0:

    private Combo emailAddressCombo = null;

    private ComboViewer emailAddressComboViewer = null;

    public void doSave(IProgressMonitor monitor) {
        userProfile.setIataCode(homeAirport.getAirportCodeCombo().getText());
        userProfile.setEmailAddress(emailAddressCombo.getText());
        userProfile.setFirstName(firstNameText.getText());
        userProfile.setLastName(lastNameText.getText());
        userProfile.setDefaultProfile(checkBox.getSelection());
        if (photographText.getText().trim().length() > 0) {
            try {
                InputStream in = new FileInputStream(new File(photographText.getText()));
                ByteArrayOutputStream out = new ByteArrayOutputStream();

                byte[] buf = new byte[2048];
                int avail;
                while ((avail = in.read(buf)) != -1) {
                    out.write(buf, 0, avail);
                }

                userProfile.setImage(out.toByteArray());
            } catch (IOException ex) {
                ex.printStackTrace();
            }
        }
        try {
            UserProfileDAO.update(userProfile);
            setInputWithNotify(new UserProfileInput(userProfile));
            populate();
        } catch (Exception e1) {
            e1.printStackTrace();
        }
    }

    public void doSaveAs() {
        // empty
    }

    public void init(IEditorSite site, IEditorInput input) throws PartInitException {
        Object adapater;
        if ((adapater = input.getAdapter(UserProfile.class)) != null) {
            userProfile = (UserProfile) adapater;
        } else {
            userProfile = null;
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

    public void createPartControl(Composite parent) {
        top = parent;

        GridData gridData4 = new GridData();
        gridData4.horizontalSpan = 3;
        GridLayout gridLayout = new GridLayout();
        gridLayout.numColumns = 3;
        createPersonalDetailsGroup();
        top.setLayout(gridLayout);
        try {
            createPreferencesGroup();
        } catch (SQLException e) {
            e.printStackTrace();
            DemoApp.logError("oops", e);
        }
        top.setSize(new Point(695, 218));
        createComposite1();
        checkBox = new Button(top, SWT.CHECK);
        checkBox.setText("Default profile");
        checkBox.setLayoutData(gridData4);
        createComposite();

        initializeToolTips();
        initializeDynamicHelp();
    }

    /**
     * This method initializes personalDetailsGroup
     * 
     */
    private void createPersonalDetailsGroup() {
        GridData gridData14 = new GridData();
        gridData14.widthHint = 165;
        GridData gridData13 = new GridData();
        gridData13.horizontalAlignment = org.eclipse.swt.layout.GridData.FILL;
        GridData gridData12 = new GridData();
        gridData12.horizontalSpan = 2;
        gridData12.widthHint = 225;
        gridData12.horizontalAlignment = org.eclipse.swt.layout.GridData.FILL;
        GridData gridData11 = new GridData();
        gridData11.horizontalSpan = 2;
        gridData11.widthHint = 225;
        gridData11.horizontalAlignment = org.eclipse.swt.layout.GridData.FILL;
        GridLayout gridLayout3 = new GridLayout();
        gridLayout3.numColumns = 3;
        GridData gridData = new GridData();
        gridData.verticalAlignment = org.eclipse.swt.layout.GridData.FILL;
        personalDetailsGroup = new Group(top, SWT.NONE);
        personalDetailsGroup.setText("Personal Details");
        personalDetailsGroup.setLayout(gridLayout3);
        personalDetailsGroup.setLayoutData(gridData);
        emailAddressLabel = new Label(personalDetailsGroup, SWT.NONE);
        emailAddressLabel.setText("Email Address");
        createEmailAddressCombo();
        lastNameLabel = new Label(personalDetailsGroup, SWT.NONE);
        lastNameLabel.setText("Last Name");
        lastNameText = new Text(personalDetailsGroup, SWT.BORDER);
        lastNameText.setLayoutData(gridData11);
        firstNameLabel = new Label(personalDetailsGroup, SWT.NONE);
        firstNameLabel.setText("First Name");
        firstNameText = new Text(personalDetailsGroup, SWT.BORDER);
        firstNameText.setLayoutData(gridData12);
        photographLabel = new Label(personalDetailsGroup, SWT.NONE);
        photographLabel.setText("Photograph");
        photographText = new Text(personalDetailsGroup, SWT.BORDER);
        photographText.setLayoutData(gridData14);
        browseButton = new Button(personalDetailsGroup, SWT.NONE);
        browseButton.setText("Browse...");
        browseButton.setLayoutData(gridData13);
        browseButton.addSelectionListener(new org.eclipse.swt.events.SelectionAdapter() {
            public void widgetSelected(org.eclipse.swt.events.SelectionEvent e) {
                FileDialog fileDialog = new FileDialog(top.getShell(), SWT.OPEN);
                fileDialog.setFilterExtensions(new String[] { "*.jpg", "*.jpeg", "*.gif", "*.png", "*.bmp", "*.*" });

                String fileName = fileDialog.open();

                if (fileName != null) {
                    photographText.setText(fileName);
                }
            }
        });
    }

    /**
     * This method initializes preferencesGroup
     * 
     * @throws SQLException
     * 
     */
    private void createPreferencesGroup() throws SQLException {
        GridData gridData1 = new GridData();
        gridData1.verticalAlignment = org.eclipse.swt.layout.GridData.FILL;
        preferencesGroup = new Group(top, SWT.NONE);
        preferencesGroup.setLayout(new GridLayout());
        preferencesGroup.setText("Preferences");
        createHomeAirportGroup();
        preferencesGroup.setLayoutData(gridData1);
    }

    /**
     * This method initializes composite
     * 
     */
    private void createComposite() {
        GridData gridData8 = new GridData();
        gridData8.horizontalAlignment = org.eclipse.swt.layout.GridData.FILL;
        GridData gridData7 = new GridData();
        gridData7.horizontalAlignment = org.eclipse.swt.layout.GridData.FILL;
        GridData gridData6 = new GridData();
        gridData6.horizontalAlignment = org.eclipse.swt.layout.GridData.FILL;
        GridLayout gridLayout1 = new GridLayout();
        gridLayout1.numColumns = 3;
        gridLayout1.makeColumnsEqualWidth = true;
        GridData gridData5 = new GridData();
        gridData5.horizontalSpan = 3;
        gridData5.horizontalAlignment = org.eclipse.swt.layout.GridData.END;
        composite = new Composite(top, SWT.NONE);
        composite.setLayoutData(gridData5);
        composite.setLayout(gridLayout1);
        addButton = new Button(composite, SWT.NONE);
        addButton.setText("Add");
        addButton.setEnabled(false);
        addButton.setLayoutData(gridData6);
        removeButton = new Button(composite, SWT.NONE);
        removeButton.setText("Remove");
        removeButton.setLayoutData(gridData7);
        removeButton.addSelectionListener(new org.eclipse.swt.events.SelectionAdapter() {
            public void widgetSelected(org.eclipse.swt.events.SelectionEvent e) {
                try {
                    UserProfileDAO.delete(userProfile);
                    userProfile = UserProfileDAO.getDefaultUserProfile();
                    enabled = false;
                    populate();
                    setInputWithNotify(new UserProfileInput(userProfile));
                } catch (Exception e1) {
                    e1.printStackTrace();
                }

            }
        });
        changeButton = new Button(composite, SWT.NONE);
        changeButton.setText("Change");
        changeButton.setLayoutData(gridData8);

        changeButton.addSelectionListener(new org.eclipse.swt.events.SelectionAdapter() {
            public void widgetSelected(org.eclipse.swt.events.SelectionEvent e) {
                // save record
                doSave(null);
            }
        });
        initialize();
        populate();
    }

    /**
     * This method initializes homeAirportGroup
     * 
     * @throws SQLException
     * 
     */
    private void createHomeAirportGroup() throws SQLException {
        homeAirportGroup = new Group(preferencesGroup, SWT.NONE);
        homeAirportGroup.setLayout(new GridLayout());
        homeAirportGroup.setText("Home Airport");
        createHomeAirport();
    }

    /**
     * This method initializes homeAirport
     * 
     * @throws SQLException
     * 
     */
    private void createHomeAirport() throws SQLException {
        homeAirport = new AirportWidget(homeAirportGroup, SWT.NONE);
    }

    /**
     * This method initializes composite1
     * 
     */
    private void createComposite1() {
        GridLayout gridLayout2 = new GridLayout();
        gridLayout2.horizontalSpacing = 5;
        gridLayout2.marginHeight = 6;
        GridData gridData9 = new GridData();
        gridData9.horizontalAlignment = org.eclipse.swt.layout.GridData.FILL;
        gridData9.verticalAlignment = org.eclipse.swt.layout.GridData.BEGINNING;
        GridData gridData3 = new GridData();
        gridData3.horizontalAlignment = org.eclipse.swt.layout.GridData.FILL;
        gridData3.verticalAlignment = org.eclipse.swt.layout.GridData.BEGINNING;
        GridData gridData2 = new GridData();
        gridData2.verticalAlignment = org.eclipse.swt.layout.GridData.BEGINNING;
        composite1 = new Composite(top, SWT.NONE);
        composite1.setLayoutData(gridData2);
        composite1.setLayout(gridLayout2);
        newButton = new Button(composite1, SWT.NONE);
        newButton.setText("New");
        newButton.setLayoutData(gridData3);
        newButton.addSelectionListener(new org.eclipse.swt.events.SelectionAdapter() {
            public void widgetSelected(org.eclipse.swt.events.SelectionEvent e) {
                IWorkbenchPage activePage = PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage();
                activePage.closeAllEditors(true);

                UserProfile profile = UserProfileDAO.createEmptyProfile();
                UserProfileInput input = new UserProfileInput(profile);
                try {
                    activePage.openEditor(input, UserProfileNewEditor.ID);
                } catch (PartInitException ex) {
                    ex.printStackTrace();
                }

            }
        });
        modifyButton = new Button(composite1, SWT.NONE);
        modifyButton.setText("Modify");
        modifyButton.setLayoutData(gridData9);
        modifyButton.addSelectionListener(new org.eclipse.swt.events.SelectionAdapter() {
            public void widgetSelected(org.eclipse.swt.events.SelectionEvent e) {
                enabled = true;
                setEnabled(enabled);
            }
        });
    }

    public void setFocus() {
        // empty
    }

    /**
     * This method initializes emailAddressCombo
     * 
     */
    private void createEmailAddressCombo() {
        GridData gridData10 = new GridData();
        gridData10.horizontalAlignment = org.eclipse.swt.layout.GridData.FILL;
        gridData10.horizontalSpan = 2;
        emailAddressCombo = new Combo(personalDetailsGroup, SWT.READ_ONLY);
        emailAddressCombo.setLayoutData(gridData10);
        emailAddressComboViewer = new ComboViewer(emailAddressCombo);
        emailAddressComboViewer.addSelectionChangedListener(new ISelectionChangedListener() {

            public void selectionChanged(SelectionChangedEvent event) {
                try {
                    if (init) {
                        userProfile = UserProfileDAO.getUserProfile(((IStructuredSelection) event.getSelection()).getFirstElement()
                                .toString());
                        populate();
                        setInputWithNotify(new UserProfileInput(userProfile));
                    }
                } catch (Exception e1) {
                    e1.printStackTrace();
                }
            }
        });
    }

    public void setEnabled(boolean enabled) {
        personalDetailsGroup.setEnabled(enabled);
        emailAddressLabel.setEnabled(enabled);
        emailAddressCombo.setEnabled(enabled);
        lastNameLabel.setEnabled(enabled);
        lastNameText.setEnabled(enabled);
        firstNameLabel.setEnabled(enabled);
        firstNameText.setEnabled(enabled);
        photographLabel.setEnabled(enabled);
        photographText.setEnabled(enabled);
        browseButton.setEnabled(enabled);
        preferencesGroup.setEnabled(enabled);
        homeAirportGroup.setEnabled(enabled);
        homeAirport.setEnabled(enabled);
        checkBox.setEnabled(enabled);
        removeButton.setEnabled(enabled);
        changeButton.setEnabled(enabled);
    }

    boolean enabled = false;

    public void populate() {
        lastNameText.setText(userProfile.getLastName());
        firstNameText.setText(userProfile.getFirstName());
        photographText.setText("");
        checkBox.setSelection(userProfile.isDefaultProfile());
        init = false;
        emailAddressComboViewer.setSelection(new StructuredSelection(userProfile.getEmailAddress()));
        init = true;
        try {
            List airports = AirportDAO.getAirportByIataCode(userProfile.getIataCode());
            if (airports.size() > 0) {
                Airport home = (Airport) airports.get(0);
                Region region = AirportDAO.getRegionByAirport(userProfile.getIataCode());
                Country country = CountryDAO.getCountryByCountryCode(home.getApCcode());

                homeAirport.getCountryViewer().setSelection(new StructuredSelection(country), true);
                homeAirport.getRegionViewer().setSelection(new StructuredSelection(region), true);
                homeAirport.getAirportCodeViewer().setSelection(new StructuredSelection(home), true);
            } else {
                homeAirport.getCountryCombo().deselectAll();
                homeAirport.getRegionCombo().removeAll();
                homeAirport.getRegionCombo().setEnabled(false);
                homeAirport.getAirportCodeCombo().removeAll();
                homeAirport.getAirportCodeCombo().setEnabled(false);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        setEnabled(enabled);
    }

    boolean init = false;

    public void initialize() {
        try {
            init = false;
            emailAddressComboViewer.add(UserProfileDAO.getEmailAddresses().toArray(new String[] {}));
            emailAddressComboViewer.setSelection(new StructuredSelection(userProfile.getEmailAddress()));
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            init = true;
        }
    }

    public UserProfile getUserProfile() {
        return userProfile;
    }

    public void setUserProfile(UserProfile userProfile) {
        this.userProfile = userProfile;
    }

    public Object getAdapter(Class adapter) {
        Object result = super.getAdapter(adapter);
        if (result == null && UserProfile.class.equals(adapter)) {
            result = userProfile;
        }
        return result;
    }

    public void initializeToolTips() {
        emailAddressCombo.setToolTipText("Profile Information\nEnter a new email address or select one from the list for updating");
        lastNameText.setToolTipText("Profile Information\nEnter your last (family) name");
        firstNameText.setToolTipText("Profile Information\nEnter your first (given) name");
        photographText.setToolTipText("Profile Information\nEnter the path and filename to a picture file or use the \"Browse...\" button");
        browseButton.setToolTipText("Profile Information\nChoose picture file...");
        checkBox.setToolTipText("Profile Information\nMake the selected profile the default");

        newButton.setToolTipText("Profile Information\nEnables and clear the form to add new profile details");
        modifyButton.setToolTipText("Profile Information\nSwitch the form to modif mode to change profile details");
        addButton.setToolTipText("Profile Information\nAdd the details into a new profile");
        removeButton.setToolTipText("Profile Information\nRemove the selected profile");
        changeButton.setToolTipText("Profile Information\nChange the selected profile");

        homeAirport.getCountryCombo().setToolTipText("Profile Information\nChoose the country you are travelling to...");
        homeAirport.getAirportCodeCombo().setToolTipText("Profile Information\nChoose the region you are travelling to...");
        homeAirport.getRegionCombo().setToolTipText("Profile Information\nChoose the airport code you are travelling to...");
    }

    public void initializeDynamicHelp() {
        PlatformUI.getWorkbench().getHelpSystem().setHelp(getSite().getShell(), HelpConstants.USER_PROFILE_EDITOR);
    }

} // @jve:decl-index=0:visual-constraint="10,10,662,457"
