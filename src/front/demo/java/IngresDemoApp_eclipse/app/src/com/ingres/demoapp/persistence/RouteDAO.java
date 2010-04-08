package com.ingres.demoapp.persistence;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.List;
import java.util.Vector;

import com.ingres.demoapp.model.Route;

public class RouteDAO extends BaseDAO {

    private static String getFlightDays(boolean[] flightDays) {
        String result;

        if (flightDays.length == 0) {
            result = "%";
        } else {
            StringBuffer buf = new StringBuffer();
            boolean wildCardBefore = false;
            for (int i = 0; i < flightDays.length; i++) {
                if (flightDays[i]) {
                    buf.append(i + 1);
                    wildCardBefore = false;
                } else {
                    if (!wildCardBefore) {
                        buf.append("%");
                    }
                    wildCardBefore = true;
                }
            }
            result = buf.toString();
        }

        return result;
    }

    public static List getRoutes(String departFrom, String arriveTo, boolean[] days, boolean includeReturn) throws SQLException {
        List result = new Vector();

        String flightDay = getFlightDays(days);

        Connection con = null;
        try {

            con = getConnection();

            PreparedStatement stmt;
            if (includeReturn) {
                stmt = con
                        .prepareStatement("SELECT rt_airline, al_iatacode, al_name, rt_flight_num, rt_depart_from, rt_arrive_to, rt_depart_at, rt_arrive_at, rt_arrive_offset, rt_flight_day FROM route r, airline a WHERE ((rt_depart_from = ? and rt_arrive_to = ?) OR (rt_depart_from = ? and rt_arrive_to = ?)) and rt_flight_day LIKE ? and rt_airline = al_icaocode ORDER BY rt_airline, rt_flight_num");

                stmt.setString(1, departFrom);
                stmt.setString(2, arriveTo);
                stmt.setString(3, arriveTo);
                stmt.setString(4, departFrom);
                stmt.setString(5, flightDay);
            } else {
                stmt = con
                        .prepareStatement("SELECT rt_airline, al_iatacode, al_name, rt_flight_num, rt_depart_from, rt_arrive_to, rt_depart_at, rt_arrive_at, rt_arrive_offset, rt_flight_day FROM route r, airline a WHERE rt_depart_from = ? and rt_arrive_to = ? and rt_flight_day LIKE ? and rt_airline = al_icaocode ORDER BY rt_airline, rt_flight_num");

                stmt.setString(1, departFrom);
                stmt.setString(2, arriveTo);
                stmt.setString(3, flightDay);
            }

            ResultSet rst = stmt.executeQuery();
            while (rst.next()) {
                Route route = new Route();
                route.setAirline(rst.getString(1));
                route.setIataCode(rst.getString(2));
                route.setName(rst.getString(3));
                // java.lang.NoSuchMethodError:
                // java.lang.Integer.valueOf(I)Ljava/lang/Integer; with JDK 1.4
                route.setFlightNum(new Integer(rst.getInt(4)));
                route.setDepartFrom(rst.getString(5));
                route.setArriveTo(rst.getString(6));
                route.setDepartAt(rst.getTimestamp(7));
                route.setArriveAt(rst.getTimestamp(8));
                // java.lang.NoSuchMethodError:
                // java.lang.Integer.valueOf(I)Ljava/lang/Integer; with JDK 1.4
                route.setArriveOffset(new Integer(rst.getInt(9)));
                route.setFlightDays(rst.getString(10));

                result.add(route);
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
