package com.ingres.demoapp.model;

/**
 * Support class for building route queries. It supports the following
 * properties:<br>
 * <ul>
 * <li>departing airport code</li>
 * <li>arriving airport code</li>
 * <li>flying days</li>
 * <li>find return journey</li>
 * </ul>
 */
public class RoutesCriteria implements Cloneable {

    /* (non-Javadoc)
     * @see java.lang.Object#clone()
     */
    public Object clone() {
        RoutesCriteria clone = new RoutesCriteria();
        clone.setArrivingAirport(getArrivingAirport());
        clone.setDepartingAirport(getDepartingAirport());
        clone.setFindReturnJourney(isFindReturnJourney());
        clone.setFlyingOn(getFlyingOn());
        return clone;
    }

    private String departingAirport;

    private String arrivingAirport;

    private boolean[] flyingOn;
    
    private boolean findReturnJourney;

    /**
     * @return the arrivingAirport
     */
    public String getArrivingAirport() {
        return arrivingAirport;
    }

    /**
     * @param arrivingAirport
     *            the arrivingAirport to set
     */
    public void setArrivingAirport(final String arrivingAirport) {
        this.arrivingAirport = arrivingAirport;
    }

    /**
     * @return the departingAirport
     */
    public String getDepartingAirport() {
        return departingAirport;
    }

    /**
     * @param departingAirport
     *            the departingAirport to set
     */
    public void setDepartingAirport(final String departingAirport) {
        this.departingAirport = departingAirport;
    }

    /**
     * @return the findReturnJourney
     */
    public boolean isFindReturnJourney() {
        return findReturnJourney;
    }

    /**
     * @param findReturnJourney
     *            the includeReturnJourney to set
     */
    public void setFindReturnJourney(final boolean findReturnJourney) {
        this.findReturnJourney = findReturnJourney;
    }

    /**
     * @return the flyingOn
     */
    public boolean[] getFlyingOn() {
        return flyingOn;
    }

    /**
     * @param flyingOn the flyingOn to set
     */
    public void setFlyingOn(boolean[] flyingOn) {
        this.flyingOn = flyingOn;
    }

}
