package com.ingres.demoapp.model;

public class Region {
    
    private String apPlace;

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
    public void setApCcode(String apCcode) {
        this.apCcode = apCcode;
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
    public void setApPlace(String apPlace) {
        this.apPlace = apPlace;
    }

    /* (non-Javadoc)
     * @see java.lang.Object#hashCode()
     */
    public int hashCode() {
        final int PRIME = 31;
        int result = 1;
        result = PRIME * result + ((apCcode == null) ? 0 : apCcode.hashCode());
        result = PRIME * result + ((apPlace == null) ? 0 : apPlace.hashCode());
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
        final Region other = (Region) obj;
        if (apCcode == null) {
            if (other.apCcode != null)
                return false;
        } else if (!apCcode.equals(other.apCcode))
            return false;
        if (apPlace == null) {
            if (other.apPlace != null)
                return false;
        } else if (!apPlace.equals(other.apPlace))
            return false;
        return true;
    }

}
