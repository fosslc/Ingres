package com.ingres.demoapp.ui.views;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.List;

import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.jface.viewers.IStructuredContentProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.ui.IPartListener2;
import org.eclipse.ui.IWorkbenchPartReference;
import org.eclipse.ui.part.ViewPart;

import com.ingres.demoapp.model.Route;
import com.ingres.demoapp.model.RoutesCriteria;
import com.ingres.demoapp.persistence.RouteDAO;
import com.ingres.demoapp.ui.editors.RoutesCriteriaEditor;

public class RoutesView extends ViewPart implements IPartListener2 {

    public static final String ID = "com.ingres.demoapp.ui.views.RoutesView";

    private Composite top = null;

    private Table table = null;

    private TableViewer tableViewer = null;

    public void createPartControl(Composite parent) {
        top = new Composite(parent, SWT.NONE);
        top.setLayout(new FillLayout());
        table = new Table(top, SWT.NONE);
        table.setHeaderVisible(true);
        table.setLinesVisible(true);
        tableViewer = new TableViewer(table);
        TableColumn tableColumn = new TableColumn(table, SWT.NONE);
        tableColumn.setWidth(60);
        tableColumn.setText("Airline");
        TableColumn tableColumn1 = new TableColumn(table, SWT.NONE);
        tableColumn1.setWidth(60);
        tableColumn1.setText("IATA");
        TableColumn tableColumn2 = new TableColumn(table, SWT.NONE);
        tableColumn2.setWidth(60);
        tableColumn2.setText("Flight No");
        TableColumn tableColumn3 = new TableColumn(table, SWT.NONE);
        tableColumn3.setWidth(60);
        tableColumn3.setText("Depart from");
        TableColumn tableColumn4 = new TableColumn(table, SWT.NONE);
        tableColumn4.setWidth(60);
        tableColumn4.setText("Arrive to");
        TableColumn tableColumn5 = new TableColumn(table, SWT.NONE);
        tableColumn5.setWidth(60);
        tableColumn5.setText("Dep time");
        TableColumn tableColumn6 = new TableColumn(table, SWT.NONE);
        tableColumn6.setWidth(60);
        tableColumn6.setText("Arrival time");
        TableColumn tableColumn8 = new TableColumn(table, SWT.NONE);
        tableColumn8.setWidth(60);
        tableColumn8.setText("Days later");
        TableColumn tableColumn9 = new TableColumn(table, SWT.NONE);
        tableColumn9.setWidth(60);
        tableColumn9.setText("On Days");

        tableViewer.setContentProvider(new IStructuredContentProvider() {

            public Object[] getElements(Object inputElement) {
                return ((List) inputElement).toArray();
            }

            public void dispose() {
            }

            public void inputChanged(Viewer viewer, Object oldInput, Object newInput) {
            }
        });

        tableViewer.setLabelProvider(new ITableLabelProvider() {

            public Image getColumnImage(Object element, int columnIndex) {
                return null;
            }

            public String getColumnText(Object element, int columnIndex) {
                SimpleDateFormat format = new SimpleDateFormat("HH:mm");
                switch (columnIndex) {
                case 0:
                    return ((Route) element).getAirline();
                case 1:
                    return ((Route) element).getIataCode();
                case 2:
                    return ((Route) element).getFlightNum().toString();
                case 3:
                    return ((Route) element).getDepartFrom();
                case 4:
                    return ((Route) element).getArriveTo();
                case 5:
                    return format.format(((Route) element).getDepartAt());
                case 6:
                    return format.format(((Route) element).getArriveAt());
                case 7:
                    return ((Route) element).getArriveOffset().toString();
                case 8:
                    return ((Route) element).getFlightDays();
                default:
                    break;
                }

                return null;
            }

            public void addListener(ILabelProviderListener listener) {
            }

            public void dispose() {
            }

            public boolean isLabelProperty(Object element, String property) {
                return false;
            }

            public void removeListener(ILabelProviderListener listener) {
            }
        });

        getSite().getWorkbenchWindow().getPartService().addPartListener(this);
    }

    public void setFocus() {
        // empty
    }

    public void partActivated(IWorkbenchPartReference partRef) {
        // empty
    }

    public void partBroughtToTop(IWorkbenchPartReference partRef) {
        // empty
    }

    public void partClosed(IWorkbenchPartReference partRef) {
        // empty
    }

    public void partDeactivated(IWorkbenchPartReference partRef) {
    }

    public void partHidden(IWorkbenchPartReference partRef) {
        if (partRef.getId().equals(RoutesCriteriaEditor.ID)) {
            tableViewer.setInput(new ArrayList());
        }
    }

    public void partInputChanged(IWorkbenchPartReference partRef) {
        RoutesCriteria criteria = (RoutesCriteria) partRef.getPart(false).getAdapter(RoutesCriteria.class);
        if (criteria != null) {
            try {
                java.util.List routes = RouteDAO.getRoutes(criteria.getDepartingAirport(), criteria.getArrivingAirport(), criteria
                        .getFlyingOn(), criteria.isFindReturnJourney());
                tableViewer.setInput(routes);
            } catch (Exception e) {
                e.printStackTrace();
            }
        } else {
            tableViewer.setInput(new ArrayList());
        }
    }

    public void partOpened(IWorkbenchPartReference partRef) {
        // empty
    }

    public void partVisible(IWorkbenchPartReference partRef) {
        // empty
    }

    public void dispose() {
        getSite().getWorkbenchWindow().getPartService().removePartListener(this);
        super.dispose();
    }

}
