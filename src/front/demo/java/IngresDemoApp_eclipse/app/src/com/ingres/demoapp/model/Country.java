package com.ingres.demoapp.model;

/**
 * Representation of an country. The following properties are supported:<br>
 * <ul>
 * <li>country code</li>
 * <li>country name</li>
 * </ul>
 */
public class Country {

    private String ctCode;

    private String ctName;

    /**
     * @return the ctCode
     */
    public String getCtCode() {
        return ctCode;
    }

    /**
     * @param ctCode
     *            the ctCode to set
     */
    public void setCtCode(final String ctCode) {
        this.ctCode = ctCode;
    }

    /**
     * @return the ctName
     */
    public String getCtName() {
        return ctName;
    }

    /**
     * @param ctName
     *            the ctName to set
     */
    public void setCtName(final String ctName) {
        this.ctName = ctName;
    }

    /*
     * (non-Javadoc)
     * 
     * @see java.lang.Object#hashCode()
     */
    public int hashCode() {
        final int PRIME = 31;
        int result = 1;
        result = PRIME * result + ((ctCode == null) ? 0 : ctCode.hashCode());
        return result;
    }

    /*
     * (non-Javadoc)
     * 
     * @see java.lang.Object#equals(java.lang.Object)
     */
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;
        final Country other = (Country) obj;
        if (ctCode == null) {
            if (other.ctCode != null)
                return false;
        } else if (!ctCode.equals(other.ctCode))
            return false;
        return true;
    }

}