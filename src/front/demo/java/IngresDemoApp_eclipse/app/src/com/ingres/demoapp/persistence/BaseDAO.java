package com.ingres.demoapp.persistence;

import java.sql.Connection;
import java.sql.SQLException;

public class BaseDAO {

    public static Connection getConnection() throws SQLException {
        return ConnectionFactory.INSTANCE.getConnection();
    }

    public static void recycle(Connection connection) throws SQLException {
        ConnectionFactory.INSTANCE.recycle(connection);
    }

}
