package com.ingres.demoapp.ui.editors;

import org.eclipse.core.runtime.Assert;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.ui.IEditorInput;
import org.eclipse.ui.IPersistableElement;

import com.ingres.demoapp.model.RoutesCriteria;

/**
 * Descriptor for a Routes Criteria Editor's (<code>RoutesCriteriaEditor</code>)
 * input.
 */
public class RoutesCriteriaInput implements IEditorInput {

    private final RoutesCriteria criteria;

    /**
     * @param criteria
     *            the input object
     */
    public RoutesCriteriaInput(RoutesCriteria criteria) {
        Assert.isNotNull(criteria);
        this.criteria = criteria;
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
     * Returns the name of this object. At the moment always the string "Route
     * search criteria".
     * 
     * @return the associated name
     * 
     * @see org.eclipse.ui.IEditorInput#getName()
     */
    public String getName() {
        return "Route search criteria";
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
     * Returns the name of this object. At the moment always the string "Route
     * search criteria".
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
     * <code>com.ingres.demoapp.model.RoutesCriteria</code> is recognized.
     * 
     * @param adapter
     *            the adapter class to look up
     * @return the UserProfile used to initialize the object if the parameter is
     *         <code>RoutesCriteria.class</code> otherwise <code>null</code>
     * 
     * @see org.eclipse.core.runtime.IAdaptable#getAdapter(java.lang.Class)
     */
    public Object getAdapter(final Class adapter) {
        Object result = null;
        if (adapter == RoutesCriteria.class) {
            result = criteria;
        }
        return result;
    }

}
