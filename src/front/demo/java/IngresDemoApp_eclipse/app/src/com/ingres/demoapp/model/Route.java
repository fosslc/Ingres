package com.ingres.demoapp.model;

import java.util.Date;

/**
 * Route for defining a flying route between two airports. It supports the
 * following properties:<br>
 * <ul>
 * <li>airline</li>
 * <li>iatacode</li>
 * <li>name</li>
 * <li>flight number</li>
 * <li>departing airport</li>
 * <li>arriving airport</li>
 * <li>departing time</li>
 * <li>arriving time</li>
 * <li>arriving offset</li>
 * <li>flightdays</li>
 * </ul>
 */
public class Route {

    private String airline;

    private String iataCode;

    private String name;

    private Integer flightNum;

    private String departFrom;

    private String arriveTo;

    private Date departAt;

    private Date arriveAt;

    private Integer arriveOffset;

    private String flightDays;

    /**
     * @return the airline
     */
    public String getAirline() {
        return airline;
    }

    /**
     * @param airline
     *            the airline to set
     */
    public void setAirline(final String airline) {
        this.airline = airline;
    }

    /**
     * @return the arriveAt
     */
    public Date getArriveAt() {
        return arriveAt;
    }

    /**
     * @param arriveAt
     *            the arriveAt to set
     */
    public void setArriveAt(final Date arriveAt) {
        this.arriveAt = arriveAt;
    }

    /**
     * @return the arriveOffset
     */
    public Integer getArriveOffset() {
        return arriveOffset;
    }

    /**
     * @param arriveOffset
     *            the arriveOffset to set
     */
    public void setArriveOffset(final Integer arriveOffset) {
        this.arriveOffset = arriveOffset;
    }

    /**
     * @return the arriveTo
     */
    public String getArriveTo() {
        return arriveTo;
    }

    /**
     * @param arriveTo
     *            the arriveTo to set
     */
    public void setArriveTo(final String arriveTo) {
        this.arriveTo = arriveTo;
    }

    /**
     * @return the departAt
     */
    public Date getDepartAt() {
        return departAt;
    }

    /**
     * @param departAt
     *            the departAt to set
     */
    public void setDepartAt(final Date departAt) {
        this.departAt = departAt;
    }

    /**
     * @return the departFrom
     */
    public String getDepartFrom() {
        return departFrom;
    }

    /**
     * @param departFrom
     *            the departFrom to set
     */
    public void setDepartFrom(final String departFrom) {
        this.departFrom = departFrom;
    }

    /**
     * @return the flightDays
     */
    public String getFlightDays() {
        return flightDays;
    }

    /**
     * @param flightDays
     *            the flightDays to set
     */
    public void setFlightDays(final String flightDays) {
        this.flightDays = flightDays;
    }

    /**
     * @return the flightNum
     */
    public Integer getFlightNum() {
        return flightNum;
    }

    /**
     * @param flightNum
     *            the flightNum to set
     */
    public void setFlightNum(final Integer flightNum) {
        this.flightNum = flightNum;
    }

    /**
     * @return the iataCode
     */
    public String getIataCode() {
        return iataCode;
    }

    /**
     * @param iataCode
     *            the iataCode to set
     */
    public void setIataCode(final String iataCode) {
        this.iataCode = iataCode;
    }

    /**
     * @return the name
     */
    public String getName() {
        return name;
    }

    /**
     * @param name
     *            the name to set
     */
    public void setName(final String name) {
        this.name = name;
    }

}
