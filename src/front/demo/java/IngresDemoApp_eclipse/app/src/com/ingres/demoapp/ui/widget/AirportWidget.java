package com.ingres.demoapp.ui.widget;

import java.sql.SQLException;
import java.util.List;

import org.eclipse.jface.viewers.ComboViewer;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;

import com.ingres.demoapp.DemoApp;
import com.ingres.demoapp.model.Airport;
import com.ingres.demoapp.model.Country;
import com.ingres.demoapp.model.Region;
import com.ingres.demoapp.persistence.AirportDAO;
import com.ingres.demoapp.persistence.CountryDAO;

/**
 * This widget provides a convenience way to select an airport. The selection is
 * done in three steps:<br>
 * <ol>
 * <li>Selection of the country</li>
 * <li>Selection of the region</li>
 * <li>Selection of the airport</li>
 * </ol>
 * At initialization only the the country combo is enabled. After selecting a
 * country, the region combo is filled with associated regions and finally
 * enabled. When selecting a region the airport combo is filled with associated
 * airports and enabled. The content of the combo boxes is dynamically loaded
 * from the configured database.
 */
public final class AirportWidget extends Composite {

    private Label countryLabel = null;

    private Combo countryCombo = null;

    private Label regionLabel = null;

    private Label airportCodeLabel = null;

    private Combo regionCombo = null;

    private Combo airportCodeCombo = null;

    private ComboViewer countryViewer = null;

    private ComboViewer regionViewer = null;

    private ComboViewer airportCodeViewer = null;

    /**
     * Constructs the control and initializes the country combo.
     * 
     * @param parent
     *            a widget which will be the parent of the new instance (cannot
     *            be null)
     * @param style
     *            style the style of widget to construct
     * @throws SQLException
     *             problem while getting country list
     */
    public AirportWidget(Composite parent, int style) throws SQLException {
        super(parent, style);
        initialize();
        initializeCountryCombo();
    }

    /**
     * Initializes country combo with values from database.
     * 
     * @throws SQLException
     *             problem while getting country list from database
     */
    private void initializeCountryCombo() throws SQLException {
        final List countries = CountryDAO.getCountries();
        Country[] list = new Country[countries.size()];
        for (int i = 0; i < countries.size(); i++) {
            list[i] = (Country) countries.get(i);
        }
        countryViewer.add(list);
    }

    /**
     * This method initializes the control.
     */
    private void initialize() {
        final GridData gridData3 = new GridData();
        gridData3.horizontalAlignment = org.eclipse.swt.layout.GridData.END;
        final GridData gridData2 = new GridData();
        gridData2.horizontalSpan = 2;
        final GridLayout gridLayout = new GridLayout();
        gridLayout.numColumns = 3;
        countryLabel = new Label(this, SWT.NONE);
        countryLabel.setText("Country");
        this.setLayout(gridLayout);
        createCountryCombo();
        setSize(new Point(300, 132));
        regionLabel = new Label(this, SWT.NONE);
        regionLabel.setText("Region");
        regionLabel.setLayoutData(gridData2);
        airportCodeLabel = new Label(this, SWT.NONE);
        airportCodeLabel.setText("Airport Code");
        airportCodeLabel.setLayoutData(gridData3);
        createRegionCombo();
        createAirportCodeCombo();
    }

    /**
     * This method initializes countryCombo
     * 
     */
    private void createCountryCombo() {
        final GridData gridData = new GridData();
        gridData.horizontalSpan = 2;
        gridData.horizontalAlignment = org.eclipse.swt.layout.GridData.END;
        gridData.widthHint = 165;
        countryCombo = new Combo(this, SWT.READ_ONLY);
        countryCombo.setLayoutData(gridData);
        countryViewer = new ComboViewer(getCountryCombo());

        countryViewer.setLabelProvider(new LabelProvider() {
            public String getText(Object element) {
                return ((Country) element).getCtName();
            }
        });

        countryViewer.addSelectionChangedListener(new ISelectionChangedListener() {

            public void selectionChanged(SelectionChangedEvent event) {
                final Country country = (Country) ((IStructuredSelection) event.getSelection()).getFirstElement();

                try {
                    // get list of regions
                    final List regions = AirportDAO.getRegionsByCountry(country.getCtCode());
                    Region[] list = new Region[regions.size()];
                    for (int i = 0; i < regions.size(); i++) {
                        list[i] = (Region) regions.get(i);
                    }
                    // handle region and airport combo
                    regionCombo.removeAll();
                    regionViewer.add(list);
                    regionCombo.setEnabled(true);
                    airportCodeCombo.removeAll();
                    airportCodeCombo.setEnabled(false);
                } catch (SQLException e) {
                    // disable region and country combo
                    regionCombo.removeAll();
                    regionCombo.setEnabled(false);
                    airportCodeCombo.removeAll();
                    airportCodeCombo.setEnabled(false);

                    // show error message and log the exception
                    DemoApp.error("Exception occured while database access", e);
                }
            }
        });
    }

    /**
     * This method initializes regionCombo
     * 
     */
    private void createRegionCombo() {
        final GridData gridData1 = new GridData();
        gridData1.horizontalSpan = 2;
        gridData1.widthHint = 115;
        regionCombo = new Combo(this, SWT.READ_ONLY);
        regionCombo.setEnabled(false);
        regionCombo.setLayoutData(gridData1);
        regionViewer = new ComboViewer(getRegionCombo());

        regionViewer.setLabelProvider(new LabelProvider() {
            public String getText(Object element) {
                return ((Region) element).getApPlace();
            }
        });

        regionViewer.addSelectionChangedListener(new ISelectionChangedListener() {
            public void selectionChanged(SelectionChangedEvent event) {
                Region region = (Region) ((IStructuredSelection) event.getSelection()).getFirstElement();
                try {
                    // get list of airports
                    List airports = AirportDAO.getAirportsByRegion(region.getApCcode(), region.getApPlace());
                    Airport[] list = new Airport[airports.size()];
                    for (int i = 0; i < airports.size(); i++) {
                        list[i] = (Airport) airports.get(i);
                    }

                    // handle airport combo
                    airportCodeCombo.removeAll();
                    airportCodeViewer.add(list);
                    airportCodeCombo.setEnabled(true);
                } catch (Exception e) {
                    // disable airport combo
                    airportCodeCombo.removeAll();
                    airportCodeCombo.setEnabled(true);

                    // show error message and log the exception
                    DemoApp.error("Exception occured while database access", e);
                }
            }
        });

    }

    /**
     * This method initializes airportCodeCombo
     * 
     */
    private void createAirportCodeCombo() {
        GridData gridData4 = new GridData();
        gridData4.widthHint = 60;
        gridData4.horizontalAlignment = org.eclipse.swt.layout.GridData.END;
        airportCodeCombo = new Combo(this, SWT.READ_ONLY);
        airportCodeCombo.setEnabled(false);
        airportCodeCombo.setLayoutData(gridData4);
        airportCodeViewer = new ComboViewer(getAirportCodeCombo());
        
        airportCodeViewer.setLabelProvider(new LabelProvider() {
            public String getText(Object element) {
                return ((Airport) element).getApIatacode();
            }
        });
    }
    
    public Combo getAirportCodeCombo() {
        return airportCodeCombo;
    }

    public Combo getCountryCombo() {
        return countryCombo;
    }

    public Combo getRegionCombo() {
        return regionCombo;
    }

    public ComboViewer getAirportCodeViewer() {
        return airportCodeViewer;
    }

    public ComboViewer getCountryViewer() {
        return countryViewer;
    }

    public ComboViewer getRegionViewer() {
        return regionViewer;
    }

    /**
     * En/disables the controls of this widget. When no country is selected the
     * region is always disabled. When no region is selected the airport is
     * always disabled.
     * 
     * @param enabled
     *            the state to set
     * 
     * @see org.eclipse.swt.widgets.Control#setEnabled(boolean)
     */
    public void setEnabled(final boolean enabled) {
        super.setEnabled(enabled);
        countryCombo.setEnabled(enabled);
        countryLabel.setEnabled(enabled);
        if (countryCombo.getSelectionIndex() > -1) {
            regionCombo.setEnabled(enabled);
        } else {
            regionCombo.setEnabled(false);
        }
        regionLabel.setEnabled(enabled);
        if (regionCombo.getSelectionIndex() > -1) {
            airportCodeCombo.setEnabled(enabled);
        } else {
            airportCodeCombo.setEnabled(false);
        }
        airportCodeLabel.setEnabled(enabled);
    }

} // @jve:decl-index=0:visual-constraint="10,10"
