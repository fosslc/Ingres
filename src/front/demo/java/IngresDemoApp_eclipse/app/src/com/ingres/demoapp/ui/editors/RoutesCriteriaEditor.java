package com.ingres.demoapp.ui.editors;

import java.sql.SQLException;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.jface.viewers.IStructuredContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ListViewer;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.List;
import org.eclipse.ui.IEditorInput;
import org.eclipse.ui.IEditorSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.part.EditorPart;

import com.ingres.demoapp.model.Airport;
import com.ingres.demoapp.model.Country;
import com.ingres.demoapp.model.Region;
import com.ingres.demoapp.model.RoutesCriteria;
import com.ingres.demoapp.persistence.AirportDAO;
import com.ingres.demoapp.persistence.CountryDAO;
import com.ingres.demoapp.ui.HelpConstants;
import com.ingres.demoapp.ui.widget.AirportWidget;

public class RoutesCriteriaEditor extends EditorPart {

    static class FlightDay {
        int id;

        String dayOfWeek;

        public FlightDay(int id, String dayOfWeek) {
            this.id = id;
            this.dayOfWeek = dayOfWeek;
        }
    }

    public static FlightDay[] getFlightDays() {
        FlightDay[] result = new FlightDay[7];
        result[0] = new FlightDay(0, "Monday");
        result[1] = new FlightDay(1, "Tuesday");
        result[2] = new FlightDay(2, "Wednesday");
        result[3] = new FlightDay(3, "Thursday");
        result[4] = new FlightDay(4, "Friday");
        result[5] = new FlightDay(5, "Saturday");
        result[6] = new FlightDay(6, "Sunday");
        return result;
    }

    public void initializeToolTips() {
        includeReturnJourneyCheckBox.setToolTipText("Route Criteria\nSet this checkbox to include return journeys too.");
        goButton.setToolTipText("Route Criteria\nFind routes...");
        newSearchButton.setToolTipText("Route Criteria\nClear your choices and start again...");
        checkBox.setToolTipText("Route Criteria\nSet this checkbox to choose any day of the week.");
        departingAirport.getCountryCombo().setToolTipText("Route Criteria\nChoose the country you are travelling from...");
        departingAirport.getAirportCodeCombo().setToolTipText("Route Criteria\nChoose the region you are travelling from...");
        departingAirport.getRegionCombo().setToolTipText("Route Criteria\nChoose the airport code you are travelling from...");
        arrivingAirport.getCountryCombo().setToolTipText("Route Criteria\nChoose the country you are travelling to...");
        arrivingAirport.getAirportCodeCombo().setToolTipText("Route Criteria\nChoose the region you are travelling to...");
        arrivingAirport.getRegionCombo().setToolTipText("Route Criteria\nChoose the airport code you are travelling to...");
        flightDaysList.setToolTipText("Route Criteria\nChoose the days of the week the flight must operate.");
    }

    public void initializeDynamicHelp() {
        PlatformUI.getWorkbench().getHelpSystem().setHelp(getSite().getShell(), HelpConstants.ROUTES_CRITERIA_EDITOR);
    }

    public static final String ID = "com.ingres.demoapp.ui.editors.RoutesEditor"; // @jve:decl-index=0:

    private Composite top = null;

    private Group routeCriteriaGroup = null;

    private Group departingGroup = null;

    private Group arrivingGroup = null;

    private Group flightDaysGroup = null;

    private Button includeReturnJourneyCheckBox = null;

    private AirportWidget departingAirport = null;

    private AirportWidget arrivingAirport = null;

    private Button checkBox = null;

    private List flightDaysList = null;

    private Label spacer = null;

    private Composite composite = null;

    private Button goButton = null;

    private Button newSearchButton = null;

    public void doSave(IProgressMonitor monitor) {
        // empty
    }

    public void doSaveAs() {
        // empty
    }

    RoutesCriteria criteria; // @jve:decl-index=0:

    private ListViewer listViewer = null;

    private RoutesCriteria defaultCriteria;

    public void init(IEditorSite site, IEditorInput input) throws PartInitException {
        criteria = (RoutesCriteria) input.getAdapter(RoutesCriteria.class);
        defaultCriteria = (RoutesCriteria) criteria.clone();
        setSite(site);
        setInput(input);
    }

    public boolean isDirty() {
        return false;
    }

    public boolean isSaveAsAllowed() {
        return false;
    }

    public void createPartControl(Composite parent) {
        top = new Composite(parent, SWT.NONE);
        top.setLayout(new GridLayout());

        GridLayout gridLayout2 = new GridLayout();
        gridLayout2.numColumns = 1;
        try {
            createRouteCriteriaGroup();
        } catch (SQLException e) {
            e.printStackTrace();
        }
        top.setLayout(gridLayout2);
        createComposite();

        initializeToolTips();
        initializeDynamicHelp();
        initializeContent();

    }

    private void initializeContent() {
        try {
            java.util.List airports = AirportDAO.getAirportByIataCode(defaultCriteria.getDepartingAirport());
            if (airports.size() > 0) {
                Airport home = (Airport) airports.get(0);
                Region region = AirportDAO.getRegionByAirport(home.getApIatacode());
                Country country = CountryDAO.getCountryByCountryCode(home.getApCcode());

                departingAirport.getCountryViewer().setSelection(new StructuredSelection(country), true);
                departingAirport.getRegionViewer().setSelection(new StructuredSelection(region), true);
                departingAirport.getAirportCodeViewer().setSelection(new StructuredSelection(home), true);
            } else {
                departingAirport.getCountryCombo().deselectAll();
                departingAirport.getRegionCombo().removeAll();
                departingAirport.getRegionCombo().setEnabled(false);
                departingAirport.getAirportCodeCombo().removeAll();
                departingAirport.getAirportCodeCombo().setEnabled(false);
            }

            arrivingAirport.getCountryCombo().deselectAll();
            arrivingAirport.getRegionCombo().removeAll();
            arrivingAirport.getRegionCombo().setEnabled(false);
            arrivingAirport.getAirportCodeCombo().removeAll();
            arrivingAirport.getAirportCodeCombo().setEnabled(false);

            checkBox.setSelection(true);

            flightDaysList.deselectAll();
            flightDaysList.setEnabled(false);

            includeReturnJourneyCheckBox.setSelection(false);
        } catch (Exception e) {
            e.printStackTrace();
        }

    }

    /**
     * This method initializes routeCriteriaGroup
     * 
     * @throws SQLException
     * 
     */
    private void createRouteCriteriaGroup() throws SQLException {
        GridData gridData1 = new GridData();
        gridData1.horizontalSpan = 2;
        GridLayout gridLayout = new GridLayout();
        gridLayout.numColumns = 3;
        routeCriteriaGroup = new Group(top, SWT.NONE);
        createDepartingGroup();
        routeCriteriaGroup.setLayout(gridLayout);
        createArrivingGroup();
        routeCriteriaGroup.setText("Route Criteria");
        createFlightDaysGroup();
        includeReturnJourneyCheckBox = new Button(routeCriteriaGroup, SWT.CHECK);
        includeReturnJourneyCheckBox.setText("Include return jouney");
        includeReturnJourneyCheckBox.setLayoutData(gridData1);
    }

    /**
     * This method initializes departingGroup
     * 
     * @throws SQLException
     * 
     */
    private void createDepartingGroup() throws SQLException {
        GridData gridData2 = new GridData();
        gridData2.verticalAlignment = org.eclipse.swt.layout.GridData.BEGINNING;
        departingGroup = new Group(routeCriteriaGroup, SWT.NONE);
        departingGroup.setLayout(new GridLayout());
        departingGroup.setLayoutData(gridData2);
        createDepartingAirport();
        departingGroup.setText("Departing");
    }

    /**
     * This method initializes arrivingGroup
     * 
     * @throws SQLException
     * 
     */
    private void createArrivingGroup() throws SQLException {
        GridData gridData3 = new GridData();
        gridData3.verticalAlignment = org.eclipse.swt.layout.GridData.BEGINNING;
        arrivingGroup = new Group(routeCriteriaGroup, SWT.NONE);
        arrivingGroup.setLayout(new GridLayout());
        arrivingGroup.setLayoutData(gridData3);
        createArrivingAirport();
        arrivingGroup.setText("Arriving");
    }

    /**
     * This method initializes flightDaysGroup
     * 
     */
    private void createFlightDaysGroup() {
        GridData gridData21 = new GridData();
        gridData21.widthHint = 85;
        gridData21.heightHint = 68;
        GridData gridData11 = new GridData();
        gridData11.widthHint = 13;
        GridData gridData4 = new GridData();
        gridData4.horizontalSpan = 2;
        GridLayout gridLayout1 = new GridLayout();
        gridLayout1.numColumns = 2;
        GridData gridData = new GridData();
        gridData.verticalSpan = 2;
        gridData.verticalAlignment = org.eclipse.swt.layout.GridData.BEGINNING;
        flightDaysGroup = new Group(routeCriteriaGroup, SWT.NONE);
        flightDaysGroup.setLayoutData(gridData);
        flightDaysGroup.setLayout(gridLayout1);
        flightDaysGroup.setText("Flying On");
        checkBox = new Button(flightDaysGroup, SWT.CHECK);
        checkBox.setText("Any day");
        checkBox.setSelection(true);
        checkBox.setLayoutData(gridData4);
        checkBox.addSelectionListener(new org.eclipse.swt.events.SelectionAdapter() {
            public void widgetSelected(org.eclipse.swt.events.SelectionEvent e) {
                flightDaysList.setEnabled(!checkBox.getSelection());
            }
        });
        spacer = new Label(flightDaysGroup, SWT.NONE);
        spacer.setText("");
        spacer.setLayoutData(gridData11);
        flightDaysList = new List(flightDaysGroup, SWT.BORDER | SWT.V_SCROLL | SWT.MULTI);
        flightDaysList.setEnabled(false);
        flightDaysList.setLayoutData(gridData21);
        listViewer = new ListViewer(flightDaysList);
        listViewer.setContentProvider(new IStructuredContentProvider() {

            public Object[] getElements(Object inputElement) {
                return (Object[]) inputElement;
            }

            public void dispose() {
            }

            public void inputChanged(Viewer viewer, Object oldInput, Object newInput) {
            }
        });
        listViewer.setLabelProvider(new ILabelProvider() {

            public Image getImage(Object element) {
                return null;
            }

            public String getText(Object element) {
                return ((FlightDay) element).dayOfWeek;
            }

            public void addListener(ILabelProviderListener listener) {
                // empty
            }

            public void dispose() {
                // empty
            }

            public boolean isLabelProperty(Object element, String property) {
                return false;
            }

            public void removeListener(ILabelProviderListener listener) {
            }
        });
        listViewer.setInput(getFlightDays());
    }

    /**
     * This method initializes departingAirport
     * 
     * @throws SQLException
     * 
     */
    private void createDepartingAirport() throws SQLException {
        departingAirport = new AirportWidget(departingGroup, SWT.NONE);
    }

    /**
     * This method initializes arrivingAirport
     * 
     * @throws SQLException
     * 
     */
    private void createArrivingAirport() throws SQLException {
        arrivingAirport = new AirportWidget(arrivingGroup, SWT.NONE);
    }

    /**
     * This method initializes composite
     * 
     */
    private void createComposite() {
        GridData gridData7 = new GridData();
        gridData7.horizontalAlignment = org.eclipse.swt.layout.GridData.FILL;
        GridData gridData6 = new GridData();
        gridData6.horizontalAlignment = org.eclipse.swt.layout.GridData.FILL;
        GridLayout gridLayout3 = new GridLayout();
        gridLayout3.numColumns = 2;
        gridLayout3.marginHeight = 0;
        gridLayout3.marginWidth = 0;
        gridLayout3.makeColumnsEqualWidth = true;
        GridData gridData5 = new GridData();
        gridData5.horizontalAlignment = org.eclipse.swt.layout.GridData.END;
        composite = new Composite(top, SWT.NONE);
        composite.setLayoutData(gridData5);
        composite.setLayout(gridLayout3);
        goButton = new Button(composite, SWT.NONE);
        goButton.setText("Go");
        goButton.setLayoutData(gridData6);
        goButton.addSelectionListener(new org.eclipse.swt.events.SelectionAdapter() {
            public void widgetSelected(org.eclipse.swt.events.SelectionEvent e) {
                try {
                    boolean[] days;
                    if (checkBox.getSelection()) {
                        days = new boolean[0];
                    } else {
                        Object[] dayArray = ((IStructuredSelection) listViewer.getSelection()).toArray();
                        // seven weekdays
                        days = new boolean[] { false, false, false, false, false, false, false };
                        for (int i = 0; i < dayArray.length; i++) {
                            FlightDay day = (FlightDay) dayArray[i];
                            days[day.id] = true;
                        }

                    }
                    criteria.setFlyingOn(days);
                    criteria.setArrivingAirport(arrivingAirport.getAirportCodeCombo().getText());
                    criteria.setDepartingAirport(departingAirport.getAirportCodeCombo().getText());
                    criteria.setFindReturnJourney(includeReturnJourneyCheckBox.getSelection());
                    setInputWithNotify(new RoutesCriteriaInput(criteria));
                } catch (Exception e1) {
                    e1.printStackTrace();
                }
            }
        });
        newSearchButton = new Button(composite, SWT.NONE);
        newSearchButton.setText("New search");
        newSearchButton.setLayoutData(gridData7);
        newSearchButton.addSelectionListener(new org.eclipse.swt.events.SelectionAdapter() {
            public void widgetSelected(org.eclipse.swt.events.SelectionEvent e) {
                initializeContent();

                try {
                    boolean[] days;
                    if (checkBox.getSelection()) {
                        days = new boolean[0];
                    } else {
                        Object[] dayArray = ((IStructuredSelection) listViewer.getSelection()).toArray();
                        // seven weekdays
                        days = new boolean[] { false, false, false, false, false, false, false };
                        for (int i = 0; i < dayArray.length; i++) {
                            FlightDay day = (FlightDay) dayArray[i];
                            days[day.id] = true;
                        }

                    }
                    criteria.setFlyingOn(days);
                    criteria.setArrivingAirport(arrivingAirport.getAirportCodeCombo().getText());
                    criteria.setDepartingAirport(departingAirport.getAirportCodeCombo().getText());
                    criteria.setFindReturnJourney(includeReturnJourneyCheckBox.getSelection());
                    setInputWithNotify(new RoutesCriteriaInput(criteria));
                } catch (Exception e1) {
                    e1.printStackTrace();
                }

            }
        });
    }

    public void setFocus() {
        // empty
    }

    public Object getAdapter(Class adapter) {
        Object result = super.getAdapter(adapter);
        if (result == null && RoutesCriteria.class.equals(adapter)) {
            result = criteria;
        }
        return result;
    }

}
