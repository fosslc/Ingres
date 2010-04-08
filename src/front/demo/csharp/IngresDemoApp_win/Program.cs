// Copyright (c) 2006 Ingres Corporation

using System;
using System.Collections.Generic;
using System.Windows.Forms;

namespace IngresDemoApp
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new IngresFrequentFlyer());
        }
    }
}