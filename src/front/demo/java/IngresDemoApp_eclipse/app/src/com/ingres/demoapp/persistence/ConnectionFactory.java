package com.ingres.demoapp.persistence;

import java.sql.Connection;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;
import java.util.Stack;

import javax.sql.DataSource;

import org.eclipse.core.runtime.Assert;

import com.ingres.demoapp.DemoApp;
import com.ingres.demoapp.model.DatabaseConfiguration;
import com.ingres.demoapp.persistence.DatabaseConfigurationDAO.DatabaseConfigurationChangeListener;
import com.ingres.jdbc.IngresDataSource;

public class ConnectionFactory implements DatabaseConfigurationChangeListener {

    // //////////////////////////////////////////
    // singleton
    // //////////////////////////////////////////

    public static final ConnectionFactory INSTANCE = new ConnectionFactory();

    private ConnectionFactory() {
        String ingresDataSource = "com.ingres.jdbc.IngresDataSource";

        try {
            Class dataSourceClass = Class.forName(ingresDataSource);
            dataSource = (DataSource) dataSourceClass.newInstance();
        } catch (Exception e) {
            // should never happen
            DemoApp.logError(e);
        }

        update(DatabaseConfigurationDAO.INSTANCE.getDatabaseConfiguration());
        DatabaseConfigurationDAO.INSTANCE.registerChangeListener(this);
    }

    // //////////////////////////////////////////
    // connection stuff
    // //////////////////////////////////////////

    public Connection getConnection() throws SQLException {
        Connection con;
        synchronized (this) {
            if (!free.empty()) {
                con = (Connection) free.pop();
                // TODO test wether connection is ready or not
            } else {
                con = createConnection();
            }
        }
        inUse.add(con);
        return con;
    }

    public void recycle(Connection connection) throws SQLException {
        Assert.isNotNull(connection);
        inUse.remove(connection);
        free.push(connection);
    }

    public void update(DatabaseConfiguration newConfig) {
        Assert.isNotNull(newConfig);

        configuration = newConfig;

        synchronized (this) {
            while (!free.empty()) {
                close((Connection) free.pop());
            }
            for (int i = 0; i < inUse.size(); i++) {
                close((Connection) inUse.get(i));
            }
            inUse.clear();

            ((IngresDataSource) dataSource).setDatabaseName(configuration.getDatabase());
            ((IngresDataSource) dataSource).setServerName(configuration.getHost());
            ((IngresDataSource) dataSource).setPassword(configuration.getPassword());
            // add 7 to instance name (XXX fixed value)
            ((IngresDataSource) dataSource).setPortName(configuration.getPort() + "7");
            ((IngresDataSource) dataSource).setUser(configuration.getUser());
        }
    }

    private DataSource dataSource;

    private DatabaseConfiguration configuration;

    // connection pooling

    private List inUse = new ArrayList();

    private Stack free = new Stack();

    private Connection createConnection() throws SQLException {
        return dataSource.getConnection();
    }

    private void close(Connection connection) {
        try {
            connection.close();
        } catch (SQLException e) {
            e.printStackTrace();
        }
    }

}
