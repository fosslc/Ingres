package com.ingres.demoapp.model;

/**
 * Object for defining a database connection. The following properties are
 * supported:<br>
 * <ul>
 * <li>host</li>
 * <li>port</li>
 * <li>username</li>
 * <li>password</li>
 * <li>database</li>
 * <li>default configuration</li>
 * </ul>
 */
public class DatabaseConfiguration implements Cloneable {
    
    /* (non-Javadoc)
     * @see java.lang.Object#clone()
     */
    public Object clone() {
        DatabaseConfiguration clone = new DatabaseConfiguration();
        clone.setDatabase(getDatabase());
        clone.setDefaultConfiguration(isDefaultConfiguration());
        clone.setHost(getHost());
        clone.setPassword(getPassword());
        clone.setPort(getPort());
        clone.setUser(getUser());
        return clone;
    }

    private String host;
    
    private String port;
    
    private String user;
    
    private String password;
    
    private String database;
    
    private boolean defaultConfiguration;

    /**
     * @return the database
     */
    public String getDatabase() {
        return database;
    }

    /**
     * @param database the database to set
     */
    public void setDatabase(final String database) {
        this.database = database;
    }

    /**
     * @return the defaultConfiguration
     */
    public boolean isDefaultConfiguration() {
        return defaultConfiguration;
    }

    /**
     * @param defaultConfiguration the defaultConfiguration to set
     */
    public void setDefaultConfiguration(final boolean defaultConfiguration) {
        this.defaultConfiguration = defaultConfiguration;
    }

    /**
     * @return the host
     */
    public String getHost() {
        return host;
    }

    /**
     * @param host the host to set
     */
    public void setHost(final String host) {
        this.host = host;
    }

    /**
     * @return the password
     */
    public String getPassword() {
        return password;
    }

    /**
     * @param password the password to set
     */
    public void setPassword(final String password) {
        this.password = password;
    }

    /**
     * @return the port
     */
    public String getPort() {
        return port;
    }

    /**
     * @param port the port to set
     */
    public void setPort(final String port) {
        this.port = port;
    }

    /**
     * @return the user
     */
    public String getUser() {
        return user;
    }

    /**
     * @param user the user to set
     */
    public void setUser(final String user) {
        this.user = user;
    }

    /* (non-Javadoc)
     * @see java.lang.Object#hashCode()
     */
    public int hashCode() {
        final int PRIME = 31;
        int result = 1;
        result = PRIME * result + ((database == null) ? 0 : database.hashCode());
        result = PRIME * result + (defaultConfiguration ? 1231 : 1237);
        result = PRIME * result + ((host == null) ? 0 : host.hashCode());
        result = PRIME * result + ((password == null) ? 0 : password.hashCode());
        result = PRIME * result + ((port == null) ? 0 : port.hashCode());
        result = PRIME * result + ((user == null) ? 0 : user.hashCode());
        return result;
    }

    /* (non-Javadoc)
     * @see java.lang.Object#equals(java.lang.Object)
     */
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;
        final DatabaseConfiguration other = (DatabaseConfiguration) obj;
        if (database == null) {
            if (other.database != null)
                return false;
        } else if (!database.equals(other.database))
            return false;
        if (defaultConfiguration != other.defaultConfiguration)
            return false;
        if (host == null) {
            if (other.host != null)
                return false;
        } else if (!host.equals(other.host))
            return false;
        if (password == null) {
            if (other.password != null)
                return false;
        } else if (!password.equals(other.password))
            return false;
        if (port == null) {
            if (other.port != null)
                return false;
        } else if (!port.equals(other.port))
            return false;
        if (user == null) {
            if (other.user != null)
                return false;
        } else if (!user.equals(other.user))
            return false;
        return true;
    }

}
