package com.ingres.demoapp.model;

/**
 * Representation of an airport. The following properties are supported:<br>
 * <ul>
 * <li>IATA (International Air Transport Association) code</li>
 * <li>name</li>
 * <li>country</li>
 * <li>place</li>
 * </ul>
 */
public class Airport {

    private String apIatacode;

    private String apPlace;

    private String apName;

    private String apCcode;

    /**
     * @return the apCcode
     */
    public String getApCcode() {
        return apCcode;
    }

    /**
     * @param apCcode the apCcode to set
     */
    public void setApCcode(final String apCcode) {
        this.apCcode = apCcode;
    }

    /**
     * @return the apIatacode
     */
    public String getApIatacode() {
        return apIatacode;
    }

    /**
     * @param apIatacode the apIatacode to set
     */
    public void setApIatacode(final String apIatacode) {
        this.apIatacode = apIatacode;
    }

    /**
     * @return the apName
     */
    public String getApName() {
        return apName;
    }

    /**
     * @param apName the apName to set
     */
    public void setApName(final String apName) {
        this.apName = apName;
    }

    /**
     * @return the apPlace
     */
    public String getApPlace() {
        return apPlace;
    }

    /**
     * @param apPlace the apPlace to set
     */
    public void setApPlace(final String apPlace) {
        this.apPlace = apPlace;
    }

    /* (non-Javadoc)
     * @see java.lang.Object#hashCode()
     */
    public int hashCode() {
        final int PRIME = 31;
        int result = 1;
        result = PRIME * result + ((apIatacode == null) ? 0 : apIatacode.hashCode());
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
        final Airport other = (Airport) obj;
        if (apIatacode == null) {
            if (other.apIatacode != null)
                return false;
        } else if (!apIatacode.equals(other.apIatacode))
            return false;
        return true;
    }

}