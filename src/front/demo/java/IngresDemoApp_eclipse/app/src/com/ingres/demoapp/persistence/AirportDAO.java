package com.ingres.demoapp.persistence;

import java.sql.CallableStatement;
import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.List;
import java.util.Vector;

import com.ingres.demoapp.model.Airport;
import com.ingres.demoapp.model.Region;

public class AirportDAO extends BaseDAO {
    
    public static Region getRegionByAirport(String apIataCode) throws SQLException {
        Region region = null;

        Connection con = null;
        try {
            con = getConnection();

            PreparedStatement stmt = con
                    .prepareStatement("SELECT DISTINCT ap_place, ap_ccode FROM airport WHERE ap_iatacode = ? ORDER BY ap_place");
            stmt.setString(1, apIataCode);
            ResultSet rst = stmt.executeQuery();
            while (rst.next()) {
                region = new Region();
                region.setApPlace((String) rst.getObject(1));
                region.setApCcode((String) rst.getObject(2));
            }

            rst.close();
            stmt.close();
        } finally {
            if (con != null) {
                recycle(con);
            }
        }

        return region;
    }

    public static List getRegionsByCountry(String apCcode) throws SQLException {
        List result = new Vector();

        Connection con = null;
        try {
            con = getConnection();

            PreparedStatement stmt = con
                    .prepareStatement("SELECT DISTINCT ap_place, ap_ccode FROM airport WHERE ap_ccode = ? ORDER BY ap_place");
            stmt.setString(1, apCcode);
            ResultSet rst = stmt.executeQuery();
            while (rst.next()) {
                Region region = new Region();
                region.setApPlace((String) rst.getObject(1));
                region.setApCcode((String) rst.getObject(2));
                result.add(region);
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

    public static List getAirportByIataCode(String iataCode) throws Exception {
        List result = new Vector();

        Connection con = null;
        try {
            con = getConnection();

            PreparedStatement stmt = con
                    .prepareStatement("SELECT ap_place, ap_ccode, ap_iatacode FROM airport WHERE ap_iatacode = ? ORDER BY ap_iatacode");
            stmt.setString(1, iataCode);
            ResultSet rst = stmt.executeQuery();
            while (rst.next()) {
                Airport airport = new Airport();
                airport.setApPlace((String) rst.getObject(1));
                airport.setApCcode((String) rst.getObject(2));
                airport.setApIatacode((String) rst.getObject(3));
                result.add(airport);
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

    public static List getAirportsByRegion(String apCcode, String apPlace) throws Exception {
        List result = new Vector();

        Connection con = null;
        try {
            con = getConnection();

            CallableStatement stmt = con.prepareCall("{call get_my_airports ( ccode = ?, area = ? )}");
            stmt.setString(1, apCcode);
            stmt.setString(2, apPlace);

            ResultSet rst = stmt.executeQuery();
            while (rst.next()) {
                Airport airport = new Airport();
                airport.setApIatacode((String) rst.getObject(1));
                airport.setApPlace((String) rst.getObject(2));
                airport.setApName((String) rst.getObject(3));
                result.add(airport);
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
