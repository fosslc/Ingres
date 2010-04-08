package com.ingres.demoapp.persistence;

import java.sql.Connection;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

import com.ingres.demoapp.model.Version;

public class VersionDAO extends BaseDAO {
    
    public static Version getVersion() throws SQLException {
        Version result = null;
        
        Connection con = null;
        try {

            con = getConnection();

            Statement stmt = con.createStatement();

            ResultSet rst = stmt.executeQuery("SELECT ver_id, ver_major, ver_minor, ver_release, ver_date, ver_install FROM version ORDER BY ver_id DESC");

            if (rst.next()) {
                result = new Version();
                result.setVerId(new Integer(rst.getInt(1)));
                result.setVerMajor(new Integer(rst.getInt(2)));
                result.setVerMinor(new Integer(rst.getInt(3)));
                result.setVerRelease(new Integer(rst.getInt(4)));
                result.setVerDate(rst.getTimestamp(5));
                result.setVerInstall(rst.getTimestamp(5));
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
