package com.ingres.demoapp.ui.views;

import java.io.ByteArrayInputStream;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.IPartListener2;
import org.eclipse.ui.IWorkbenchPartReference;
import org.eclipse.ui.part.ViewPart;

import com.ingres.demoapp.DemoApp;
import com.ingres.demoapp.model.UserProfile;
import com.ingres.demoapp.ui.editors.UserProfileModifyEditor;
import com.ingres.demoapp.ui.editors.UserProfileNewEditor;

/**
 * This class implements a view for displaying detailed profile information. It
 * shows the following profile properties:<br>
 * <ul>
 * <li>First name</li>
 * <li>Email address</li>
 * <li>Preferred airport</li>
 * <li>Image (default image if no image available)</li>
 * </ul>
 * When no user profile is available the fields are left blank.
 */
public class ProfileResultView extends ViewPart implements IPartListener2 {

    /**
     * The ID of this view. Needs to be whatever is mentioned in plugin.xml.
     */
    public static final String ID = "com.ingres.demoapp.ui.views.ProfileResultView";

    private Composite top = null;

    private Label label = null;

    private Label nameLabel = null;

    /**
     * Canvas for displaying the users image.
     */
    private Canvas photographCanvas = null;

    private Label label1 = null;

    private Label label2 = null;

    /**
     * Label for showing the users preferred email address.
     */
    private Label emailAddressLabel = null;

    /**
     * Label for showing the users preferred Airport.
     */
    private Label airportPreferenceLabel = null;

    /**
     * Image from shown user profile. Can be <code>null</code>.
     */
    private Image image; // @jve:decl-index=0:

    /**
     * Default image. Shown when no image from user profile available.
     */
    private Image defaultImage; // @jve:decl-index=0:

    /**
     * GC (graphical context) for displayed image.
     */
    private GC gc;

    /**
     * Creates the SWT controls for this view and registers the instance as
     * IPartListener at
     * 
     * <code>
     * PartService
     * </code>.
     * 
     * @param parent
     *            the parent control
     * 
     * @see IWorkbenchPart
     * 
     * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
     * 
     */
    public void createPartControl(final Composite parent) {
        final GridData gridData5 = new GridData();
        gridData5.horizontalSpan = 3;
        gridData5.horizontalAlignment = org.eclipse.swt.layout.GridData.FILL;
        final GridData gridData4 = new GridData();
        gridData4.verticalAlignment = org.eclipse.swt.layout.GridData.BEGINNING;
        gridData4.horizontalAlignment = org.eclipse.swt.layout.GridData.FILL;
        gridData4.horizontalIndent = 5;
        final GridData gridData3 = new GridData();
        gridData3.verticalAlignment = org.eclipse.swt.layout.GridData.BEGINNING;
        gridData3.horizontalIndent = 5;
        final GridData gridData2 = new GridData();
        gridData2.verticalAlignment = org.eclipse.swt.layout.GridData.BEGINNING;
        gridData2.horizontalAlignment = org.eclipse.swt.layout.GridData.FILL;
        gridData2.widthHint = 300;
        gridData2.horizontalIndent = 5;
        final GridData gridData1 = new GridData();
        gridData1.verticalAlignment = org.eclipse.swt.layout.GridData.BEGINNING;
        gridData1.horizontalIndent = 5;
        final GridLayout gridLayout = new GridLayout();
        gridLayout.numColumns = 4;
        gridLayout.marginWidth = 40;
        gridLayout.marginHeight = 30;
        top = new Composite(parent, SWT.NONE);
        top.setLayout(gridLayout);
        label = new Label(top, SWT.NONE);
        label.setText("Welcome");
        label.setFont(new Font(Display.getDefault(), "Tahoma", 14, SWT.BOLD));
        nameLabel = new Label(top, SWT.NONE);
        nameLabel.setLayoutData(gridData5);
        nameLabel.setFont(new Font(Display.getDefault(), "Tahoma", 14, SWT.BOLD));
        createPhotographCanvas();
        label1 = new Label(top, SWT.NONE);
        label1.setText("Email Address");
        label1.setLayoutData(gridData1);
        label1.setFont(new Font(Display.getDefault(), "Tahoma", 12, SWT.NORMAL));
        emailAddressLabel = new Label(top, SWT.NONE);
        emailAddressLabel.setLayoutData(gridData2);
        emailAddressLabel.setFont(new Font(Display.getDefault(), "Tahoma", 12, SWT.BOLD));
        label2 = new Label(top, SWT.NONE);
        label2.setText("Airport Preference");
        label2.setLayoutData(gridData3);
        label2.setFont(new Font(Display.getDefault(), "Tahoma", 12, SWT.NORMAL));
        airportPreferenceLabel = new Label(top, SWT.NONE);
        airportPreferenceLabel.setLayoutData(gridData4);
        airportPreferenceLabel.setFont(new Font(Display.getDefault(), "Tahoma", 12, SWT.BOLD));

        defaultImage = DemoApp.getImageDescriptor("/images/NoPicture.bmp").createImage();

        getSite().getWorkbenchWindow().getPartService().addPartListener(this);
    }

    /**
     * No functionality
     * 
     * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
     */
    public void setFocus() {
        // empty
    }

    /**
     * This method initializes photographCanvas
     */
    private void createPhotographCanvas() {
        final GridData gridData = new GridData();
        gridData.verticalSpan = 2;
        gridData.widthHint = 130;
        gridData.heightHint = 160;
        gridData.horizontalSpan = 2;
        photographCanvas = new Canvas(top, SWT.NONE);
        photographCanvas.setLayoutData(gridData);
        gc = new GC(photographCanvas);

        // show the actual image whenever a repaint is requested
        photographCanvas.addPaintListener(new PaintListener() {
            public void paintControl(PaintEvent event) {
                showImage();
            }
        });
    }

    /**
     * If the given partRef's <code>getAdapter</code> returns an object of the
     * type <code>UserProfile</code>, the view is initialized with the
     * objects content otherwise it's reset to blank.
     * 
     * @param partRef
     *            the part whose input was changed
     * 
     * @see org.eclipse.ui.IPartListener2#partActivated(org.eclipse.ui.IWorkbenchPartReference)
     */
    public void partActivated(final IWorkbenchPartReference partRef) {
        partInputChanged(partRef);
    }

    /**
     * No functionality
     * 
     * @param partRef
     *            the part that is visible
     * 
     * @see org.eclipse.ui.IPartListener2#partBroughtToTop(org.eclipse.ui.IWorkbenchPartReference)
     */
    public void partBroughtToTop(final IWorkbenchPartReference partRef) {
        // empty
    }

    /**
     * No functionality
     * 
     * @param partRef
     *            the part that is visible
     * 
     * @see org.eclipse.ui.IPartListener2#partClosed(org.eclipse.ui.IWorkbenchPartReference)
     */
    public void partClosed(final IWorkbenchPartReference partRef) {
        // empty
    }

    /**
     * No functionality
     * 
     * @param partRef
     *            the part that is visible
     * 
     * @see org.eclipse.ui.IPartListener2#partDeactivated(org.eclipse.ui.IWorkbenchPartReference)
     */
    public void partDeactivated(final IWorkbenchPartReference partRef) {
        // empty
    }

    /**
     * No functionality
     * 
     * @param partRef
     *            the part that is visible
     * 
     * @see org.eclipse.ui.IPartListener2#partHidden(org.eclipse.ui.IWorkbenchPartReference)
     */
    public void partHidden(final IWorkbenchPartReference partRef) {
        // empty
    }

    /**
     * If the given partRef's <code>getAdapter</code> returns an object of the
     * type <code>UserProfile</code>, the view is initialized with the
     * objects content otherwise it is not changed. If the UserProfile dosesn't
     * contains image data a default picture is shown.
     * 
     * @param partRef
     *            the part whose input was changed
     * 
     * @see org.eclipse.ui.IPartListener2#partInputChanged(org.eclipse.ui.IWorkbenchPartReference)
     */
    public void partInputChanged(final IWorkbenchPartReference partRef) {
        if (partRef.getId().equals(UserProfileModifyEditor.ID) || partRef.getId().equals(UserProfileNewEditor.ID)) {
            final UserProfile userProfile = (UserProfile) partRef.getPart(false).getAdapter(UserProfile.class);
            if (userProfile != null) {
                nameLabel.setText(userProfile.getFirstName());
                emailAddressLabel.setText(userProfile.getEmailAddress());
                airportPreferenceLabel.setText(userProfile.getIataCode());

                if (image != null && !image.isDisposed()) {
                    image.dispose();
                }
                if (userProfile.getImage() != null) {
                    image = new Image(photographCanvas.getDisplay(), new ByteArrayInputStream(userProfile.getImage()));
                }
                showImage();
            } else {
                nameLabel.setText("");
                emailAddressLabel.setText("");
                airportPreferenceLabel.setText("");

                if (image != null && !image.isDisposed()) {
                    image.dispose();
                }
                image = null;

                showImage();
            }
        }
    }

    /**
     * No functionality
     * 
     * @param partRef
     *            the part that is visible
     * 
     * @see org.eclipse.ui.IPartListener2#partOpened(org.eclipse.ui.IWorkbenchPartReference)
     */
    public void partOpened(final IWorkbenchPartReference partRef) {
        // empty
    }

    /**
     * No functionality
     * 
     * @param partRef
     *            the part that is visible
     * 
     * @see org.eclipse.ui.IPartListener2#partVisible(org.eclipse.ui.IWorkbenchPartReference)
     */
    public void partVisible(final IWorkbenchPartReference partRef) {
        // empty
    }

    /**
     * Displays the Image of the actual profile, if not available displays the
     * default image.
     */
    private void showImage() {
        gc.fillRectangle(0, 0, photographCanvas.getBounds().width, photographCanvas.getBounds().height);
        Image profileImage;
        if (image == null || image.isDisposed()) {
            profileImage = defaultImage;
        } else {
            profileImage = image;

        }

        // TODO resize large image
        final int left = (photographCanvas.getBounds().width - profileImage.getBounds().width) / 2;
        final int top = (photographCanvas.getBounds().height - profileImage.getBounds().height) / 2;
        gc.drawImage(profileImage, left, top);
    }

    /**
     * Dispose the image (of the actual shown profile) and the default image.
     * Unregisters from <code>PartService</code>. Afterwards the parents
     * <code>dispose()</code> implementation is called.
     * 
     * @see org.eclipse.ui.part.WorkbenchPart#dispose()
     * 
     */
    public void dispose() {
        getSite().getWorkbenchWindow().getPartService().removePartListener(this);
        if (image != null && !image.isDisposed()) {
            image.dispose();
        }
        if (defaultImage != null && !defaultImage.isDisposed()) {
            defaultImage.dispose();
        }
        super.dispose();
    }

} // @jve:decl-index=0:visual-constraint="10,10,688,274"
