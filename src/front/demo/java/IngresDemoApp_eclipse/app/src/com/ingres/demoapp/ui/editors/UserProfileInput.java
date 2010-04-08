package com.ingres.demoapp.ui.editors;

import org.eclipse.core.runtime.Assert;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.ui.IEditorInput;
import org.eclipse.ui.IPersistableElement;

import com.ingres.demoapp.model.UserProfile;

/**
 * Descriptor for the User Profile Editor's (<code>UserProfileModifyEditor</code>
 * and <code>UserProfileNewEditor</code>) input.
 */
public class UserProfileInput implements IEditorInput {

    private final UserProfile userProfile;

    /**
     * @param userProfile
     *            the input object
     */
    public UserProfileInput(UserProfile userProfile) {
        Assert.isNotNull(userProfile);
        this.userProfile = userProfile;
    }

    /**
     * Returns always false as there is no "File Most Recently Used" menu.
     * 
     * @return always <code>false</code>
     * 
     * @see org.eclipse.ui.IEditorInput#exists()
     */
    public boolean exists() {
        return false;
    }

    /**
     * Image descriptor is not required.
     * 
     * @return always <code>null</code>
     * 
     * @see org.eclipse.ui.IEditorInput#getImageDescriptor()
     */
    public ImageDescriptor getImageDescriptor() {
        return null;
    }

    /**
     * Returns the UserProfiles email address or (if <code>null</code>) an
     * empty string.
     * 
     * @return the associated name
     * 
     * @see org.eclipse.ui.IEditorInput#getName()
     */
    public String getName() {
        String name = userProfile.getEmailAddress();
        if (name == null) {
            name = "";
        }
        return name;
    }

    /**
     * Returns always <code>null</code> because the actual state of the
     * object input is not saved on workbench shutdown.
     * 
     * @return always <code>null</code>
     * 
     * @see org.eclipse.ui.IEditorInput#getPersistable()
     */
    public IPersistableElement getPersistable() {
        return null;
    }

    /**
     * Returns the UserProfiles email address or (if <code>null</code>) an
     * empty string.
     * 
     * @return the associated tooltiptext
     * 
     * @see org.eclipse.ui.IEditorInput#getToolTipText()
     */
    public String getToolTipText() {
        return getName();
    }

    /**
     * Returns an object which is an instance of the given class associated with
     * this object. At the moment only
     * <code>com.ingres.demoapp.model.UserProfile</code> is recognized.
     * 
     * @param adapter
     *            the adapter class to look up
     * @return the UserProfile used to initialize the object if the parameter is
     *         <code>UserProfile.class</code> otherwise <code>null</code>
     * 
     * @see org.eclipse.core.runtime.IAdaptable#getAdapter(java.lang.Class)
     */
    public Object getAdapter(final Class adapter) {
        Object result = null;
        if (adapter == UserProfile.class) {
            result = userProfile;
        }
        return result;
    }

}
