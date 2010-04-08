package com.ingres.demoapp.persistence;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.List;
import java.util.Vector;

import org.eclipse.core.runtime.Preferences;

import com.ingres.demoapp.DemoApp;
import com.ingres.demoapp.model.UserProfile;

public class UserProfileDAO extends BaseDAO {

    private static final String PREF_DEFAULT_PROFILE = "com.ingres.demoapp.persistence.UserProfileDAO.defaultprofile";

    private static Preferences preferences;

    private static Preferences getPreferences() {
        if (preferences == null) {
            preferences = DemoApp.getDefault().getPluginPreferences();
        }
        return preferences;
    }

    /**
     * insert profile
     * 
     * @param profile
     * @return row count
     * @throws Exception
     */
    public static int insert(UserProfile profile) throws Exception {
        Connection con = null;
        int count = 0;
        try {

            con = getConnection();
            con.setAutoCommit(false);

            PreparedStatement stmt = con
                    .prepareStatement("INSERT INTO user_profile (up_id, up_last, up_first, up_airport, up_image, up_email) VALUES(next value FOR up_id, ?, ?, ?, ?, ?)");
            stmt.setString(1, profile.getLastName());
            stmt.setString(2, profile.getFirstName());
            stmt.setString(3, profile.getIataCode());
            stmt.setBytes(4, profile.getImage());
            stmt.setString(5, profile.getEmailAddress());

            count = stmt.executeUpdate();

            con.commit();
            con.setAutoCommit(true);
        } catch (SQLException se) {
            con.rollback();
            con.setAutoCommit(true);
        } finally {
            if (con != null) {
                recycle(con);
            }
        }

        if (profile.isDefaultProfile()) {
            storeDefaultUserProfile(profile.getEmailAddress());
        }

        return count;
    }

    /**
     * updates profile with the given emailaddress
     * 
     * @param profile
     * @return row count
     * @throws Exception
     */
    public static int update(UserProfile profile) throws Exception {
        Connection con = null;
        int count = 0;
        try {

            con = getConnection();
            con.setAutoCommit(false);

            PreparedStatement stmt = con
                    .prepareStatement("UPDATE user_profile SET up_last = ?, up_first = ?, up_airport = ?, up_image = ? WHERE up_email = ?");
            stmt.setString(1, profile.getLastName());
            stmt.setString(2, profile.getFirstName());
            stmt.setString(3, profile.getIataCode());
            stmt.setBytes(4, profile.getImage());
            stmt.setString(5, profile.getEmailAddress());

            count = stmt.executeUpdate();

            con.commit();
            con.setAutoCommit(true);
        } catch (SQLException se) {
            con.rollback();
            con.setAutoCommit(true);
        } finally {
            if (con != null) {
                recycle(con);
            }
        }

        if (profile.isDefaultProfile()) {
            storeDefaultUserProfile(profile.getEmailAddress());
        } else {
            if (loadDefaultUserProfile().equals(profile.getEmailAddress())) {
                storeDefaultUserProfile("");
            }
        }

        return count;
    }

    /**
     * deletes all records with the given emailaddress
     * 
     * @param profile
     * @return row count
     * @throws Exception
     */
    public static int delete(UserProfile profile) throws Exception {
        Connection con = null;
        int count = 0;
        try {

            con = getConnection();
            con.setAutoCommit(false);

            PreparedStatement stmt = con.prepareStatement("DELETE FROM user_profile WHERE up_email = ?");
            stmt.setString(1, profile.getEmailAddress());

            count = stmt.executeUpdate();

            con.commit();
            con.setAutoCommit(true);
        } catch (SQLException se) {
            con.rollback();
            con.setAutoCommit(true);
        } finally {
            if (con != null) {
                recycle(con);
            }
        }

        if (loadDefaultUserProfile().equals(profile.getEmailAddress())) {
            storeDefaultUserProfile("");
        }

        return count;
    }

    /**
     * @return list of strings containing all emailadresses (alphabetical
     *         ordered)
     * @throws Exception
     */
    public static List getEmailAddresses() throws Exception {
        List result = new Vector();

        Connection con = null;
        try {

            con = getConnection();

            Statement stmt = con.createStatement();
            ResultSet rst = stmt.executeQuery("SELECT up_email FROM user_profile ORDER BY up_email");

            while (rst.next()) {
                result.add(rst.getString(1));
            }

            rst.close();
            stmt.close();

        } finally {
            if (con != null) {
                recycle(con);
            }
        }

        return result;
    }

    /**
     * @param emailAddress
     * @return user profile for given emailaddress (could be null)
     * @throws SQLException
     * @throws Exception
     */
    public static UserProfile getUserProfile(String emailAddress) throws SQLException {
        UserProfile result = null;

        Connection con = null;
        try {

            con = getConnection();

            PreparedStatement stmt = con
                    .prepareStatement("SELECT up_last, up_first, up_email, up_airport, up_image FROM user_profile WHERE up_email = ?");
            stmt.setString(1, emailAddress);
            ResultSet rst = stmt.executeQuery();

            while (rst.next()) {
                result = new UserProfile();
                result.setLastName(rst.getString(1));
                result.setFirstName(rst.getString(2));
                result.setEmailAddress(rst.getString(3));
                result.setIataCode(rst.getString(4));
                result.setImage(rst.getBytes(5));
            }

            rst.close();
            stmt.close();

        } finally {
            if (con != null) {
                recycle(con);
            }
        }

        checkDefaultProfile(result);

        return result;
    }

    // handle default profile

    /**
     * @return default user profile or empty profile (if no default's available)
     * @throws SQLException
     * @throws Exception
     * @throws Exception
     */
    public static UserProfile getDefaultUserProfile() throws SQLException {
        UserProfile profile = getUserProfile(loadDefaultUserProfile());
        if (profile == null) {
            profile = createEmptyProfile();
        }

        return profile;
    }

    /**
     * set default user profile in preferences
     * 
     * @param profile
     */
    public static void storeDefaultUserProfile(UserProfile profile) {
        storeDefaultUserProfile(profile.getEmailAddress());
    }

    /**
     * stores default profile (emailaddress) in preferences
     * 
     * @param emailAddress
     */
    public static void storeDefaultUserProfile(String emailAddress) {
        getPreferences().setValue(PREF_DEFAULT_PROFILE, emailAddress);
    }

    /**
     * @return default user profiles email address from preferences. could be
     *         null.
     */
    public static String loadDefaultUserProfile() {
        return getPreferences().getString(PREF_DEFAULT_PROFILE);
    }

    /**
     * @param profile
     *            check wether the profile is the default profile and sets
     *            profile.isDefaultProfile
     */
    public static void checkDefaultProfile(UserProfile profile) {
        if (profile != null) {
            String defaultProfileEmailAddress = loadDefaultUserProfile();
            if (defaultProfileEmailAddress.equals(profile.getEmailAddress())) {
                profile.setDefaultProfile(true);
            } else {
                profile.setDefaultProfile(false);
            }
        }
    }

    /**
     * @return empty profile
     */
    public static UserProfile createEmptyProfile() {
        UserProfile profile = new UserProfile();

        return profile;
    }

}
