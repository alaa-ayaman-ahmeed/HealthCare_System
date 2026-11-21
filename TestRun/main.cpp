
#include <iostream>
#include <string>
#include <limits>
#include <cstdlib>

#include "StorageManager.h"
#include "PrimaryIndex.h"
#include "SecondaryIndex.h"
#include "OperationsManager.h"
#include "QueryEngine.h"

using namespace std;


// =====================================
// Rebuild Secondary Indexes
// =====================================

bool rebuildSecondaryIndexes(
    StorageManager& storage,
    PrimaryIndex& primary,
    SecondaryIndex& docNameIdx,
    SecondaryIndex& docSpecIdx,
    SecondaryIndex& appDocIdx
) {

    // Start with empty secondary indexes
    docNameIdx.clear();
    docSpecIdx.clear();
    appDocIdx.clear();


    // Build Doctor Name and Specialty indexes
    for (const auto& entry : primary.getDoctorsIndex()) {

        char record[DOCTOR_RECORD_SIZE];

        storage.readAt(
            "doctors.dat",
            entry.offset,
            record,
            DOCTOR_RECORD_SIZE
        );

        string doctorName(record + 15, 30);
        string doctorSpecialty(record + 45, 20);

        if (!docNameIdx.add(doctorName, entry.key))
            return false;

        if (!docSpecIdx.add(doctorSpecialty, entry.key))
            return false;
    }


    // Build Appointments Doctor ID index
    for (const auto& entry : primary.getAppointmentsIndex()) {

        char record[APPOINTMENT_RECORD_SIZE];

        storage.readAt(
            "appointments.dat",
            entry.offset,
            record,
            APPOINTMENT_RECORD_SIZE
        );

        string doctorID(record + 45, 15);

        if (!appDocIdx.add(doctorID, entry.key))
            return false;
    }

    return true;
}


// =====================================
// Main Menu
// =====================================

void displayMenu() {

    cout << "\n";
    cout << "========================================\n";
    cout << "       HEALTHCARE MANAGEMENT SYSTEM\n";
    cout << "========================================\n";

    cout << "1.  Add New Doctor\n";
    cout << "2.  Add New Appointment\n";
    cout << "3.  Update Doctor Name\n";
    cout << "4.  Update Doctor ID\n";
    cout << "5.  Update Appointment Date\n";
    cout << "6.  Delete Appointment\n";
    cout << "7.  Delete Doctor\n";
    cout << "8.  Print Doctor Info\n";
    cout << "9.  Print Appointment Info\n";
    cout << "10. Write Query\n";
    cout << "11. Exit\n";

    cout << "========================================\n";
    cout << "Enter your choice: ";
}


// =====================================
// Main Function
// =====================================

int main() {

    // ---------------------------------
    // 1. Create Storage Manager
    // ---------------------------------

    StorageManager storage;

    storage.initializeFiles();


    // ---------------------------------
    // 2. Create and Build Primary Index
    // ---------------------------------

    PrimaryIndex primary(storage);

    if (!primary.buildIndexes()) {

        cout << "Error: Could not build Primary Indexes.\n";
        return 1;
    }


    // ---------------------------------
    // 3. Create Secondary Indexes
    // ---------------------------------

    SecondaryIndex docNameIdx(
        "docname_keys.idx",
        "docname_list.dat",
        30
    );

    SecondaryIndex docSpecIdx(
        "docspec_keys.idx",
        "docspec_list.dat",
        20
    );

    SecondaryIndex appDocIdx(
        "appdoc_keys.idx",
        "appdoc_list.dat",
        15
    );


    // ---------------------------------
    // 4. Rebuild Secondary Indexes
    // ---------------------------------

    if (!rebuildSecondaryIndexes(
        storage,
        primary,
        docNameIdx,
        docSpecIdx,
        appDocIdx
    )) {

        cout << "Error: Could not rebuild Secondary Indexes.\n";
        return 1;
    }


    // ---------------------------------
    // 5. Create Operations Manager
    // ---------------------------------

    OperationsManager operations(
        storage,
        primary,
        docNameIdx,
        docSpecIdx,
        appDocIdx
    );


    // ---------------------------------
    // 6. Create Query Engine
    // ---------------------------------

    QueryEngine queryEngine(
        storage,
        primary,
        docNameIdx,
        docSpecIdx,
        appDocIdx
    );


    cout << "\nSystem initialized successfully!\n";


    // ---------------------------------
    // 7. Main Menu Loop
    // ---------------------------------

    int choice;

    while (true) {

        displayMenu();

        if (!(cin >> choice)) {

            cout << "Invalid input. Please enter a number.\n";

            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            continue;
        }

        cin.ignore(numeric_limits<streamsize>::max(), '\n');


        // ---------------------------------
        // OPTION 1: Add New Doctor
        // ---------------------------------

        if (choice == 1) {

            string id, name, specialty, address;

            cout << "\nEnter Doctor ID: ";
            getline(cin, id);

            cout << "Enter Doctor Name: ";
            getline(cin, name);

            cout << "Enter Doctor Specialty: ";
            getline(cin, specialty);

            cout << "Enter Doctor Address: ";
            getline(cin, address);


            operations.addDoctor(id, name, specialty, address);

            // if (operations.addDoctor(id, name, specialty, address))
            //     cout << "\nDoctor added successfully!\n";

            // else
            //     cout << "\nFailed to add Doctor.\n";
        }


        // ---------------------------------
        // OPTION 2: Add New Appointment
        // ---------------------------------

        else if (choice == 2) {

            string appointmentID, date, doctorID;

            cout << "\nEnter Appointment ID: ";
            getline(cin, appointmentID);

            cout << "Enter Appointment Date: ";
            getline(cin, date);

            cout << "Enter Doctor ID: ";
            getline(cin, doctorID);


            if (operations.addAppointment(
                appointmentID,
                date,
                doctorID
            ))

                cout << "\nAppointment added successfully!\n";

            else
                cout << "\nFailed to add Appointment.\n";
        }


        // ---------------------------------
        // OPTION 3: Update Doctor Name
        // ---------------------------------

        else if (choice == 3) {

            string id, newName;

            cout << "\nEnter Doctor ID: ";
            getline(cin, id);

            cout << "Enter New Doctor Name: ";
            getline(cin, newName);


            if (operations.updateDoctorName(id, newName))
                cout << "\nDoctor Name updated successfully!\n";

            else
                cout << "\nFailed to update Doctor Name.\n";
        }


        // ---------------------------------
        // OPTION 4: Update Doctor ID
        // ---------------------------------

        else if (choice == 4) {

            string oldID, newID;

            cout << "\nEnter Current Doctor ID: ";
            getline(cin, oldID);

            cout << "Enter New Doctor ID: ";
            getline(cin, newID);


            if (operations.updateDoctorID(oldID, newID))
                cout << "\nDoctor ID updated successfully!\n";

            else
                cout << "\nFailed to update Doctor ID.\n";
        }


        // ---------------------------------
        // OPTION 5: Update Appointment Date
        // ---------------------------------

        else if (choice == 5) {

            string id, newDate;

            cout << "\nEnter Appointment ID: ";
            getline(cin, id);

            cout << "Enter New Appointment Date: ";
            getline(cin, newDate);


            if (operations.updateAppointmentDate(id, newDate))
                cout << "\nAppointment Date updated successfully!\n";

            else
                cout << "\nFailed to update Appointment Date.\n";
        }


        // ---------------------------------
        // OPTION 6: Delete Appointment
        // ---------------------------------

        else if (choice == 6) {

            string id;

            cout << "\nEnter Appointment ID to delete: ";
            getline(cin, id);


            if (operations.deleteAppointment(id))
                cout << "\nAppointment deleted successfully!\n";

            else
                cout << "\nFailed to delete Appointment.\n";
        }


        // ---------------------------------
        // OPTION 7: Delete Doctor
        // ---------------------------------

        else if (choice == 7) {

            string id;

            cout << "\nEnter Doctor ID to delete: ";
            getline(cin, id);


            if (operations.deleteDoctor(id))
                cout << "\nDoctor deleted successfully!\n";

            else
                cout << "\nFailed to delete Doctor.\n";
        }


        // ---------------------------------
        // OPTION 8: Print Doctor Info
        // ---------------------------------

        else if (choice == 8) {

            string id;

            cout << "\nEnter Doctor ID: ";
            getline(cin, id);

            string query =
                "SELECT * FROM Doctors WHERE DoctorID = '" + id + "'";

            queryEngine.executeQuery(query);
        }


        // ---------------------------------
        // OPTION 9: Print Appointment Info
        // ---------------------------------

        else if (choice == 9) {

            string id;

            cout << "\nEnter Appointment ID: ";
            getline(cin, id);

            string query =
                "SELECT * FROM Appointments WHERE AppointmentID = '" + id + "'";

            queryEngine.executeQuery(query);
        }


        // ---------------------------------
        // OPTION 10: Write Query
        // ---------------------------------

        else if (choice == 10) {

            string query;

            cout << "\nEnter your Query:\n";

            getline(cin, query);

            queryEngine.executeQuery(query);
        }


        // ---------------------------------
        // OPTION 11: Exit
        // ---------------------------------

        else if (choice == 11) {

            cout << "\nThank you for using Healthcare Management System!\n";

            break;
        }


        // ---------------------------------
        // INVALID CHOICE
        // ---------------------------------

        else {

            cout << "\nInvalid choice. Please select from 1 to 11.\n";
        }
    }

    return 0;
}
