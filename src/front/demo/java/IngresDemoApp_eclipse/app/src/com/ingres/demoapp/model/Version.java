package com.ingres.demoapp.model;

import java.lang.Integer;
import java.sql.Timestamp;

public class Version {

    private Integer verId;

    private Integer verMajor;

    private Integer verMinor;

    private Integer verRelease;

    private Timestamp verDate;

    private Timestamp verInstall;

    public Integer getVerId() {
        return verId;
    }

    public void setVerId(Integer verId) {
        this.verId = verId;
    }

    public Integer getVerMajor() {
        return verMajor;
    }

    public void setVerMajor(Integer verMajor) {
        this.verMajor = verMajor;
    }

    public Integer getVerMinor() {
        return verMinor;
    }

    public void setVerMinor(Integer verMinor) {
        this.verMinor = verMinor;
    }

    public Integer getVerRelease() {
        return verRelease;
    }

    public void setVerRelease(Integer verRelease) {
        this.verRelease = verRelease;
    }

    public Timestamp getVerDate() {
        return verDate;
    }

    public void setVerDate(Timestamp verDate) {
        this.verDate = verDate;
    }

    public Timestamp getVerInstall() {
        return verInstall;
    }

    public void setVerInstall(Timestamp verInstall) {
        this.verInstall = verInstall;
    }

}