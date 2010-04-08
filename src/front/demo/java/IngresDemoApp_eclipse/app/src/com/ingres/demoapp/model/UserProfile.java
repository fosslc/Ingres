package com.ingres.demoapp.model;

/**
 * Instances of this class represents a users profile. It supports the following
 * properties:<br>
 * <ul>
 * <li>first name</li>
 * <li>last name</li>
 * <li>email address</li>
 * <li>home airport code</li>
 * <li>image</li>
 * <li>default profile</li>
 * </ul>
 */
public class UserProfile {

    private String lastName = "";

    private String firstName = "";

    private String emailAddress = "";

    private String iataCode = "";

    private byte[] image;

    private boolean defaultProfile = false;

    /**
     * @return the defaultProfile
     */
    public boolean isDefaultProfile() {
        return defaultProfile;
    }

    /**
     * @param defaultProfile
     *            the defaultProfile to set
     */
    public void setDefaultProfile(final boolean defaultProfile) {
        this.defaultProfile = defaultProfile;
    }

    /**
     * @return the emailAddress
     */
    public String getEmailAddress() {
        return emailAddress;
    }

    /**
     * @param emailAddress
     *            the emailAddress to set
     */
    public void setEmailAddress(final String emailAddress) {
        this.emailAddress = emailAddress;
    }

    /**
     * @return the firstName
     */
    public String getFirstName() {
        return firstName;
    }

    /**
     * @param firstName
     *            the firstName to set
     */
    public void setFirstName(final String firstName) {
        this.firstName = firstName;
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
     * @return the image
     */
    public byte[] getImage() {
        return image;
    }

    /**
     * @param image
     *            the image to set
     */
    public void setImage(final byte[] image) {
        this.image = image;
    }

    /**
     * @return the lastName
     */
    public String getLastName() {
        return lastName;
    }

    /**
     * @param lastName
     *            the lastName to set
     */
    public void setLastName(final String lastName) {
        this.lastName = lastName;
    }

}