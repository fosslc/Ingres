package com.ingres.demoapp.ui.editors;

import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.ui.IEditorInput;
import org.eclipse.ui.IPersistableElement;

import com.ingres.demoapp.model.DatabaseConfiguration;

public class DatabaseConfigurationEditorInput implements IEditorInput {
    
    DatabaseConfiguration databaseConfiguration;

    public DatabaseConfigurationEditorInput(DatabaseConfiguration databaseConfiguration) {
        this.databaseConfiguration = databaseConfiguration;
    }

    public boolean exists() {
        return false;
    }

    public ImageDescriptor getImageDescriptor() {
        return null;
    }

    public String getName() {
        return databaseConfiguration.toString();
    }

    public IPersistableElement getPersistable() {
        return null;
    }

    public String getToolTipText() {
        return databaseConfiguration.toString();
    }

    public Object getAdapter(Class adapter) {
        if (adapter == DatabaseConfiguration.class) {
            return databaseConfiguration;
        }
        return null;
    }

    public int hashCode() {
        final int PRIME = 31;
        int result = 1;
        result = PRIME * result + ((databaseConfiguration == null) ? 0 : databaseConfiguration.hashCode());
        return result;
    }

    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;
        final DatabaseConfigurationEditorInput other = (DatabaseConfigurationEditorInput) obj;
        if (databaseConfiguration == null) {
            if (other.databaseConfiguration != null)
                return false;
        } else if (!databaseConfiguration.equals(other.databaseConfiguration))
            return false;
        return true;
    }

}
