package com.ingres.demoapp.persistence;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.List;
import java.util.Vector;

import com.ingres.demoapp.model.Country;

public class CountryDAO extends BaseDAO {

    public static Country getCountryByCountryCode(String countryCode) throws Exception {
        Country result = null;

        Connection con = null;
        try {

            con = getConnection();

            PreparedStatement stmt = con.prepareStatement("SELECT ct_name, ct_code FROM country WHERE ct_code = ? ORDER BY ct_name");
            stmt.setString(1, countryCode);
            ResultSet rst = stmt.executeQuery();
            while (rst.next()) {
                result = new Country();
                result.setCtName(rst.getString(1));
                result.setCtCode(rst.getString(2));
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

    public static List getCountries() throws SQLException {
        List result = new Vector();

        Connection con = null;
        try {

            con = getConnection();

            Statement stmt = con.createStatement();
            ResultSet rst = stmt.executeQuery("SELECT ct_name, ct_code FROM country ORDER BY ct_name");
            while (rst.next()) {
                Country country = new Country();
                country.setCtName(rst.getString(1));
                country.setCtCode(rst.getString(2));

                result.add(country);
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

}
